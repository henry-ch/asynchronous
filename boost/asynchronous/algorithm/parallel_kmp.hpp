// Boost.Asynchronous library
//  Copyright (C) Tobias Holl 2018
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNCHRONOUS_PARALLEL_KMP_HPP
#define BOOST_ASYNCHRONOUS_PARALLEL_KMP_HPP

#include <functional>
#include <iterator>
#include <memory>
#include <type_traits>

#include <boost/asynchronous/algorithm/detail/safe_advance.hpp>
#include <boost/asynchronous/algorithm/then.hpp>
#include <boost/asynchronous/continuation_task.hpp>
#include <boost/asynchronous/detail/job_type_from.hpp>


namespace boost
{
namespace asynchronous
{

// This is a parallel implementation of the Knuth-Morris-Pratt search algorithm.
// It is mostly intended for substring search, but works for any type of iterable range.
// N.B. that this implementation (unlike most other parallel implementations of KMP)
// parallelizes over the length of the "haystack" string, not the number of searched
// "needles" (which is trivially done using other algorithms like parallel_for).

namespace detail
{

// The failure function of KMP describes how many characters of the haystack string we
// can skip when a mismatch occurs. This allows a more efficient search of the haystack,
// in O(n + m) instead of O(nm) time.
using kmp_failure_function = std::vector<ssize_t>;

// This function builds the "failure function"
template <typename Iterator>
kmp_failure_function build_kmp_failure_function(Iterator begin, Iterator end)
{
    auto items = std::distance(begin, end);
    kmp_failure_function T(items + 1);

    T[0] = -1;
    ssize_t candidate = 0;
    for (decltype(items) index = 1; index < items; ++index, ++candidate)
    {
        if (*(begin + index) == *(begin + candidate))
        {
            T[index] = T[candidate];
        }
        else
        {
            T[index] = candidate;
            do { candidate = T[candidate]; }
            while (candidate >= 0 && *(begin + index) != *(begin + candidate));
        }
    }
    T[items] = candidate;

    return T;
}

// The actual implementation of KMP.
template <typename NeedleIterator>
struct kmp_matcher
{
    kmp_matcher(NeedleIterator begin, NeedleIterator end)
        : needle_begin(begin)
        , needle_end(end)
        , needle_size(static_cast<ssize_t>(std::distance(begin, end)))
        , failure_function(boost::asynchronous::detail::build_kmp_failure_function(begin, end))
    {}

    template <typename HaystackIterator, typename HaystackIndex, typename Functor>
    void operator()(HaystackIterator begin, HaystackIterator end, HaystackIndex begin_index, Functor& functor) const
    {
        HaystackIndex haystack_offset = 0;
        HaystackIndex haystack_chunk_size = std::distance(begin, end);
        ssize_t needle_offset = 0;
        while (haystack_offset != haystack_chunk_size)
        {
            if (*(begin + haystack_offset) == *(needle_begin + needle_offset))
            {
                // Matched, advance both positions
                ++needle_offset;
                ++haystack_offset;
                // Check if we have matched all characters
                if (needle_offset == needle_size)
                {
                    functor(begin_index + haystack_offset - needle_size);
                    needle_offset = failure_function[needle_offset];
                }
            }
            else
            {
                // We failed, shift by the failure function
                needle_offset = failure_function[needle_offset];
                // If the failure function gives -1, advance the position in the haystack
                if (needle_offset < 0)
                {
                    ++needle_offset;
                    ++haystack_offset;
                }
            }
        }
    }

    NeedleIterator needle_begin;
    NeedleIterator needle_end;
    ssize_t        needle_size;
    kmp_failure_function failure_function;
};

template <typename Iterator, typename KmpMatcher, typename Functor, typename IndexType, typename Job>
struct parallel_kmp_helper : public boost::asynchronous::continuation_task<Functor>
{
    parallel_kmp_helper(Iterator first, Iterator last, KmpMatcher matcher, Functor functor, IndexType offset, long cutoff, const std::string& task_name, std::size_t prio)
        : boost::asynchronous::continuation_task<Functor>(task_name)
        , first_(first)
        , last_(last)
        , matcher_(std::move(matcher))
        , functor_(std::move(functor))
        , offset_(offset)
        , cutoff_(cutoff)
        , prio_(prio)
    {}

