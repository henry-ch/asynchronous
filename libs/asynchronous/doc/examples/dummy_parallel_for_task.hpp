// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2013
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef DUMMY_PARALLEL_FOR_TASK_HPP
#define DUMMY_PARALLEL_FOR_TASK_HPP

#include <boost/asynchronous/scheduler/serializable_task.hpp>
#include <boost/asynchronous/algorithm/parallel_for.hpp>
#include <boost/asynchronous/any_serializable.hpp>

struct dummy_parallel_for_subtask : public boost::asynchronous::serializable_task
{
    dummy_parallel_for_subtask(int d=0):boost::asynchronous::serializable_task("dummy_parallel_for_subtask"),m_data(d){}
    template <class Archive>
    void serialize(Archive & ar, const unsigned int /*version*/)
    {
        ar & m_data;
    }
    void operator()(int const& i)const
    {
        const_cast<int&>(i) += m_data;
    }

    int m_data;
};

struct dummy_parallel_for_task : public boost::asynchronous::serializable_task
{
    dummy_parallel_for_task():boost::asynchronous::serializable_task("dummy_parallel_for_task"),m_data(50000,1){}
    template <class Archive>
    void serialize(Archive & ar, const unsigned int /*version*/)
    {
        ar & m_data;
    }
    auto operator()() -> decltype(boost::asynchronous::parallel_for<std::vector<int>,dummy_parallel_for_subtask,boost::asynchronous::any_serializable>(
                                      std::move(std::vector<int>()),
                                      dummy_parallel_for_subtask(2),
                                      1000))
    {
        return boost::asynchronous::parallel_for
                <std::vector<int>,dummy_parallel_for_subtask,boost::asynchronous::any_serializable>(
            std::move(m_data),
            dummy_parallel_for_subtask(2),
            10);
    }

    std::vector<int> m_data;
};
#endif // DUMMY_PARALLEL_FOR_TASK_HPP
