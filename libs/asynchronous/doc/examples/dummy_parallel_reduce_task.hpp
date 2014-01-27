// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2013
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef DUMMY_PARALLEL_REDUCE_TASK_HPP
#define DUMMY_PARALLEL_REDUCE_TASK_HPP

#include <boost/asynchronous/scheduler/serializable_task.hpp>
#include <boost/asynchronous/algorithm/parallel_reduce.hpp>
#include <boost/asynchronous/any_serializable.hpp>

struct dummy_parallel_reduce_subtask : public boost::asynchronous::serializable_task
{
    dummy_parallel_reduce_subtask():boost::asynchronous::serializable_task("dummy_parallel_reduce_subtask"){}
    template <class Archive>
    void serialize(Archive &, const unsigned int /*version*/)
    {
    }
    long operator()(long const& a, long const& b)const
    {
        return a + b;
    }
};

struct dummy_parallel_reduce_task : public boost::asynchronous::serializable_task
{
    dummy_parallel_reduce_task():boost::asynchronous::serializable_task("dummy_parallel_reduce_task"),m_data()
    {
        for (long i = 1; i <= 100000; ++i) m_data.push_back(i);
    }
    template <class Archive>
    void serialize(Archive & ar, const unsigned int /*version*/)
    {
        ar & m_data;
    }
    auto operator()() -> decltype(boost::asynchronous::parallel_reduce<std::vector<long>,dummy_parallel_reduce_subtask,boost::asynchronous::any_serializable>(
                                      std::move(std::vector<long>()),
                                      dummy_parallel_reduce_subtask(),
                                      10))
    {
        return boost::asynchronous::parallel_reduce
                <std::vector<long>,dummy_parallel_reduce_subtask,boost::asynchronous::any_serializable>(
            std::move(m_data),
            dummy_parallel_reduce_subtask(),
            10);
    }

    std::vector<long> m_data;
};
#endif // DUMMY_PARALLEL_REDUCE_TASK_HPP
