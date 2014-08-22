// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2013
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef DUMMY_PARALLEL_FIND_ALL_TASK_HPP
#define DUMMY_PARALLEL_FIND_ALL_TASK_HPP

#include <boost/asynchronous/scheduler/serializable_task.hpp>
#include <boost/asynchronous/algorithm/parallel_find_all.hpp>
#include <boost/asynchronous/any_serializable.hpp>

struct dummy_parallel_find_all_subtask : public boost::asynchronous::serializable_task
{
    dummy_parallel_find_all_subtask(int min_=0,int max_=0)
        :boost::asynchronous::serializable_task("dummy_parallel_find_all_subtask")
        ,m_min(min_),m_max(max_){}
    template <class Archive>
    void serialize(Archive & ar, const unsigned int /*version*/)
    {
        ar & m_min;
        ar & m_max;
    }
    bool operator()(int i)const
    {
        return (m_min <= i) && (i < m_max);
    }

    int m_min;
    int m_max;
};
std::vector<int> mkdata() {
    std::vector<int> result;
    for (int i = 0; i < 50000; ++i) {
        result.push_back(i);
    }
    return result;
}
struct dummy_parallel_find_all_task : public boost::asynchronous::serializable_task
{
    dummy_parallel_find_all_task():boost::asynchronous::serializable_task("dummy_parallel_find_all_task"),m_data(mkdata()){}
    template <class Archive>
    void serialize(Archive & ar, const unsigned int /*version*/)
    {
        ar & m_data;
    }
    auto operator()() -> decltype(boost::asynchronous::parallel_find_all<std::vector<int>,dummy_parallel_find_all_subtask,std::vector<int>,boost::asynchronous::any_serializable>(
                                      std::move(std::vector<int>()),
                                      dummy_parallel_find_all_subtask(400,600),
                                      1000))
    {
        return boost::asynchronous::parallel_find_all
                <std::vector<int>,dummy_parallel_find_all_subtask,std::vector<int>,boost::asynchronous::any_serializable>(
            std::move(m_data),
            dummy_parallel_find_all_subtask(400,600),
            10);
    }

    std::vector<int> m_data;
};
#endif // DUMMY_PARALLEL_FIND_ALL_TASK_HPP

