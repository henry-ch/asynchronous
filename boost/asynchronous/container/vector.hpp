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

//#include <vector>
#include <limits>

#include <boost/shared_ptr.hpp>
#include <boost/asynchronous/algorithm/parallel_placement.hpp>
#include <boost/asynchronous/algorithm/parallel_move.hpp>
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
// helper for destructors, avoids c++14 move lambda
template <class T>
struct move_only
{
    move_only(T&& data):m_data(std::forward<T>(data)){}
    move_only(move_only&& rhs)noexcept
        : m_data(std::move(rhs.m_data)){}

    move_only& operator= (move_only&& rhs) noexcept
    {
        std::swap(m_data,rhs.m_data);
        return *this;
    }
    move_only(move_only const& rhs)noexcept
        : m_data(std::move(const_cast<move_only&>(rhs).m_data)){}
    move_only& operator= (move_only const& rhs) noexcept
    {
        std::swap(m_data,const_cast<move_only&>(rhs).m_data);
        return *this;
    }
    void operator()()const{} //dummy

private:
    T m_data;

};

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
    typedef T*                          pointer;
    typedef const T*                    const_pointer;
    typedef boost::asynchronous::vector<T,Job,Alloc> this_type;

    enum { default_capacity = 10 };

    // TODO with allocator
    vector()
    : m_data()
    {}

    // only for use inside a threadpool using make_asynchronous_range
    vector(long cutoff,size_type n,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
           const std::string& task_name, std::size_t prio)
#else
           const std::string& task_name="", std::size_t prio=0)
#endif
        : m_cutoff(cutoff)
        , m_task_name(task_name)
        , m_prio(prio)
        , m_size(n)
        , m_capacity(n)
    {
    }

    vector(boost::asynchronous::any_shared_scheduler_proxy<Job> scheduler,long cutoff,
           size_type n,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
           const std::string& task_name, std::size_t prio)
#else
           const std::string& task_name="", std::size_t prio=0)
#endif
        : m_scheduler(scheduler)
        , m_cutoff(cutoff)
        , m_task_name(task_name)
        , m_prio(prio)
        , m_size(n)
        , m_capacity(n)
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

    vector(boost::asynchronous::any_shared_scheduler_proxy<Job> scheduler,long cutoff,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
           const std::string& task_name, std::size_t prio)
#else
           const std::string& task_name="", std::size_t prio=0)
#endif
        : m_scheduler(scheduler)
        , m_cutoff(cutoff)
        , m_task_name(task_name)
        , m_prio(prio)
        , m_size(0)
        , m_capacity(default_capacity)
    {
        boost::shared_array<char> raw (new char[default_capacity * sizeof(T)]);

        m_data =  boost::make_shared<boost::asynchronous::placement_deleter<T,Job>>(0,raw,cutoff,task_name,prio);
    }

    ~vector()
    {
        // if in threadpool (algorithms) already, nothing to do, placement deleter will handle dtors and freeing of memory
        // else we need to handle destruction
        if (m_scheduler.is_valid())
        {
            try
            {
                boost::asynchronous::post_future(m_scheduler,
                                                 boost::asynchronous::detail::move_only<boost::shared_ptr<boost::asynchronous::placement_deleter<T,Job>>>(std::move(m_data)),
                                                 m_task_name+"_dtor",m_prio);
            }
            catch(std::exception&)
            {}
        }
    }

    iterator begin()
    {
        return (iterator) (m_data->data());
    }
    const_iterator begin() const
    {
        return (const_iterator) (m_data->data());
    }
    const_iterator cbegin() const
    {
        return (const_iterator) (m_data->data());
    }

    iterator end()
    {
        return ((iterator) (m_data->data())) + m_size;
    }
    const_iterator end() const
    {
        return ((const_iterator) (m_data->data())) + m_size;
    }
    const_iterator cend() const
    {
        return ((const_iterator) (m_data->data())) + m_size;
    }

    reference operator[] (size_type n)
    {
        return *(begin()+n);
    }
    const_reference operator[] (size_type n) const
    {
        return *(begin()+n);
    }

    reference at (size_type n)
    {
        if (n >= size())
        {
            throw std::out_of_range("boost::asynchronous::vector: at() out of range");
        }
        return *(begin()+n);
    }
    const_reference at (size_type n) const
    {
        if (n >= size())
        {
            throw std::out_of_range("boost::asynchronous::vector: at() out of range");
        }
        return *(begin()+n);
    }

    reference front()
    {
        return *(begin());
    }
    const_reference front() const
    {
        return *(begin());
    }
    reference back()
    {
        return *(begin()+ (m_size - 1) );
    }
    const_reference back() const
    {
        return *(begin()+ (m_size - 1) );
    }
    pointer data()
    {
        return (pointer) (m_data->data());
    }
    const_pointer data() const
    {
        return (const_pointer) (m_data->data());
    }

    size_type capacity() const
    {
        return m_capacity;
    }
    size_type size() const
    {        
        return m_size;
    }
    bool empty() const
    {
        return size() == 0;
    }
    size_type max_size() const
    {
        return std::numeric_limits<size_type>::max();
    }
    void swap( this_type& other )
    {
        std::swap(m_data,other.m_data);
        std::swap(m_size,other.m_size);
        std::swap(m_capacity,other.m_capacity);
    }

    void clear()
    {
        if (m_scheduler.is_valid())
        {
            boost::asynchronous::post_future(m_scheduler,
                                             boost::asynchronous::detail::move_only<boost::shared_ptr<boost::asynchronous::placement_deleter<T,Job>>>(std::move(m_data)),
                                             m_task_name+"_clear",m_prio);
        }
        m_size=0;
    }

    void push_back( const T& value )
    {
        if (m_size + 1 <= m_capacity )
        {
            new (end()) T(value);
            ++m_size;
            ++m_data->size_;            
        }
        else
        {
            // reallocate memory
            auto new_memory = reallocate_helper(calc_new_capacity());
            // add new element, update size and capacity
            m_capacity = new_memory;
            push_back(value);
        }
    }
    void push_back( T&& value )
    {
        if (m_size + 1 <= m_capacity )
        {
            new (end()) T(std::forward<T>(value));
            ++m_size;
            ++m_data->size_;
        }
        else
        {
            // reallocate memory
            auto new_memory = reallocate_helper(calc_new_capacity());
            // add new element, update size and capacity
            m_capacity = new_memory;
            push_back(std::forward<T>(value));
        }
    }
    template< class... Args >
    void emplace_back( Args&&... args )
    {
        if (m_size + 1 <= m_capacity )
        {
            new (end()) T(std::forward<Args...>(args...));
            ++m_size;
            ++m_data->size_;
        }
        else
        {
            // reallocate memory
            auto new_memory = reallocate_helper(calc_new_capacity());
            // add new element, update size and capacity
            m_capacity = new_memory;
            push_back(std::forward<Args...>(args...));
        }
    }
    void pop_back()
    {
        ((T*)(begin()))->~T();
        --m_size;
        --m_data->size_;
    }

    void reserve( size_type new_cap )
    {
        if (new_cap <= m_capacity)
            return;
        // increase capacity, move our elements
        auto new_memory = reallocate_helper(new_cap);
        // add new element, update size and capacity
        m_capacity = new_memory;
    }
    void shrink_to_fit()
    {
        if (m_size == m_capacity)
            // nothing to do
            return;
        // reallocate, move our elements
        auto new_size = reallocate_helper(m_size);
        assert(new_size == m_size);
        assert(new_size == m_data->size_);
        m_capacity = new_size;
    }

