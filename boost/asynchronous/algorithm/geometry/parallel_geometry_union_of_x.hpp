// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2014
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNCHRONOUS_PARALLEL_GEOMETRY_UNION_HPP
#define BOOST_ASYNCHRONOUS_PARALLEL_GEOMETRY_UNION_HPP

#include <algorithm>

#include <boost/utility/enable_if.hpp>
#include <boost/geometry.hpp>

#include <boost/asynchronous/callable_any.hpp>
#include <boost/asynchronous/continuation_task.hpp>
#include <boost/asynchronous/post.hpp>
#include <boost/asynchronous/algorithm/detail/safe_advance.hpp>
#include <boost/asynchronous/detail/metafunctions.hpp>

namespace boost { namespace asynchronous
{
namespace detail
{
template <class Iterator,class Range>
Range pairwise_union(Iterator beg, Iterator end)
{
    auto it1 = beg;
    auto it2 = beg;
    boost::asynchronous::detail::safe_advance(it2,1,end);
    typedef typename boost::range_value<Range>::type Element;
    Range output_collection;
    while (it2 != end)
    {
        Element one_union;
        boost::geometry::union_(*it1,*it2,one_union);
        boost::asynchronous::detail::safe_advance(it1,2,end);
        boost::asynchronous::detail::safe_advance(it2,2,end);
        output_collection.push_back(std::move(one_union));
    }
    if ( it1 != end)
    {
        output_collection.push_back(std::move(*it1));
    }
    return std::move(output_collection);
}

template <class Iterator,class Range,class Job>
struct parallel_geometry_union_of_x_helper: public boost::asynchronous::continuation_task<Range>
{
    parallel_geometry_union_of_x_helper(Iterator beg, Iterator end,
                            long cutoff, const std::string& task_name, std::size_t prio)
        : boost::asynchronous::continuation_task<Range>(task_name),
          beg_(beg),end_(end),cutoff_(cutoff),prio_(prio)
    {}    

    void operator()()
    {
        boost::asynchronous::continuation_result<Range> task_res = this->this_task_result();
        // advance up to cutoff
        Iterator it = boost::asynchronous::detail::find_cutoff(beg_,cutoff_,end_);
        if (it == end_)
        {
            Range temp = std::move(boost::asynchronous::detail::pairwise_union<Iterator,Range>(beg_,end_));
            while(temp.size() > 1)
            {
                temp = std::move(boost::asynchronous::detail::pairwise_union<Iterator,Range>(temp.begin(),temp.end()));
            }
            task_res.set_value(std::move(temp));
        }
        else
        {
            boost::asynchronous::create_callback_continuation_job<Job>(
                        // called when subtasks are done, set our result
                        [task_res](std::tuple<boost::asynchronous::expected<Range>,boost::asynchronous::expected<Range> > res)
                        {
                            try
                            {
                                // make a new collection containing both sub-collections
                                Range output_collection;
                                auto sub1 = std::move(std::get<0>(res).get());
                                auto sub2 = std::move(std::get<1>(res).get());
                                // TODO reserve if vector
                                auto it1 = sub1.begin();
                                auto it2 = sub2.begin();
                                auto end1 = sub1.end();
                                auto end2 = sub2.end();
                                // merge elements in turn to union one from each container next
                                while (it1 != end1 && it2 != end2)
                                {
                                    output_collection.push_back(std::move(*it1));
                                    output_collection.push_back(std::move(*it2));
                                    ++it1;++it2;
                                }
                                if (it1 != end1)
                                {
                                    output_collection.push_back(std::move(*it1));
                                }
                                if (it2 != end2)
                                {
                                    output_collection.push_back(std::move(*it2));
                                }
                                Range res = std::move(boost::asynchronous::detail::pairwise_union<Iterator,Range>(
                                                          output_collection.begin(),output_collection.end()));
                                while(res.size() > 1)
                                {
                                    res = std::move(boost::asynchronous::detail::pairwise_union<Iterator,Range>(res.begin(),res.end()));
                                }
                                task_res.set_value(std::move(res));
                            }
                            catch(std::exception& e)
                            {
                                task_res.set_exception(boost::copy_exception(e));
                            }
                        },
                        // recursive tasks
                        parallel_geometry_union_of_x_helper<Iterator,Range,Job>
                            (beg_,it,cutoff_,this->get_name(),prio_),
                        parallel_geometry_union_of_x_helper<Iterator,Range,Job>
                            (it,end_,cutoff_,this->get_name(),prio_)
            );
        }
    }
    Iterator beg_;
    Iterator end_;
    long cutoff_;
    std::size_t prio_;
};
}

template <class Iterator, class Range, class Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
boost::asynchronous::detail::callback_continuation<Range,Job>
parallel_geometry_union_of_x(Iterator beg, Iterator end, long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                    const std::string& task_name, std::size_t prio)
#else
                    const std::string& task_name="", std::size_t prio=0)
#endif
{
    return boost::asynchronous::top_level_callback_continuation_job<Range,Job>
            (boost::asynchronous::detail::parallel_geometry_union_of_x_helper<Iterator,Range,Job>
                (beg,end,cutoff,task_name,prio));

}

}}
#endif // BOOST_ASYNCHRONOUS_PARALLEL_GEOMETRY_UNION_HPP
