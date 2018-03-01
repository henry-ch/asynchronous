// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2018
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNCHRONOUS_PARALLEL_UNIQUE_HPP
#define BOOST_ASYNCHRONOUS_PARALLEL_UNIQUE_HPP

#include <algorithm>
#include <utility>
#include <type_traits>
#include <iterator>


#include <type_traits>
#include <boost/serialization/vector.hpp>

#include <boost/asynchronous/detail/any_interruptible.hpp>
#include <boost/asynchronous/callable_any.hpp>
#include <boost/asynchronous/detail/continuation_impl.hpp>
#include <boost/asynchronous/continuation_task.hpp>
#include <boost/asynchronous/post.hpp>
#include <boost/asynchronous/scheduler/serializable_task.hpp>
#include <boost/asynchronous/algorithm/detail/safe_advance.hpp>
#include <boost/asynchronous/detail/metafunctions.hpp>
#include <boost/asynchronous/algorithm/then.hpp>

#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/range/adaptors.hpp>
#include <boost/range/algorithm.hpp>

namespace boost { namespace asynchronous
{
namespace detail
{
// version for iterators
template <class Iterator, class Func,class Job>
struct parallel_unique_helper: public boost::asynchronous::continuation_task<Iterator>
{
    parallel_unique_helper(Iterator beg, Iterator end,Func func,long cutoff,
                        const std::string& task_name, std::size_t prio)
        : boost::asynchronous::continuation_task<Iterator>(task_name)
        , beg_(beg),end_(end),func_(std::move(func)),cutoff_(cutoff),prio_(prio)
    {}
    void operator()()
    {
        boost::asynchronous::continuation_result<Iterator> task_res = this->this_task_result();
        // advance up to cutoff
        Iterator it = boost::asynchronous::detail::find_cutoff(beg_,cutoff_,end_);
        // if not at end, recurse, otherwise execute here
        if (it == end_)
        {
            task_res.set_value(std::unique(beg_,end_,func_));
        }
        else
        {
            auto func = func_;
            // remember our current iterators, as they will be needed for moving from within continuation's callback
            auto end=end_;
            auto beg=beg_;
            boost::asynchronous::create_callback_continuation_job<Job>(
                        // called when subtasks are done, set our result
                        [task_res,beg,end,it,func](std::tuple<boost::asynchronous::expected<Iterator>,
                                              boost::asynchronous::expected<Iterator>> res) mutable
                        {
                            try
                            {
                                // new end from each range
                                Iterator itend1 = std::get<0>(res).get();
                                Iterator itend2 = std::get<1>(res).get();

                                // last of first range
                                Iterator last1 = beg;
                                std::advance(last1,std::distance(beg,itend1)-1);
                                // remember valid size of total range
                                auto size = std::distance(beg,itend1) + std::distance(it,itend2);
                                // it is beginning of 2nd range
                                if (func(*last1,*it))
                                {
                                    // handle overlap and move range2's begin+1 to range1's end
                                    std::move(++it,itend2,itend1);
                                    --size;
                                }
                                // if there were unique's in range1
                                else if (it != itend1)
                                {
                                    // move range2's begin to range1's end
                                    std::move(it,itend2,itend1);
                                }
                                // done
                                auto newend = beg;
                                std::advance(newend,size);
                                task_res.set_value(newend);
                            }
                            catch(...)
                            {
                                task_res.set_exception(std::current_exception());
                            }
                        },
                        // recursive tasks
                        parallel_unique_helper<Iterator,Func,Job>(beg_,it,func_,cutoff_,this->get_name(),prio_),
                        parallel_unique_helper<Iterator,Func,Job>(it,end_,func_,cutoff_,this->get_name(),prio_)
            );
        }
    }
    Iterator beg_;
    Iterator end_;
    Func func_;
    long cutoff_;
    std::size_t prio_;
};
}
// version for iterators
template <class Iterator, class Func,
          class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
boost::asynchronous::detail::callback_continuation<Iterator,Job>
parallel_unique(Iterator beg, Iterator end,Func func,long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
             const std::string& task_name, std::size_t prio=0)
#else
             const std::string& task_name="", std::size_t prio=0)
#endif
{
    return boost::asynchronous::top_level_callback_continuation_job<Iterator,Job>
            (boost::asynchronous::detail::parallel_unique_helper<Iterator,Func,Job>
                (beg,end,func,cutoff,task_name,prio));
}

// version for ranges returned as continuations
template <class Range, class Func, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
typename std::enable_if<boost::asynchronous::detail::has_is_continuation_task<Range>::value,
                        boost::asynchronous::detail::callback_continuation<typename Range::return_type,Job> >::type
parallel_unique(Range range,Func func,long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
             const std::string& task_name, std::size_t prio=0)
#else
             const std::string& task_name="", std::size_t prio=0)
#endif
{

    return boost::asynchronous::then(
                 std::move(range),
                 [func,cutoff,task_name,prio](typename Range::return_type&& r)mutable
                 {
                    auto res = std::make_shared<typename Range::return_type>(std::move(r));
                    return boost::asynchronous::then(
                            boost::asynchronous::parallel_unique
                                  <typename Range::return_type::iterator, Func, Job>
                                       (boost::begin(*res),boost::end(*res),func,cutoff,task_name,prio),
                            [res](typename Range::return_type::iterator it)mutable
                            {
                                res->erase(it, res->end());
                                return *res;
                            }
                           );
                 });
}
}}
#endif // BOOST_ASYNCHRONOUS_PARALLEL_UNIQUE_HPP

