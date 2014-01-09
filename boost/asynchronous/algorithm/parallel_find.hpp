// Boost.Asynchronous library
// (C) Christophe Henry, 2013
//
// Use, modification and distribution is subject to the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNCHRON_PARALLEL_FOR_HPP
#define BOOST_ASYNCHRON_PARALLEL_FOR_HPP

#include <algorithm>
#include <utility>

#include <boost/thread/future.hpp>
#include <boost/asynchronous/detail/any_interruptible.hpp>
#include <boost/asynchronous/callable_any.hpp>
#include <boost/asynchronous/detail/continuation_impl.hpp>
#include <boost/asynchronous/continuation_task.hpp>
#include <boost/asynchronous/post.hpp>

namespace boost { namespace asynchronous
{
namespace detail
{
template <class Iterator, class Func, class Job, class Value=Iterator::value_type>
struct parallel_find_helper: public boost::asynchronous::continuation_task<Iterator>
{
    // Boolean value signifies validity of the iterator as a result (find returns the end iterator
    // if nothing is found, but for individual parts "end" is still a valid position for the entire
    // vector (or whatever other item is searched)).
    typedef std::pair<Iterator, bool> IteratorWithFlag;
    
    parallel_find_helper(Iterator beg, Iterator end, Func func, const Value& key, long cutoff,
                         const std::string& task_name, std::size_t prio)
        : beg_(beg),end_(end),func_(std::move(func)),key_(key),cutoff_(cutoff),task_name_(std::move(task_name)),prio_(prio)
    {}
    void operator()() const
    {
        std::vector<boost::future<IteratorWithFlag> > fus;
        boost::asynchronous::any_weak_scheduler<Job> weak_scheduler = boost::asynchronous::get_thread_scheduler<Job>();
        boost::asynchronous::any_shared_scheduler<Job> locked_scheduler = weak_scheduler.lock();
        //TODO return what?
    // if (!locked_scheduler.is_valid())
    // // give up
    // return;
        for (Iterator it = beg_; it != end_ ; )
        {
            Iterator itp = it;
            std::advance(it, cutoff_);
            auto func = func_;
            boost::future<IteratorWithFlag> fu = boost::asynchronous::post_future(locked_scheduler,
                                                                                  [it,func]()
                                                                                  {
                                                                                      Iterator found = std::find(itp,it,func);
                                                                                      return IteratorWithFlag(found, found == it);
                                                                                  },
                                                                                  task_name_, prio_);
            fus.emplace_back(std::move(fu));
        }
        boost::asynchronous::continuation_result<IteratorWithFlag> task_res = this_task_result();
        boost::asynchronous::create_continuation_log<Job>(
                    // called when subtasks are done, set our result
                    [task_res](std::vector<boost::future<IteratorWithFlag>> res)
                    {
                        try
                        {
                            for (std::vector<boost::future<IteratorWithFlag>>::iterator itr = res.begin();itr != res.end();++itr)
                            {
                                // Get value to check that no exception occurred
                                IteratorWithFlag iwf = (*itr).get();
                                // If this is a result
                                if (iwf.second) {
                                    task_res.set_value(iwf.first);
                                    break;
                                }
                            }
                        }
                        catch(std::exception& e)
                        {
                            task_res.set_exception(boost::copy_exception(e));
                        }
                    },
                    // future results of recursive tasks
                    std::move(fus));
    }
    Iterator beg_;
    Iterator end_;
    Func func_;
    Value key_;
    long cutoff_;
    std::string task_name_;
    std::size_t prio_;
};
}
//template <class Iterator, class S1, class S2, class Func, class Callback>
//boost::asynchronous::any_interruptible
//parallel_for(Iterator begin, Iterator end, S1 const& scheduler,Func && func, S2 const& weak_cb_scheduler, Callback&& c,
// const std::string& task_name="", std::size_t post_prio=0, std::size_t cb_prio=0)
//{

//}

//template <class Range, class Func, class Job=boost::asynchronous::any_callable>
//boost::asynchronous::detail::continuation<Range,Job>
//parallel_for(Range& range,Func func,long cutoff,
// TODO ranges
template <class Iterator, class Func, class Job=boost::asynchronous::any_callable, class Value=Iterator::value_type>
boost::asynchronous::detail::continuation<Iterator,Job>
parallel_for(Iterator beg, Iterator end,Func func, Value v, long cutoff,
             const std::string& task_name="", std::size_t prio=0)
{
    return boost::asynchronous::top_level_continuation_log<Iterator,Job>
            (boost::asynchronous::detail::parallel_for_helper<Iterator,Func,Job, Value>(beg,end,func,v,cutoff,task_name,prio));
}

}}
#endif // BOOST_ASYNCHRON_PARALLEL_FOR_HPP

