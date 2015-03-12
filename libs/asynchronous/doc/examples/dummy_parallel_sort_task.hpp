// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2013
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef DUMMY_PARALLEL_SORT_TASK_HPP
#define DUMMY_PARALLEL_SORT_TASK_HPP
#include <boost/asynchronous/scheduler/serializable_task.hpp>
#include <boost/asynchronous/algorithm/parallel_sort.hpp>
#include <boost/asynchronous/any_serializable.hpp>

struct dummy_parallel_sort_subtask : public boost::asynchronous::serializable_task
{
    dummy_parallel_sort_subtask():boost::asynchronous::serializable_task("dummy_parallel_sort_subtask"){}
    template <class Archive>
    void serialize(Archive & /*ar*/, const unsigned int /*version*/)
    {
    }
    bool operator()(int i, int j)const
    {
        return i < j;
    }
};

struct dummy_parallel_sort_task : public boost::asynchronous::serializable_task
{
    // empty ctor in case of deserialization
    dummy_parallel_sort_task():boost::asynchronous::serializable_task("dummy_parallel_sort_task"){}
    dummy_parallel_sort_task(std::vector<int> const& v):boost::asynchronous::serializable_task("dummy_parallel_sort_task"),m_data(v)
    {
    }    
    template <class Archive>
    void serialize(Archive & ar, const unsigned int /*version*/)
    {
        ar & m_data;
    }
    auto operator()() -> decltype(boost::asynchronous::parallel_sort<std::vector<int>,dummy_parallel_sort_subtask,boost::asynchronous::any_serializable>(
                                      std::move(std::vector<int>()),
                                      dummy_parallel_sort_subtask(),
                                      1000))
    {
        return boost::asynchronous::parallel_sort
                <std::vector<int>,dummy_parallel_sort_subtask,boost::asynchronous::any_serializable>(
            std::move(m_data),
            dummy_parallel_sort_subtask(),
            10);
    }
    std::vector<int> m_data;
};
#endif // DUMMY_PARALLEL_SORT_TASK_HPP
