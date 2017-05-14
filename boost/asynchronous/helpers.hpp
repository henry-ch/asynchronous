// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2016
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNCHRONOUS_HELPERS_HPP
#define BOOST_ASYNCHRONOUS_HELPERS_HPP

#include <vector>
#include <tuple>
#include <numeric>
#include <chrono>
#include <boost/asynchronous/post.hpp>

// provides a few helpers.
// The most useful is find_best_cutoff (example in example_cutoff_sort.cpp)
// Finding the best cutoff for an algorithm and a given type on a given machine is dark art.
// find_best_cutoff will do for you a kind of deployment optimization.
// Call find_best_cutoff providing a lambda calling the algorithm, a first and last cutoff value forming the range of cutoffs to try out,
// steps between 2 cutoffs and the number of retries (the higher the better mean).
// It will return a std::tuple<std::size_t,std::vector<std::size_t>>.
// [0] : the best cutoff found
// [1] : the average execution times of this cutoff, for statistics.

namespace boost { namespace asynchronous
{
// calls f(cutoff) on the given scheduler and measures elapsed time
// returns elapsed time in us
template <class Func, class Scheduler>
std::size_t measure_cutoff(Scheduler s, Func f,std::size_t cutoff,const std::string& task_name, std::size_t prio)
{
    typename std::chrono::high_resolution_clock::time_point start;
    typename std::chrono::high_resolution_clock::time_point stop;
    start = std::chrono::high_resolution_clock::now();
    auto fu = boost::asynchronous::post_future(s,[cutoff,&f]()mutable{return f(cutoff);},task_name,prio);
    fu.get();
    stop = std::chrono::high_resolution_clock::now();
    return (std::chrono::nanoseconds(stop - start).count() / 1000);
}

// uses measure_cutoff to build statistics to find best cutoff
// executes a functor f of the form void(std::size_t cutoff) calling a user provided algorithm
// on the scheduler s with cutoff values [cutoff_begin, cutoff_end)
// in evenly divided steps, retries number of times (to get a better average value)
// ifdef BOOST_ASYNCHRONOUS_USE_COUT, prints intermediate results
template <class Func, class Scheduler>
std::tuple<std::size_t,std::vector<std::size_t>> find_best_cutoff(Scheduler s, Func f,
                                                     std::size_t cutoff_begin,
                                                     std::size_t cutoff_end,
                                                     std::size_t steps,
                                                     std::size_t retries,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                                                     const std::string& task_name,
                                                     std::size_t prio
#else
                                                     const std::string& task_name="",
                                                     std::size_t prio=0
#endif
                                                     )
{
    std::map<std::size_t, std::vector<std::size_t>> map_cutoff_to_elapsed;
    for (std::size_t i = 0; i< steps; ++i)
    {
        map_cutoff_to_elapsed[cutoff_begin+((cutoff_end-cutoff_begin)/steps)].reserve(retries);
        for (std::size_t j = 0; j< retries; ++j)
        {
            std::size_t cutoff = cutoff_begin+((double)(cutoff_end-cutoff_begin)/steps)*i;
            std::size_t one_step = measure_cutoff(s, f, cutoff,task_name,prio);
            map_cutoff_to_elapsed[cutoff].push_back(one_step);
#ifdef BOOST_ASYNCHRONOUS_USE_COUT
            std::cout << "algorithm took for cutoff "<< cutoff << " in us:" << one_step << std::endl;
#endif
        }
    }
    // look for best
    std::size_t best_cutoff=0;
    std::size_t best = std::numeric_limits<std::size_t>::max();
    for (auto it = map_cutoff_to_elapsed.begin(); it != map_cutoff_to_elapsed.end();++it)
    {
        std::size_t acc = std::accumulate((*it).second.begin(),(*it).second.end(),0,[](std::size_t a, std::size_t b){return a+b;});
        if (acc < best)
        {
            best_cutoff = (*it).first;
            best = acc;
        }
    }
    return std::make_tuple(best_cutoff,std::move(map_cutoff_to_elapsed[best_cutoff]));
}

template <class T>
struct force_move_t
{
    force_move_t(T data): m_data(std::move(data)){}
    force_move_t(force_move_t&& rhs)noexcept
        : m_data(std::move(rhs.m_data)){}
    force_move_t(force_move_t const& rhs)noexcept
        : m_data(std::move(const_cast<force_move_t&>(rhs).m_data)){}

    force_move_t& operator= (force_move_t&& rhs) noexcept
    {
        std::swap(m_data,rhs.m_data);
        return *this;
    }
    force_move_t& operator= (force_move_t const& rhs) noexcept
    {
        std::swap(m_data,const_cast<force_move_t&>(rhs).m_data);
        return *this;
    }

    operator T() {return std::move(m_data);}
private:
    T m_data;
};

template <class T>
force_move_t<T> force_move(T t)
{
    return force_move_t<T>(std::move(t));
}
}}
#endif // BOOST_ASYNCHRONOUS_HELPERS_HPP
