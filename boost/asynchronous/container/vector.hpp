// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2015
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNCHRONOUS_CONTAINER_VECTOR_HPP
#define BOOST_ASYNCHRONOUS_CONTAINER_VECTOR_HPP

#include <vector>

#include <boost/shared_ptr.hpp>
#include <boost/asynchronous/algorithm/parallel_placement.hpp>
#include <boost/asynchronous/any_shared_scheduler_proxy.hpp>
#include <boost/asynchronous/post.hpp>
#include <boost/mpl/has_xxx.hpp>
BOOST_MPL_HAS_XXX_TRAIT_DEF(asynchronous_container)

namespace boost { namespace asynchronous
{
template<typename T, typename Job, typename Alloc>
class vector;
namespace detail
{

template<typename Range, typename Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
struct make_asynchronous_range_task: public boost::asynchronous::continuation_task<boost::shared_ptr<Range>>
{
    make_asynchronous_range_task(std::size_t n,long cutoff,const std::string& task_name, std::size_t prio)
    : m_size(n),m_cutoff(cutoff),m_task_name(task_name),m_prio(prio)
    {}
    void operator()();
    std::size_t m_size;
    long m_cutoff;
    std::string m_task_name;
    std::size_t m_prio;
};
}

template<typename T, typename Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB, typename Alloc = std::allocator<T> >
class vector
{
public:
    // we want to indicate we are an asynchronous container and support continuation-based construction
    typedef int                         asynchronous_container;
    typedef std::size_t					size_type;
    typedef T*                          iterator;
    typedef const T*                    const_iterator;
    typedef T&                          reference;
    typedef T const&                    const_reference;
    typedef T					        value_type;
    // TODO with allocator
    vector()
    : m_data()
    {}

    vector(long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
           const std::string& task_name, std::size_t prio)
#else
           const std::string& task_name="", std::size_t prio=0)
#endif
        : m_cutoff(cutoff)
        , m_task_name(task_name)
        , m_prio(prio)
    {
    }

    vector(boost::asynchronous::any_shared_scheduler_proxy<Job> scheduler,size_type n,
           long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
           const std::string& task_name, std::size_t prio)
#else
           const std::string& task_name="", std::size_t prio=0)
#endif
        : m_scheduler(scheduler)
        , m_cutoff(cutoff)
        , m_task_name(task_name)
        , m_prio(prio)
    {
        boost::shared_array<char> raw (new char[n * sizeof(T)]);
        auto fu = boost::asynchronous::post_future(scheduler,
        [n,raw,cutoff,task_name,prio]()mutable
        {
            return boost::asynchronous::parallel_placement<T,Job>(0,n,raw,cutoff,task_name+"_placement",prio);
        },
        task_name+"_ctor",prio);
        // if exception, will be forwarded
        fu.get();
        m_data =  boost::make_shared<boost::asynchronous::placement_deleter<T,Job>>(n,raw,cutoff,task_name,prio);
    }
    ~vector()
    {
        // if in threadpool (algorithms) already, nothing to do, placement deleter will handle dtors and freeing of memory
        // else we need to handle destruction
        if (m_scheduler.is_valid())
        {
            // TODO solution for not C++14?
            try
            {
                auto fu = boost::asynchronous::post_future(m_scheduler,
                [data = std::move(m_data)]()mutable
                {
                },
                m_task_name+"_dtor",m_prio);
            }
            catch(std::exception&)
            {}
        }
    }

    iterator begin()
    {
        return (iterator) (m_data->data_.get());
    }

    const_iterator begin() const
    {
        return (const_iterator) (m_data->data_.get());
    }

    iterator end()
    {
        return ((iterator) (m_data->data_.get())) + m_data->size_;
    }

    const_iterator end() const
    {
        return ((const_iterator) (m_data->data_.get())) + m_data->size_;
    }

    reference operator[] (size_type n)
    {
        return *(begin()+n);
    }

    size_type size() const
    {
        return m_data->size_;
    }
private:

