#ifndef BOOST_ASYNCHRONOUS_JOB_TYPE_FROM_HPP
#define BOOST_ASYNCHRONOUS_JOB_TYPE_FROM_HPP

#include <boost/asynchronous/callable_any.hpp>

namespace boost
{
namespace asynchronous
{
namespace detail
{

// Attempts to deduce the job type from a list of types that might be continuations (it stops at the first).
// If no continuation is found, we use BOOST_ASYNCHRONOUS_DEFAULT_JOB.

// The unspecialized version does nothing, because we specialize for both continuation and non-continuation.
template <typename... Ts> struct job_type_from;

// If no continuations are left, use the default job type.
template <> struct job_type_from<> { using type = BOOST_ASYNCHRONOUS_DEFAULT_JOB; };

// Otherwise, check if the first type is a continuation.
template <typename T, typename... Ts>
struct job_type_from<T, Ts...>
{
private:
    template <typename U, typename Job = typename U::job_type> static Job job_detector(const U&); // Returns the job type of U. Version is SFINAE'd away if U is not a continuation
    static typename job_type_from<Ts...>::type                            job_detector(...);      // Returns the deduced return type of the remaining types.

public:
    using type = decltype(job_detector(std::declval<T>())); // Will try to match the continuation version, but fall back to the recursive detection if T is not a continuation.
};

} // namespace detail
} // namespace asynchronous
} // namespace boost

#endif // BOOST_ASYNCHRONOUS_JOB_TYPE_FROM_HPP