private:

    std::size_t calc_new_capacity() const
    {
        return (m_size *2) + 10;
    }

    std::size_t reallocate_helper(size_type new_memory)
    {
        boost::shared_array<char> raw (new char[new_memory * sizeof(T)]);

        // create our current number of objects with placement new
        auto n = m_size;
        auto cutoff = m_cutoff;
        auto task_name = m_task_name;
        auto prio = m_prio;
        auto fu = boost::asynchronous::post_future(m_scheduler,
        [n,raw,cutoff,task_name,prio]()mutable
        {
            return boost::asynchronous::parallel_placement<T,Job>(0,n,raw,cutoff,task_name+"push_back_placement",prio);
        },
        task_name+"_push_back_placement",prio);
        // if exception, will be forwarded
        auto placement_result = fu.get();
        if (placement_result.first != boost::asynchronous::detail::parallel_placement_helper_enum::success)
        {
            boost::rethrow_exception(placement_result.second);
        }

        auto new_data =  boost::make_shared<boost::asynchronous::placement_deleter<T,Job>>(n,std::move(raw),cutoff,task_name,prio);

        auto beg_it = begin();
        auto end_it = end();

        // move our data to new memory
        auto fu2 = boost::asynchronous::post_future(m_scheduler,
        [new_data,beg_it,end_it,cutoff,task_name,prio]()mutable
        {
            return boost::asynchronous::parallel_move<iterator,iterator,Job>(beg_it,end_it,(iterator)(new_data->data()),cutoff,task_name+"push_back_copy",prio);
        },
        task_name+"_push_back_placement",prio);
        // if exception, will be forwarded
        fu2.get();
        // destroy our old data (no need to wait until done)
        boost::asynchronous::post_future(m_scheduler,
                                         boost::asynchronous::detail::move_only<boost::shared_ptr<boost::asynchronous::placement_deleter<T,Job>>>(std::move(m_data)),
                                         m_task_name+"push_back_delete",m_prio);;
        std::swap(m_data,new_data);

        return new_memory;
    }

    template <class _Range, class _Job>
    friend struct boost::asynchronous::detail::make_asynchronous_range_task;
    // only used from "outside" (i.e. not algorithms)
    boost::asynchronous::any_shared_scheduler_proxy<Job> m_scheduler;
    boost::shared_ptr<boost::asynchronous::placement_deleter<T,Job>> m_data;
    long m_cutoff;
    std::string m_task_name;
    std::size_t m_prio;
    std::size_t m_size;
    std::size_t m_capacity;
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
        auto v = boost::make_shared<Range>(m_cutoff,m_size,m_task_name,m_prio);
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