    template <class _Range, class _Job>
    friend struct boost::asynchronous::detail::make_asynchronous_range_task;
    // only used from "outside" (i.e. not algorithms)
    boost::asynchronous::any_shared_scheduler_proxy<Job> m_scheduler;
    boost::shared_ptr<boost::asynchronous::placement_deleter<T,Job>> m_data;
    long m_cutoff;
    std::string m_task_name;
    std::size_t m_prio;
};

namespace detail
{
//version asynchronous contaimners
template<typename Range, typename Job>
void make_asynchronous_range_task<Range,Job>::operator()()
{
    boost::asynchronous::continuation_result<boost::shared_ptr<Range>> task_res = this->this_task_result();
    try
    {
        auto v = boost::make_shared<Range>(m_cutoff,m_task_name,m_prio);
        boost::shared_array<char> raw (new char[m_size * sizeof(typename Range::value_type)]);
        auto n = m_size;
        auto cutoff = m_cutoff;
        auto task_name = m_task_name;
        auto prio = m_prio;

        auto cont = boost::asynchronous::parallel_placement<typename Range::value_type,Job>(0,n,raw,cutoff,task_name+"_placement",prio);
        cont.on_done(
        [task_res, raw,v,n,cutoff,task_name,prio]
        (std::tuple<boost::asynchronous::expected<boost::asynchronous::detail::parallel_placement_helper_result> >&& continuation_res) mutable
        {
            try
            {
                auto res = std::get<0>(continuation_res).get();
                if (res.first != boost::asynchronous::detail::parallel_placement_helper_enum::success)
                {
                    task_res.set_exception(res.second);
                }
                else
                {
                    v->m_data = boost::make_shared<boost::asynchronous::placement_deleter<typename Range::value_type,Job>>(n,raw,cutoff,task_name,prio);
                    task_res.set_value(v);
                }
            }
            catch(std::exception& e)
            {
                task_res.set_exception(boost::copy_exception(e));
            }
        });
    }
    catch(std::exception& e)
    {
        task_res.set_exception(boost::copy_exception(e));
    }
}

//version for plain old contaimners
template<typename Range>
struct make_standard_range_task: public boost::asynchronous::continuation_task<boost::shared_ptr<Range>>
{
    make_standard_range_task(std::size_t n)
    : m_size(n)
    {}
    void operator()()
    {
        boost::asynchronous::continuation_result<boost::shared_ptr<Range>> task_res = this->this_task_result();
        task_res.set_value(boost::make_shared<Range>(m_size));
    }

    std::size_t m_size;
};

}

//version asynchronous contaimners
template <typename Range, typename Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
boost::asynchronous::detail::callback_continuation<boost::shared_ptr<Range>,Job>
make_asynchronous_range(std::size_t n,long cutoff,
 #ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
            const std::string& task_name, std::size_t prio, typename boost::enable_if<has_asynchronous_container<Range>>::type* = 0)
 #else
            const std::string& task_name="", std::size_t prio=0, typename boost::enable_if<has_asynchronous_container<Range>>::type* = 0)
 #endif
{
    return boost::asynchronous::top_level_callback_continuation_job<boost::shared_ptr<Range>,Job>
            (boost::asynchronous::detail::make_asynchronous_range_task<Range,Job>
             (n,cutoff,task_name,prio));
}

//version for plain old contaimners
template <typename Range, typename Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB>
boost::asynchronous::detail::callback_continuation<boost::shared_ptr<Range>,Job>
make_asynchronous_range(std::size_t n,long ,
 #ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
            const std::string& , std::size_t , typename boost::disable_if<has_asynchronous_container<Range>>::type* = 0)
 #else
            const std::string& ="", std::size_t =0, typename boost::disable_if<has_asynchronous_container<Range>>::type* = 0)
 #endif
{
    return boost::asynchronous::top_level_callback_continuation_job<boost::shared_ptr<Range>,Job>
            (boost::asynchronous::detail::make_standard_range_task<Range>(n));
}

}} // boost::async



#endif /* BOOST_ASYNCHRONOUS_CONTAINER_VECTOR_HPP */
