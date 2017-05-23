// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2013
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org
#ifndef BOOST_ASYNCHRONOUS_PARALLEL_SORT_HELPER_HPP
#define BOOST_ASYNCHRONOUS_PARALLEL_SORT_HELPER_HPP

#include <algorithm>
#ifdef BOOST_ASYNCHRONOUS_USE_BOOST_SPREADSORT
#include <boost/sort/spreadsort/spreadsort.hpp>
#endif
#ifdef BOOST_ASYNCHRONOUS_USE_BOOST_SORT
#include <boost/sort/parallel/sort.hpp>
#endif
namespace boost { namespace asynchronous
{

#ifdef BOOST_ASYNCHRONOUS_USE_BOOST_SPREADSORT
struct boost_spreadsort
{
    template <class Iterator, class Func>
    void operator()(Iterator beg, Iterator end, Func&)
    {
        boost::sort::spreadsort::spreadsort(beg,end);
    }
};
#endif
#ifdef BOOST_ASYNCHRONOUS_USE_BOOST_SORT
struct boost_intro_sort
{
    template <class Iterator, class Func>
    void operator()(Iterator beg, Iterator end, Func& f)
    {
        boost::sort::parallel::detail::intro_sort(beg,end,f);
    }
};
struct boost_indirect_sort
{
    template <class Iterator, class Func>
    void operator()(Iterator beg, Iterator end, Func& f)
    {
        boost::sort::parallel::indirect_sort(beg,end,f);
    }
};
struct boost_stable_sort
{
    template <class Iterator, class Func>
    void operator()(Iterator beg, Iterator end, Func& f)
    {
        boost::sort::parallel::stable_sort(beg,end,f);
    }
};
struct boost_spin_sort
{
    template <class Iterator, class Func>
    void operator()(Iterator beg, Iterator end, Func& f)
    {
        boost::sort::parallel::detail::spin_sort<Iterator,Func>(beg,end,f);
    }
};
#endif
struct std_sort
{
    template <class Iterator, class Func>
    void operator()(Iterator beg, Iterator end, Func& f)
    {
        std::sort(beg,end,f);
    }
};
struct std_stable_sort
{
    template <class Iterator, class Func>
    void operator()(Iterator beg, Iterator end, Func& f)
    {
        std::stable_sort(beg,end,f);
    }
};

// version for moved ranges => will return the range as continuation
namespace detail
{
template <class Continuation, class Range1>
struct parallel_sort_range_move_helper : public boost::asynchronous::continuation_task<Range1>
{
    parallel_sort_range_move_helper(Continuation const& c,std::shared_ptr<Range1> range_in,
                                    const std::string& task_name)
        :boost::asynchronous::continuation_task<Range1>(task_name)
        ,cont_(c),range_in_(std::move(range_in))
    {}
    void operator()()
    {
        boost::asynchronous::continuation_result<Range1> task_res = this->this_task_result();
        auto range_in = range_in_;
        cont_.on_done([task_res,range_in](std::tuple<boost::asynchronous::expected<void> >&& continuation_res)
        {
            try
            {
                // get to check that no exception
                std::get<0>(continuation_res).get();
                task_res.set_value(std::move(*range_in));
            }
            catch(std::exception& e)
            {
                task_res.set_exception(std::make_exception_ptr(e));
            }
        }
        );
    }
    Continuation cont_;
    std::shared_ptr<Range1> range_in_;
};
}

}}

#endif // BOOST_ASYNCHRONOUS_PARALLEL_SORT_HELPER_HPP