    void operator()()
    {
        boost::asynchronous::continuation_result<Functor> task_res = this->this_task_result();
        try
        {
            // Never find an empty needle
            if (matcher_.needle_size <= 0)
            {
                task_res.set_value(std::move(functor_));
                return;
            }

            // Advance first_, depending on the iterator type (either by the specified cutoff exactly, or to the half-way mark)
            Iterator middle = boost::asynchronous::detail::find_cutoff(first_, cutoff_, last_);

            // Advance middle by the length of the needle that we are searching for
            Iterator padded = middle;
            boost::asynchronous::detail::safe_advance(padded, matcher_.needle_size - 1, last_);

            // If we can't split further, run the KMP matcher. Otherwise, recurse.
            if (padded == last_)
            {
                matcher_(first_, padded, offset_, functor_);
                task_res.set_value(std::move(functor_));
            }
            else
            {
                // Compute the offset of the middle element
                IndexType middle_offset = offset_ + std::distance(first_, middle);

                // Recurse.
                boost::asynchronous::create_callback_continuation_job<Job>(
                    [task_res](std::tuple<boost::asynchronous::expected<Functor>, boost::asynchronous::expected<Functor>> result) mutable
                    {
                        try
                        {
                            // Check for exceptions
                            auto functor = std::get<0>(result).get();
                            functor.merge(std::move(std::get<1>(result).get()));
                            task_res.set_value(std::move(functor));
                        }
                        catch (...)
                        {
                            task_res.set_exception(std::current_exception());
                        }
                    },
                    parallel_kmp_helper<Iterator, KmpMatcher, Functor, IndexType, Job>(first_, padded, matcher_, functor_, offset_,       cutoff_, this->get_name(), prio_),
                    parallel_kmp_helper<Iterator, KmpMatcher, Functor, IndexType, Job>(middle, last_,  matcher_, functor_, middle_offset, cutoff_, this->get_name(), prio_)
                );
            }
        }
        catch (...)
        {
            task_res.set_exception(std::current_exception());
        }
    }

    Iterator    first_;
    Iterator    last_;
    KmpMatcher  matcher_;
    Functor     functor_;
    IndexType   offset_;
    long        cutoff_;
    std::size_t prio_;
};

// Pure iterator version (this is the base implementation that all other combinations of argument types rely on)
template <typename Job, typename HaystackIterator, typename NeedleIterator, typename Functor>
boost::asynchronous::detail::callback_continuation<Functor, Job>
parallel_kmp_impl(
    HaystackIterator first, HaystackIterator last,
    NeedleIterator s_first, NeedleIterator s_last,
    Functor functor, long cutoff,
    const std::string& task_name, std::size_t prio
)
{
    boost::asynchronous::detail::kmp_matcher<NeedleIterator> matcher(s_first, s_last);
    return boost::asynchronous::top_level_callback_continuation_job<Functor, Job>(
        boost::asynchronous::detail::parallel_kmp_helper<HaystackIterator, decltype(matcher), Functor, decltype(std::distance(first, last)), Job>(
            first, last, std::move(matcher), std::move(functor), 0, cutoff, task_name, prio
        )
    );
}

// Check if a functor type is a valid functor for parallel_kmp
template <typename F>
struct is_kmp_functor
{
private:
    // Check for the function call operator (operator()(const index_type&))
    template <typename T> static auto has_call_operator(T* t) -> decltype((*t)(0), void(), std::true_type{});
    template <typename T> static auto has_call_operator(...)  -> decltype(std::false_type{});
    // Check for the merge operation (merge(const functor_type&) or merge(functor_type&&))
    template <typename T> static auto has_merge(T* a, T* b) -> decltype(a->merge(std::move(*b)), void(), std::true_type{});
    template <typename T> static auto has_merge(...)        -> decltype(std::false_type{});

public:
    constexpr static bool value = decltype(has_call_operator<F>(nullptr))::value && decltype(has_merge<F>(nullptr, nullptr))::value;
};

} // detail

// Preprocessor magic to generate all 16 different combinations.

// BOOST_ASYNC_KMP_IMPL_CHECK_...: Checks the conditions on the specific argument type
#define BOOST_ASYNC_KMP_IMPL_CHECK_ITER(argtype) true
#define BOOST_ASYNC_KMP_IMPL_CHECK_RNGE(argtype) !boost::asynchronous::detail::has_is_continuation_task<typename std::decay<argtype>::type>::value
#define BOOST_ASYNC_KMP_IMPL_CHECK_MOVE(argtype) !boost::asynchronous::detail::has_is_continuation_task<typename std::decay<argtype>::type>::value
#define BOOST_ASYNC_KMP_IMPL_CHECK_CONT(argtype) boost::asynchronous::detail::has_is_continuation_task<typename std::decay<argtype>::type>::value

// BOOST_ASYNC_KMP_IMPL_ARGS_...: Unwraps the prefix and type into function argument code
#define BOOST_ASYNC_KMP_IMPL_ARGS_ITER(prefix, argtype) argtype prefix##begin, argtype prefix##end
#define BOOST_ASYNC_KMP_IMPL_ARGS_RNGE(prefix, argtype) const argtype& prefix##range
#define BOOST_ASYNC_KMP_IMPL_ARGS_MOVE(prefix, argtype) argtype&& prefix##range
#define BOOST_ASYNC_KMP_IMPL_ARGS_CONT(prefix, argtype) argtype prefix##continuation

// BOOST_ASYNC_KMP_IMPL_OTHER_ARGS: The remaining arguments that are always the same
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
#define BOOST_ASYNC_KMP_IMPL_OTHER_ARGS Functor functor, long cutoff, const std::string& task_name, std::size_t prio
#else
#define BOOST_ASYNC_KMP_IMPL_OTHER_ARGS Functor functor, long cutoff, const std::string& task_name = "", std::size_t prio = 0
#endif

// BOOST_ASYNC_KMP_IMPL_UNWRAP_...: Adapts the function to the specified argument modes
// The iterator version does not need to do anything (we want iterators for the baseline implementation)
// The range (reference) version just uses begin and end to get the iterators we need
// If the range is moved, we keep it alive while the inner code runs
// For continuations, we need to use boost::asynchronous::then on the input
// The _INIT variants are run at the start of the function.
#define BOOST_ASYNC_KMP_IMPL_UNWRAP_INIT_ITER(prefix)
#define BOOST_ASYNC_KMP_IMPL_UNWRAP_INIT_RNGE(prefix) \
    using std::begin;                                 \
    using std::end;                                   \
    auto prefix##begin = begin(prefix##range);        \
    auto prefix##end   = end(prefix##range);
#define BOOST_ASYNC_KMP_IMPL_UNWRAP_INIT_MOVE(prefix)                               \
    using prefix##type = typename std::decay<decltype(prefix##range)>::type;        \
    auto prefix##shared = std::make_shared<prefix##type>(std::move(prefix##range)); \
    using std::begin;                                                               \
    using std::end;                                                                 \
    auto prefix##begin = begin(*prefix##shared);                                    \
    auto prefix##end   = end(*prefix##shared);
#define BOOST_ASYNC_KMP_IMPL_UNWRAP_INIT_CONT(prefix) \
    using prefix##result = typename std::decay<decltype(prefix##continuation)>::type::return_type;

#define BOOST_ASYNC_KMP_IMPL_UNWRAP_ITER(prefix, args, next) next
#define BOOST_ASYNC_KMP_IMPL_UNWRAP_RNGE(prefix, args, next) next
#define BOOST_ASYNC_KMP_IMPL_UNWRAP_MOVE(prefix, args, next)                                          \
    boost::asynchronous::then_job<Job>(                                                               \
        next,                                                                                         \
        [prefix##shared](boost::asynchronous::expected<Functor> expected) { return expected.get(); }, \
        task_name + ": boost::asynchronous::parallel_kmp: keep-alive of moved range"                  \
    )
#define BOOST_ASYNC_KMP_IMPL_UNWRAP_CONT(prefix, args, next)                                                  \
    boost::asynchronous::then_job<Job>(                                                                       \
        std::move(prefix##continuation),                                                                      \
        [args](boost::asynchronous::expected<prefix##result> expected) mutable                                \
        {                                                                                                     \
            auto prefix##shared = std::make_shared<prefix##result>(std::move(expected.get()));                \
            using std::begin;                                                                                 \
            using std::end;                                                                                   \
            auto prefix##begin = begin(*prefix##shared);                                                      \
            auto prefix##end   = end(*prefix##shared);                                                        \
            return boost::asynchronous::then_job<Job>(                                                        \
                next,                                                                                         \
                [prefix##shared](boost::asynchronous::expected<Functor> expected) { return expected.get(); }, \
                task_name + ": boost::asynchronous::parallel_kmp: keep-alive of moved range"                  \
            );                                                                                                \
        },                                                                                                    \
        task_name + ": boost::asynchronous::parallel_kmp: argument callback"                                  \
    )

// BOOST_ASYNC_KMP_IMPL_CAPTURE_...: Things we need to capture until the callback from a continuation argument comes, and how we capture them.
// Because these happen after BOOST_ASYNC_KMP_IMPL_UNWRAP_INIT_..., we capture iterators for all but the continuation versions.
// Because we do not want to keep the continuation around on which we call boost::asynchronous::then, there are three versions of this:
// BEFORE is used when the capture happens before the relevant UNWRAP (captures continuations etc.), WHILE is used during the relevant UNWRAP
// (does not capture continuations nor the resulting iterators), and AFTER is used after the UNWRAP (captures iterators resulting from
// continuation arguments).
#define BOOST_ASYNC_KMP_IMPL_CAPTURE_BEFORE_ITER(prefix) prefix##begin, prefix##end,
#define BOOST_ASYNC_KMP_IMPL_CAPTURE_BEFORE_RNGE(prefix) prefix##begin, prefix##end,
#define BOOST_ASYNC_KMP_IMPL_CAPTURE_BEFORE_MOVE(prefix) prefix##begin, prefix##end, prefix##shared,
#define BOOST_ASYNC_KMP_IMPL_CAPTURE_BEFORE_CONT(prefix) prefix##continuation,
#define BOOST_ASYNC_KMP_IMPL_CAPTURE_WHILE_ITER(prefix) prefix##begin, prefix##end,
#define BOOST_ASYNC_KMP_IMPL_CAPTURE_WHILE_RNGE(prefix) prefix##begin, prefix##end,
#define BOOST_ASYNC_KMP_IMPL_CAPTURE_WHILE_MOVE(prefix) prefix##begin, prefix##end, prefix##shared,
#define BOOST_ASYNC_KMP_IMPL_CAPTURE_WHILE_CONT(prefix)
#define BOOST_ASYNC_KMP_IMPL_CAPTURE_AFTER_ITER(prefix) prefix##begin, prefix##end,
#define BOOST_ASYNC_KMP_IMPL_CAPTURE_AFTER_RNGE(prefix) prefix##begin, prefix##end,
#define BOOST_ASYNC_KMP_IMPL_CAPTURE_AFTER_MOVE(prefix) prefix##begin, prefix##end, prefix##shared,
#define BOOST_ASYNC_KMP_IMPL_CAPTURE_AFTER_CONT(prefix) prefix##begin, prefix##end, prefix##shared,

// BOOST_ASYNC_KMP_IMPL_OTHER_CAPTURES: The remaining captures, independent of mode
#ifdef __cpp_init_captures
#define BOOST_ASYNC_KMP_IMPL_OTHER_CAPTURES functor = std::move(functor), cutoff, task_name, prio
#else
#define BOOST_ASYNC_KMP_IMPL_OTHER_CAPTURES functor, cutoff, task_name, prio
#endif

// BOOST_ASYNC_KMP_IMPL_CAPTURES: Lists all the captures
#define BOOST_ASYNC_KMP_IMPL_CAPTURES(h_modif, h_capture_mode, n_modif, n_capture_mode) \
    BOOST_ASYNC_KMP_IMPL_CAPTURE_##h_capture_mode##_##h_modif(h_)                       \
    BOOST_ASYNC_KMP_IMPL_CAPTURE_##n_capture_mode##_##n_modif(n_)                       \
    BOOST_ASYNC_KMP_IMPL_OTHER_CAPTURES

// BOOST_ASYNC_KMP_IMPL_DELEGATE: The function we ultimately want to call
#define BOOST_ASYNC_KMP_IMPL_DELEGATE boost::asynchronous::detail::parallel_kmp_impl<Job>(h_begin, h_end, n_begin, n_end, std::move(functor), cutoff, task_name, prio)

// BOOST_ASYNC_KMP_IMPL: Build a parallel_kmp function for the specified argument modes
#define BOOST_ASYNC_KMP_IMPL(h_modif, n_modif)                                                                                                                         \
    template <                                                                                                                                                         \
        typename Haystack,                                                                                                                                             \
        typename Needle,                                                                                                                                               \
        typename Functor,                                                                                                                                              \
        typename Job = typename boost::asynchronous::detail::job_type_from<Haystack, Needle>::type                                                                     \
    >                                                                                                                                                                  \
    typename std::enable_if<                                                                                                                                           \
        boost::asynchronous::detail::is_kmp_functor<Functor>::value && BOOST_ASYNC_KMP_IMPL_CHECK_##h_modif(Haystack) && BOOST_ASYNC_KMP_IMPL_CHECK_##n_modif(Needle), \
        boost::asynchronous::detail::callback_continuation<Functor, Job>                                                                                               \
    >::type                                                                                                                                                            \
    parallel_kmp(                                                                                                                                                      \
        BOOST_ASYNC_KMP_IMPL_ARGS_##h_modif(h_, Haystack),                                                                                                             \
        BOOST_ASYNC_KMP_IMPL_ARGS_##n_modif(n_, Needle),                                                                                                               \
        BOOST_ASYNC_KMP_IMPL_OTHER_ARGS                                                                                                                                \
    )                                                                                                                                                                  \
    {                                                                                                                                                                  \
        BOOST_ASYNC_KMP_IMPL_UNWRAP_INIT_##h_modif(h_)                                                                                                                 \
        BOOST_ASYNC_KMP_IMPL_UNWRAP_INIT_##n_modif(n_)                                                                                                                 \
        return BOOST_ASYNC_KMP_IMPL_UNWRAP_##h_modif(                                                                                                                  \
            h_,                                                                                                                                                        \
            BOOST_ASYNC_KMP_IMPL_CAPTURES(h_modif, WHILE, n_modif, BEFORE),                                                                                            \
            BOOST_ASYNC_KMP_IMPL_UNWRAP_##n_modif(                                                                                                                     \
                n_,                                                                                                                                                    \
                BOOST_ASYNC_KMP_IMPL_CAPTURES(h_modif, AFTER, n_modif, WHILE),                                                                                         \
                BOOST_ASYNC_KMP_IMPL_DELEGATE                                                                                                                          \
            )                                                                                                                                                          \
        );                                                                                                                                                             \
    }

// These versions allow taking any combination of iterators, ranges (by reference or moved), and continuations.
// Always called as parallel_kmp(haystack, needle, functor, ...), but 'haystack' and 'needle' can also be two iterators
// instead of a single argument.
// The functor acts like the functor for parallel_for_each, it should implement 'void operator()(const index_type&)'
// (where index_type is an integral type usable for indexing into the haystack), and 'void merge(const functor_type&)'
// or 'void merge(functor_type&&)', which is called on the first of the two functors when two chunks of work are finished.
// The continuation returns the functor type that was passed in.

BOOST_ASYNC_KMP_IMPL(ITER, ITER)
BOOST_ASYNC_KMP_IMPL(ITER, RNGE)
BOOST_ASYNC_KMP_IMPL(ITER, MOVE)
BOOST_ASYNC_KMP_IMPL(ITER, CONT)

BOOST_ASYNC_KMP_IMPL(RNGE, ITER)
BOOST_ASYNC_KMP_IMPL(RNGE, RNGE)
BOOST_ASYNC_KMP_IMPL(RNGE, MOVE)
BOOST_ASYNC_KMP_IMPL(RNGE, CONT)

BOOST_ASYNC_KMP_IMPL(MOVE, ITER)
BOOST_ASYNC_KMP_IMPL(MOVE, RNGE)
BOOST_ASYNC_KMP_IMPL(MOVE, MOVE)
BOOST_ASYNC_KMP_IMPL(MOVE, CONT)

BOOST_ASYNC_KMP_IMPL(CONT, ITER)
BOOST_ASYNC_KMP_IMPL(CONT, RNGE)
BOOST_ASYNC_KMP_IMPL(CONT, MOVE)
BOOST_ASYNC_KMP_IMPL(CONT, CONT)

// Undefine anything that might cause trouble
#undef BOOST_ASYNC_KMP_IMPL
#undef BOOST_ASYNC_KMP_IMPL_DELEGATE
#undef BOOST_ASYNC_KMP_IMPL_CAPTURES
#undef BOOST_ASYNC_KMP_IMPL_OTHER_CAPTURES
#undef BOOST_ASYNC_KMP_IMPL_OTHER_CAPTURES
#undef BOOST_ASYNC_KMP_IMPL_CAPTURE_AFTER_CONT
#undef BOOST_ASYNC_KMP_IMPL_CAPTURE_AFTER_MOVE
#undef BOOST_ASYNC_KMP_IMPL_CAPTURE_AFTER_RNGE
#undef BOOST_ASYNC_KMP_IMPL_CAPTURE_AFTER_ITER
#undef BOOST_ASYNC_KMP_IMPL_CAPTURE_WHILE_CONT
#undef BOOST_ASYNC_KMP_IMPL_CAPTURE_WHILE_MOVE
#undef BOOST_ASYNC_KMP_IMPL_CAPTURE_WHILE_RNGE
#undef BOOST_ASYNC_KMP_IMPL_CAPTURE_WHILE_ITER
#undef BOOST_ASYNC_KMP_IMPL_CAPTURE_BEFORE_CONT
#undef BOOST_ASYNC_KMP_IMPL_CAPTURE_BEFORE_MOVE
#undef BOOST_ASYNC_KMP_IMPL_CAPTURE_BEFORE_RNGE
#undef BOOST_ASYNC_KMP_IMPL_CAPTURE_BEFORE_ITER
#undef BOOST_ASYNC_KMP_IMPL_UNWRAP_CONT
#undef BOOST_ASYNC_KMP_IMPL_UNWRAP_MOVE
#undef BOOST_ASYNC_KMP_IMPL_UNWRAP_RNGE
#undef BOOST_ASYNC_KMP_IMPL_UNWRAP_ITER
#undef BOOST_ASYNC_KMP_IMPL_UNWRAP_INIT_CONT
#undef BOOST_ASYNC_KMP_IMPL_UNWRAP_INIT_MOVE
#undef BOOST_ASYNC_KMP_IMPL_UNWRAP_INIT_RNGE
#undef BOOST_ASYNC_KMP_IMPL_UNWRAP_INIT_ITER
#undef BOOST_ASYNC_KMP_IMPL_OTHER_ARGS
#undef BOOST_ASYNC_KMP_IMPL_OTHER_ARGS
#undef BOOST_ASYNC_KMP_IMPL_ARGS_CONT
#undef BOOST_ASYNC_KMP_IMPL_ARGS_MOVE
#undef BOOST_ASYNC_KMP_IMPL_ARGS_RNGE
#undef BOOST_ASYNC_KMP_IMPL_ARGS_ITER
#undef BOOST_ASYNC_KMP_IMPL_CHECK_CONT
#undef BOOST_ASYNC_KMP_IMPL_CHECK_MOVE
#undef BOOST_ASYNC_KMP_IMPL_CHECK_RNGE
#undef BOOST_ASYNC_KMP_IMPL_CHECK_ITER

} // namespace asynchronous
} // namespace boost

#endif // BOOST_ASYNCHRONOUS_PARALLEL_KMP_HPP
