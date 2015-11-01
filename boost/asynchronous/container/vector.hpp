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

#include <iterator>
#include <limits>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <boost/asynchronous/algorithm/parallel_placement.hpp>
#include <boost/asynchronous/algorithm/parallel_move.hpp>
#include <boost/asynchronous/algorithm/parallel_copy.hpp>
#include <boost/asynchronous/algorithm/parallel_equal.hpp>
#include <boost/asynchronous/algorithm/parallel_lexicographical_compare.hpp>
#include <boost/asynchronous/any_shared_scheduler_proxy.hpp>
#include <boost/asynchronous/post.hpp>
#include <boost/asynchronous/detail/move_bind.hpp>

namespace boost { namespace asynchronous
{
template<typename T, typename Job, typename Alloc>
class vector;
namespace detail
{
// helper for destructors, avoids c++14 move lambda
template <class T>
struct move_only :  public boost::asynchronous::continuation_task<void>
{
    move_only(T&& data)
        : boost::asynchronous::continuation_task<void>("vector_move_only")
        , m_data(std::forward<T>(data)){}
    move_only(move_only&& rhs)noexcept
        : boost::asynchronous::continuation_task<void>("vector_move_only")
        , m_data(std::move(rhs.m_data)){}

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
    void operator()()
    {
        boost::asynchronous::continuation_result<void> task_res = this->this_task_result();
        m_data->clear([task_res]()mutable
            {
                task_res.set_value();
            });
    }

private:
    T m_data;

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
    typedef std::reverse_iterator<const_iterator>  const_reverse_iterator;
    typedef std::reverse_iterator<iterator>		 reverse_iterator;
    typedef Alloc                       allocator_type;

    typedef boost::asynchronous::vector<T,Job,Alloc> this_type;
    typedef boost::shared_ptr<boost::asynchronous::placement_deleter<T,Job,boost::shared_ptr<T>>> internal_data_type;

    enum { default_capacity = 10 };

    // TODO with allocator
    vector()
    : m_data()
    {}

    // only for use from within a threadpool using make_asynchronous_range
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
        boost::shared_ptr<T> raw (m_allocator.allocate(n),[this,n](T* p){this->m_allocator.deallocate(p,n);});
        m_data =  boost::make_shared<boost::asynchronous::placement_deleter<T,Job,boost::shared_ptr<T>>>(n,raw,cutoff,task_name,prio);
        auto fu = boost::asynchronous::post_future(scheduler,
        [n,raw,cutoff,task_name,prio]()mutable
        {
            return boost::asynchronous::parallel_placement<T,Job>(0,n,(char*)raw.get(),T(),cutoff,task_name+"_vector_ctor_placement",prio);
        },
        task_name+"vector_ctor",prio);
        // if exception, will be forwarded
        fu.get();
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
        boost::shared_ptr<T> raw (m_allocator.allocate(default_capacity),
                                  [this](T* p){this->m_allocator.deallocate(p,default_capacity);});

        m_data =  boost::make_shared<boost::asynchronous::placement_deleter<T,Job,boost::shared_ptr<T>>>(0,raw,cutoff,task_name,prio);
    }
    vector(boost::asynchronous::any_shared_scheduler_proxy<Job> scheduler,long cutoff,
           size_type n,
           T const& value,
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
        boost::shared_ptr<T> raw (m_allocator.allocate(n),[this,n](T* p){this->m_allocator.deallocate(p,n);});

        m_data =  boost::make_shared<boost::asynchronous::placement_deleter<T,Job,boost::shared_ptr<T>>>(n,raw,cutoff,task_name,prio);
        auto fu = boost::asynchronous::post_future(scheduler,
        [n,raw,value,cutoff,task_name,prio]()mutable
        {
            return boost::asynchronous::parallel_placement<T,Job>(0,n,(char*)raw.get(),value,cutoff,task_name+"_vector_ctor_placement",prio);
        },
        task_name+"vector_ctor",prio);
        // if exception, will be forwarded
        fu.get();
    }
    template< class InputIt >
    vector(boost::asynchronous::any_shared_scheduler_proxy<Job> scheduler,long cutoff,
           InputIt first, InputIt last,
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
        auto n = std::distance(first,last);
        m_size=n;
        m_capacity=n;
        boost::shared_ptr<T> raw (m_allocator.allocate(n),[this,n](T* p){this->m_allocator.deallocate(p,n);});
        m_data =  boost::make_shared<boost::asynchronous::placement_deleter<T,Job,boost::shared_ptr<T>>>(n,raw,cutoff,task_name,prio);
        auto fu = boost::asynchronous::post_future(scheduler,
        [n,raw,first,cutoff,task_name,prio]()mutable
        {
            return boost::asynchronous::parallel_placement<T,InputIt,boost::shared_ptr<T>,Job>
                        (0,n,first,raw,cutoff,task_name+"_vector_ctor_placement",prio);
        },
        task_name+"vector_ctor",prio);
        // if exception, will be forwarded
        fu.get();
    }
    vector( vector&& other )
        : m_scheduler(std::forward<boost::asynchronous::any_shared_scheduler_proxy<Job>>(other.m_scheduler))
        , m_data(std::forward<internal_data_type>(other.m_data))
        , m_cutoff(other.m_cutoff)
        , m_task_name(std::forward<std::string>(other.m_task_name))
        , m_prio(other.m_prio)
        , m_size(other.m_size)
        , m_capacity(other.m_capacity)
        , m_allocator(other.m_allocator)
    {
    }
    vector( vector const& other )
        : m_scheduler(other.m_scheduler)
        , m_cutoff(other.m_cutoff)
        , m_task_name(other.m_task_name)
        , m_prio(other.m_prio)
        , m_allocator(other.m_allocator)
    {
        // resize to other's size
        auto n = other.size();
        m_size=n;
        m_capacity=n;
        boost::shared_ptr<T> raw (m_allocator.allocate(n),[this,n](T* p){this->m_allocator.deallocate(p,n);});
        auto cutoff = m_cutoff;
        auto task_name = m_task_name;
        auto prio = m_prio;
        m_data =  boost::make_shared<boost::asynchronous::placement_deleter<T,Job,boost::shared_ptr<T>>>(n,raw,cutoff,task_name,prio);
        auto fu = boost::asynchronous::post_future(m_scheduler,
        [n,raw,cutoff,task_name,prio]()mutable
        {
            return boost::asynchronous::parallel_placement<T,Job>
                        (0,n,(char*)raw.get(),T(),cutoff,task_name+"_vector_ctor_placement",prio);
        },
        task_name+"vector_ctor",prio);
        // if exception, will be forwarded
        fu.get();

        // copy
        auto beg_it = begin();
        auto other_end_it = other.end();
        auto other_beg_it = other.begin();
        auto fu2 = boost::asynchronous::post_future(m_scheduler,
        [beg_it,other_end_it,other_beg_it,cutoff,task_name,prio]()mutable
        {
            return boost::asynchronous::parallel_copy<const_iterator,iterator,Job>
                    (other_beg_it,other_end_it,beg_it,cutoff,task_name+"_vector_copy",prio);
        },
        task_name+"_vector_copy_top",prio);
        // if exception, will be forwarded
        fu2.get();
    }
    vector( boost::asynchronous::any_shared_scheduler_proxy<Job> scheduler,long cutoff,
            std::initializer_list<T> init,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
            const std::string& task_name, std::size_t prio
#else
            const std::string& task_name="", std::size_t prio=0
#endif
            /*,const Alloc& = Alloc()*/ )
     : m_scheduler(scheduler)
     , m_cutoff(cutoff)
     , m_task_name(task_name)
     , m_prio(prio)
    {
        auto n = std::distance(init.begin(),init.end());
        auto first = init.begin();
        m_size=n;
        m_capacity=n;
        boost::shared_ptr<T> raw (m_allocator.allocate(n),[this,n](T* p){this->m_allocator.deallocate(p,n);});
        m_data =  boost::make_shared<boost::asynchronous::placement_deleter<T,Job,boost::shared_ptr<T>>>(n,raw,cutoff,task_name,prio);
        auto fu = boost::asynchronous::post_future(scheduler,
        [n,raw,first,cutoff,task_name,prio]()mutable
        {
            return boost::asynchronous::parallel_placement<T,decltype(first),boost::shared_ptr<T>,Job>
                    (0,n,first,raw,cutoff,task_name+"_vector_ctor_placement",prio);
        },
        task_name+"vector_ctor",prio);
        // if exception, will be forwarded
        fu.get();
    }

    ~vector()
    {
        // if in threadpool (algorithms) already, nothing to do, placement deleter will handle dtors and freeing of memory
        // else we need to handle destruction
        if (m_scheduler.is_valid() && !!m_data)
        {
            try
            {
                auto fu =
                boost::asynchronous::post_future(m_scheduler,
                                                 boost::asynchronous::move_bind(
                                                     [](internal_data_type data)mutable
                                                     {
                                                        return boost::asynchronous::top_level_callback_continuation_job<void,Job>(
                                                         boost::asynchronous::detail::move_only<internal_data_type>
                                                            (std::move(data)));
                                                     },
                                                     std::move(m_data)),
                                                 m_task_name+"_vector_dtor",m_prio);

                fu.get();
            }
            catch(std::exception&)
            {}
        }
    }
    allocator_type get_allocator() const
    {
        return m_allocator;
    }
    vector& operator=( vector&& other )
    {
        std::swap(m_scheduler,other.m_scheduler);
        std::swap(m_data,other.m_data);
        std::swap(m_cutoff,other.m_cutoff);
        std::swap(m_task_name,other.m_task_name);
        std::swap(m_prio,other.m_prio);
        std::swap(m_size,other.m_size);
        std::swap(m_capacity,other.m_capacity);
        return *this;
    }

    vector& operator=( const vector& other )
    {
        if (&other != this)
        {
            // we will switch our scheduler to other
            m_scheduler = other.m_scheduler;
            // resize to other's size
            resize(other.size());
            // copy
            auto beg_it = begin();
            auto other_end_it = other.end();
            auto other_beg_it = other.begin();
            auto cutoff = m_cutoff;
            auto task_name = m_task_name;
            auto prio = m_prio;
            auto fu = boost::asynchronous::post_future(m_scheduler,
            [beg_it,other_end_it,other_beg_it,cutoff,task_name,prio]()mutable
            {
                return boost::asynchronous::parallel_copy<const_iterator,iterator,Job>
                        (other_beg_it,other_end_it,beg_it,cutoff,task_name+"_vector_copy",prio);
            },
            task_name+"_vector_copy_top",prio);
            // if exception, will be forwarded
            fu.get();
            m_cutoff = other.m_cutoff;
            m_task_name = other.m_task_name;
            m_prio = other.m_prio;
        }
        return *this;
    }
    template< class InputIt >
    void assign( InputIt first, InputIt last )
    {
        // resize
        resize(std::distance(first,last));
        // copy
        auto beg_it = begin();
        auto cutoff = m_cutoff;
        auto task_name = m_task_name;
        auto prio = m_prio;
        auto fu = boost::asynchronous::post_future(m_scheduler,
        [beg_it,first,last,cutoff,task_name,prio]()mutable
        {
            return boost::asynchronous::parallel_copy<InputIt,iterator,Job>
                    (first,last,beg_it,cutoff,task_name+"_vector_assign",prio);
        },
        task_name+"_vector_assign_top",prio);
        // if exception, will be forwarded
        fu.get();
    }
    void assign( size_type count, const T& value )
    {
        this_type temp(m_scheduler,m_cutoff,count,value,m_task_name,m_prio);
        *this = std::move(temp);
    }
    void assign( std::initializer_list<T> ilist )
    {
        // resize
        auto first = ilist.begin();
        auto last = ilist.end();
        resize(std::distance(first,last));
        // copy
        auto beg_it = begin();
        auto cutoff = m_cutoff;
        auto task_name = m_task_name;
        auto prio = m_prio;
        auto fu = boost::asynchronous::post_future(m_scheduler,
        [beg_it,first,last,cutoff,task_name,prio]()mutable
        {
            return boost::asynchronous::parallel_copy<decltype(first),iterator,Job>
                    (first,last,beg_it,cutoff,task_name+"_vector_assign",prio);
        },
        task_name+"_vector_assign_top",prio);
        // if exception, will be forwarded
        fu.get();
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
    reverse_iterator rbegin()
    {
        return reverse_iterator(end());
    }
    const_reverse_iterator rbegin() const
    {
        return const_reverse_iterator(end());
    }
    const_reverse_iterator crbegin() const
    {
        return const_reverse_iterator(cend());
    }
    reverse_iterator rend()
    {
        return reverse_iterator(begin());
    }
    const_reverse_iterator rend() const
    {
        return const_reverse_iterator(begin());
    }
    const_reverse_iterator crend() const
    {
        return const_reverse_iterator(cbegin());
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
        if (m_scheduler.is_valid() && !!m_data)
        {
            try
            {
                auto fu =
                boost::asynchronous::post_future(m_scheduler,
                                                 boost::asynchronous::move_bind(
                                                     [](internal_data_type data)mutable
                                                     {
                                                        return boost::asynchronous::top_level_callback_continuation_job<void,Job>(
                                                         boost::asynchronous::detail::move_only<internal_data_type>
                                                            (std::move(data)));
                                                     },
                                                     std::move(m_data)),
                                                 m_task_name+"_vector_clear",m_prio);

                fu.get();
            }
            catch(std::exception&)
            {}
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
            auto new_memory = reallocate_helper(calc_new_capacity(size()));
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
            auto new_memory = reallocate_helper(calc_new_capacity(size()));
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
            auto new_memory = reallocate_helper(calc_new_capacity(size()));
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
    void resize( size_type count, T value = T() )
    {
        if (count == size())
            return;
        auto cutoff = m_cutoff;
        auto task_name = m_task_name;
        auto prio = m_prio;

        if (count > size())
        {
            // we need to allocate new elements
            if (count > capacity())
            {
                // reallocate memory
                reserve(count);
            }
            // add count - size() elements
            auto s = size();
            char* raw = (char*) begin();
            auto fu = boost::asynchronous::post_future(m_scheduler,
            [s,count,raw,value,cutoff,task_name,prio]()mutable
            {
                return boost::asynchronous::parallel_placement<T,Job>(s,count,raw,value,cutoff,task_name+"_vector_resize_placement",prio);
            },
            task_name+"_vector_resize_placement_top",prio);
            // if exception, will be forwarded
            auto placement_result = fu.get();
            if (placement_result.first != boost::asynchronous::detail::parallel_placement_helper_enum::success)
            {
                boost::rethrow_exception(placement_result.second);
            }
            m_size = count;
            m_data->size_ = count;
        }
        else
        {
            // remove elements
            auto s = size();
            m_size = count;
            m_data->size_ = count;
            auto raw = m_data->data_;
            auto fu =
            boost::asynchronous::post_future(m_scheduler,
            [s,count,raw,cutoff,task_name,prio]()mutable
            {
                return boost::asynchronous::parallel_placement_delete<T,Job>(raw,count,s,cutoff,task_name+"_vector_resize_placement_delete",prio);
            },
            task_name+"_vector_resize_placement_delete_top",prio);
            fu.get();
        }
    }
    iterator erase( const_iterator first, const_iterator last )
    {
        auto cutoff = m_cutoff;
        auto task_name = m_task_name;
        auto prio = m_prio;
        auto cur_end = end();
        if (last == end())
        {
            // nothing to move, just resize
            resize(first - cbegin());
            return end();
        }
        else if((last - first) >= cend() - last )
        {
            // move our data to new memory
            auto fu = boost::asynchronous::post_future(m_scheduler,
            [first,last,cur_end,cutoff,task_name,prio]()mutable
            {
                return boost::asynchronous::parallel_move<iterator,iterator,Job>
                        ((iterator)last,(iterator)cur_end,(iterator)first,cutoff,task_name+"_vector_erase_move",prio);
            },
            task_name+"_vector_erase_move_top",prio);
            // if exception, will be forwarded
            fu.get();
            // set to new size
            resize(size()-(last - first));
            return end();
        }
        else
        {
            // least efficient way, requires twice moving as parallel_move does not support overlapping ranges
            auto n = cur_end-last;

            // create temporary buffer
            boost::shared_array<char> raw (new char[n * sizeof(T)]);
            auto fu = boost::asynchronous::post_future(m_scheduler,
            [n,raw,cutoff,task_name,prio]()mutable
            {
                return boost::asynchronous::parallel_placement<T,Job>(0,n,raw,cutoff,task_name+"_vector_erase_placement",prio);
            },
            task_name+"_vector_erase_placement_top",prio);
            // if exception, will be forwarded
            auto placement_result = fu.get();
            if (placement_result.first != boost::asynchronous::detail::parallel_placement_helper_enum::success)
            {
                boost::rethrow_exception(placement_result.second);
            }
            fu.get();
            // move to temporary buffer
            auto temp_begin = (iterator)raw.get();
            auto fu2 = boost::asynchronous::post_future(m_scheduler,
            [cur_end,last,temp_begin,cutoff,task_name,prio]()mutable
            {
                return boost::asynchronous::parallel_move<iterator,iterator,Job>
                        ((iterator)last,(iterator)cur_end,temp_begin,cutoff,task_name+"_vector_erase_move",prio);
            },
            task_name+"_vector_erase_move_top",prio);
            // if exception, will be forwarded
            fu2.get();

            // move back to vector
            auto temp_end = temp_begin + n;
            auto fu3 = boost::asynchronous::post_future(m_scheduler,
            [temp_begin,temp_end,first,cutoff,task_name,prio]()mutable
            {
                return boost::asynchronous::parallel_move<iterator,iterator,Job>
                        (temp_begin,temp_end,(iterator)first,cutoff,task_name+"_vector_erase_move",prio);
            },
            task_name+"_vector_erase_move_top",prio);
            // if exception, will be forwarded
            fu3.get();
            // call dtors on temp
            boost::shared_ptr<boost::asynchronous::placement_deleter<T,Job>> temp_deleter =
                    boost::make_shared<boost::asynchronous::placement_deleter<T,Job>>(n,std::move(raw),cutoff,task_name,prio);
            auto fu4 =
            boost::asynchronous::post_future(m_scheduler,
                                             boost::asynchronous::move_bind(
                                                 [](boost::shared_ptr<boost::asynchronous::placement_deleter<T,Job>> data)mutable
                                                 {
                                                    return boost::asynchronous::top_level_callback_continuation_job<void,Job>(
                                                     boost::asynchronous::detail::move_only<boost::shared_ptr<boost::asynchronous::placement_deleter<T,Job>>>
                                                        (std::move(data)));
                                                 },
                                                 std::move(temp_deleter)),
                                             task_name+"_vector_erase_delete",prio);

            fu4.get();
            // remove our excess elements
            resize(size()-(last - first));
            return end();
        }
    }
    iterator erase( const_iterator pos)
    {
        return erase(pos,pos+1);
    }
    template< class InputIt >
    iterator insert( const_iterator orgpos, InputIt first, InputIt last )
    {
        if (first == last)
        {
            // nothing to do
            return (iterator)orgpos;
        }
        auto distance_begin_pos = orgpos-cbegin();
        // check if we need to reallocate
        if (size()+(last-first) > capacity())
        {
            auto new_capacity = calc_new_capacity(size()+(last-first));
            reallocate_helper(new_capacity);
        }
        iterator pos = begin() + distance_begin_pos;
        // we need move twice data after pos as parallel_move does not support overlapping ranges
        // first we create a temporary buffer and move our data after pos to temporary buffer
        auto n = cend() - pos;
        boost::shared_array<char> raw (new char[n * sizeof(T)]);
        auto cutoff = m_cutoff;
        auto task_name = m_task_name;
        auto prio = m_prio;
        auto fu = boost::asynchronous::post_future(m_scheduler,
        [n,raw,cutoff,task_name,prio]()mutable
        {
            return boost::asynchronous::parallel_placement<T,Job>(0,n,raw,cutoff,task_name+"_vector_insert_placement",prio);
        },
        task_name+"_vector_insert_placement_top",prio);
        // if exception, will be forwarded
        auto placement_result = fu.get();
        if (placement_result.first != boost::asynchronous::detail::parallel_placement_helper_enum::success)
        {
            boost::rethrow_exception(placement_result.second);
        }
        fu.get();
        // move to temporary buffer
        auto temp_begin = (iterator)raw.get();
        auto cur_begin = pos;
        auto cur_end = cend();
        auto fu2 = boost::asynchronous::post_future(m_scheduler,
        [cur_end,cur_begin,temp_begin,cutoff,task_name,prio]()mutable
        {
            return boost::asynchronous::parallel_move<iterator,iterator,Job>
                    ((iterator)cur_begin,(iterator)cur_end,temp_begin,cutoff,task_name+"_vector_insert_move",prio);
        },
        task_name+"_vector_insert_move_top",prio);
        // if exception, will be forwarded
        fu2.get();
        // we need to add the missing placement new's
        auto to_add = (last - first);
        auto where = ((iterator)m_data->data())+ size();
        auto fu3 = boost::asynchronous::post_future(m_scheduler,
        [to_add,where,cutoff,task_name,prio]()mutable
        {
            return boost::asynchronous::parallel_placement<T,Job>(0,to_add,(char*)where,T(),cutoff,task_name+"_vector_insert_placement_2",prio);
        },
        task_name+"_vector_insert_placement_2_top",prio);
        // if exception, will be forwarded
        auto placement_result2 = fu3.get();
        if (placement_result2.first != boost::asynchronous::detail::parallel_placement_helper_enum::success)
        {
            boost::rethrow_exception(placement_result2.second);
        }
        // copy input
        auto fu4 = boost::asynchronous::post_future(m_scheduler,
        [first,last,pos,cutoff,task_name,prio]()mutable
        {
            return boost::asynchronous::parallel_copy<InputIt,iterator,Job>
                    (first,last,(iterator)pos,cutoff,task_name+"_vector_insert_copy",prio);
        },
        task_name+"_vector_insert_copy_top",prio);
        fu4.get();
        // move back from temporary
        auto temp_end = temp_begin + n;
        auto beg_move_back = pos + (last-first);
        auto fu5 = boost::asynchronous::post_future(m_scheduler,
        [temp_begin,temp_end,beg_move_back,cutoff,task_name,prio]()mutable
        {
            return boost::asynchronous::parallel_move<iterator,iterator,Job>
                    (temp_begin,temp_end,(iterator)beg_move_back,cutoff,task_name+"_vector_insert_move_2",prio);
        },
        task_name+"_vector_insert_move_2_top",prio);
        // if exception, will be forwarded
        fu5.get();
        // call dtors on temp
        boost::shared_ptr<boost::asynchronous::placement_deleter<T,Job>> temp_deleter =
                boost::make_shared<boost::asynchronous::placement_deleter<T,Job>>(n,std::move(raw),cutoff,task_name,prio);
        auto fu6 =
        boost::asynchronous::post_future(m_scheduler,
                                         boost::asynchronous::move_bind(
                                             [](boost::shared_ptr<boost::asynchronous::placement_deleter<T,Job>> data)mutable
                                             {
                                                return boost::asynchronous::top_level_callback_continuation_job<void,Job>(
                                                 boost::asynchronous::detail::move_only<boost::shared_ptr<boost::asynchronous::placement_deleter<T,Job>>>
                                                    (std::move(data)));
                                             },
                                             std::move(temp_deleter)),
                                         task_name+"_vector_insert_delete",prio);

        fu6.get();
        // update size
        m_size += (last-first);
        m_data->size_ += (last-first);

        return begin() + distance_begin_pos;
    }
    iterator insert( const_iterator orgpos, const T& value )
    {
        auto distance_begin_pos = orgpos-cbegin();
        // check if we need to reallocate
        if (size()+1 > capacity())
        {
            auto new_capacity = calc_new_capacity(size()+1);
            reallocate_helper(new_capacity);
        }
        iterator pos = begin() + distance_begin_pos;
        // we need move twice data after pos as parallel_move does not support overlapping ranges
        // first we create a temporary buffer and move our data after pos to temporary buffer
        auto n = cend() - pos;
        boost::shared_array<char> raw (new char[n * sizeof(T)]);
        auto cutoff = m_cutoff;
        auto task_name = m_task_name;
        auto prio = m_prio;
        auto fu = boost::asynchronous::post_future(m_scheduler,
        [n,raw,cutoff,task_name,prio]()mutable
        {
            return boost::asynchronous::parallel_placement<T,Job>(0,n,raw,cutoff,task_name+"_vector_insert_placement",prio);
        },
        task_name+"_vector_insert_placement_top",prio);
        // if exception, will be forwarded
        auto placement_result = fu.get();
        if (placement_result.first != boost::asynchronous::detail::parallel_placement_helper_enum::success)
        {
            boost::rethrow_exception(placement_result.second);
        }
        fu.get();
        // move to temporary buffer
        auto temp_begin = (iterator)raw.get();
        auto cur_begin = pos;
        auto cur_end = cend();
        auto fu2 = boost::asynchronous::post_future(m_scheduler,
        [cur_end,cur_begin,temp_begin,cutoff,task_name,prio]()mutable
        {
            return boost::asynchronous::parallel_move<iterator,iterator,Job>
                    ((iterator)cur_begin,(iterator)cur_end,temp_begin,cutoff,task_name+"_vector_insert_move",prio);
        },
        task_name+"_vector_insert_move_top",prio);
        // if exception, will be forwarded
        fu2.get();
        // we need to add the missing placement new's
        auto to_add = 1;
        auto where = ((iterator)m_data->data())+ size();
        auto fu3 = boost::asynchronous::post_future(m_scheduler,
        [to_add,where,cutoff,task_name,prio]()mutable
        {
            return boost::asynchronous::parallel_placement<T,Job>(0,to_add,(char*)where,T(),cutoff,task_name+"_vector_insert_placement_2",prio);
        },
        task_name+"_vector_insert_placement_2_top",prio);
        // if exception, will be forwarded
        auto placement_result2 = fu3.get();
        if (placement_result2.first != boost::asynchronous::detail::parallel_placement_helper_enum::success)
        {
            boost::rethrow_exception(placement_result2.second);
        }

        // copy input
        T a[1] = {value};
        auto fu4 = boost::asynchronous::post_future(m_scheduler,
        [a,pos,cutoff,task_name,prio]()mutable
        {
            return boost::asynchronous::parallel_copy<T*,iterator,Job>
                    (a,a+1,(iterator)pos,cutoff,task_name+"_vector_insert_copy",prio);
        },
        task_name+"_vector_insert_copy_top",prio);
        fu4.get();
        // move back from temporary
        auto temp_end = temp_begin + n;
        auto beg_move_back = pos + 1;
        auto fu5 = boost::asynchronous::post_future(m_scheduler,
        [temp_begin,temp_end,beg_move_back,cutoff,task_name,prio]()mutable
        {
            return boost::asynchronous::parallel_move<iterator,iterator,Job>
                    (temp_begin,temp_end,(iterator)beg_move_back,cutoff,task_name+"_vector_insert_move_2",prio);
        },
        task_name+"_vector_insert_move_2_top",prio);
        // if exception, will be forwarded
        fu5.get();
        // call dtors on temp
        boost::shared_ptr<boost::asynchronous::placement_deleter<T,Job>> temp_deleter =
                boost::make_shared<boost::asynchronous::placement_deleter<T,Job>>(n,std::move(raw),cutoff,task_name,prio);
        auto fu6 =
        boost::asynchronous::post_future(m_scheduler,
                                         boost::asynchronous::move_bind(
                                             [](boost::shared_ptr<boost::asynchronous::placement_deleter<T,Job>> data)mutable
                                             {
                                                return boost::asynchronous::top_level_callback_continuation_job<void,Job>(
                                                 boost::asynchronous::detail::move_only<boost::shared_ptr<boost::asynchronous::placement_deleter<T,Job>>>
                                                    (std::move(data)));
                                             },
                                             std::move(temp_deleter)),
                                         task_name+"_vector_insert_delete",prio);

        fu6.get();
        // update size
        ++m_size;
        ++m_data->size_;

        return begin() + distance_begin_pos;
    }
    iterator insert( const_iterator orgpos, T&& value )
    {
        auto distance_begin_pos = orgpos-cbegin();
        // check if we need to reallocate
        if (size()+1 > capacity())
        {
            auto new_capacity = calc_new_capacity(size()+1);
            reallocate_helper(new_capacity);
        }
        iterator pos = begin() + distance_begin_pos;
        // we need move twice data after pos as parallel_move does not support overlapping ranges
        // first we create a temporary buffer and move our data after pos to temporary buffer
        auto n = cend() - pos;
        boost::shared_array<char> raw (new char[n * sizeof(T)]);
        auto cutoff = m_cutoff;
        auto task_name = m_task_name;
        auto prio = m_prio;
        auto fu = boost::asynchronous::post_future(m_scheduler,
        [n,raw,cutoff,task_name,prio]()mutable
        {
            return boost::asynchronous::parallel_placement<T,Job>(0,n,raw,cutoff,task_name+"_vector_insert_placement",prio);
        },
        task_name+"_vector_insert_placement_top",prio);
        // if exception, will be forwarded
        auto placement_result = fu.get();
        if (placement_result.first != boost::asynchronous::detail::parallel_placement_helper_enum::success)
        {
            boost::rethrow_exception(placement_result.second);
        }
        fu.get();
        // move to temporary buffer
        auto temp_begin = (iterator)raw.get();
        auto cur_begin = pos;
        auto cur_end = cend();
        auto fu2 = boost::asynchronous::post_future(m_scheduler,
        [cur_end,cur_begin,temp_begin,cutoff,task_name,prio]()mutable
        {
            return boost::asynchronous::parallel_move<iterator,iterator,Job>
                    ((iterator)cur_begin,(iterator)cur_end,temp_begin,cutoff,task_name+"_vector_insert_move",prio);
        },
        task_name+"_vector_insert_move_top",prio);
        // if exception, will be forwarded
        fu2.get();
        // we need to add the missing placement new's
        auto to_add = 1;
        auto where = ((iterator)m_data->data())+ size();
        auto fu3 = boost::asynchronous::post_future(m_scheduler,
        [to_add,where,cutoff,task_name,prio]()mutable
        {
            return boost::asynchronous::parallel_placement<T,Job>(0,to_add,(char*)where,T(),cutoff,task_name+"_vector_insert_placement_2",prio);
        },
        task_name+"_vector_insert_placement_2_top",prio);
        // if exception, will be forwarded
        auto placement_result2 = fu3.get();
        if (placement_result2.first != boost::asynchronous::detail::parallel_placement_helper_enum::success)
        {
            boost::rethrow_exception(placement_result2.second);
        }

        // copy input
        T a[1] = {std::forward<T>(value)};
        auto fu4 = boost::asynchronous::post_future(m_scheduler,
        [a,pos,cutoff,task_name,prio]()mutable
        {
            return boost::asynchronous::parallel_copy<T*,iterator,Job>
                    (a,a+1,(iterator)pos,cutoff,task_name+"_vector_insert_copy",prio);
        },
        task_name+"_vector_insert_copy_top",prio);
        fu4.get();
        // move back from temporary
        auto temp_end = temp_begin + n;
        auto beg_move_back = pos + 1;
        auto fu5 = boost::asynchronous::post_future(m_scheduler,
        [temp_begin,temp_end,beg_move_back,cutoff,task_name,prio]()mutable
        {
            return boost::asynchronous::parallel_move<iterator,iterator,Job>
                    (temp_begin,temp_end,(iterator)beg_move_back,cutoff,task_name+"_vector_insert_move_2",prio);
        },
        task_name+"_vector_insert_move_2_top",prio);
        // if exception, will be forwarded
        fu5.get();
        // call dtors on temp
        boost::shared_ptr<boost::asynchronous::placement_deleter<T,Job>> temp_deleter =
                boost::make_shared<boost::asynchronous::placement_deleter<T,Job>>(n,std::move(raw),cutoff,task_name,prio);
        auto fu6 =
        boost::asynchronous::post_future(m_scheduler,
                                         boost::asynchronous::move_bind(
                                             [](boost::shared_ptr<boost::asynchronous::placement_deleter<T,Job>> data)mutable
                                             {
                                                return boost::asynchronous::top_level_callback_continuation_job<void,Job>(
                                                 boost::asynchronous::detail::move_only<boost::shared_ptr<boost::asynchronous::placement_deleter<T,Job>>>
                                                    (std::move(data)));
                                             },
                                             std::move(temp_deleter)),
                                         task_name+"_vector_insert_delete",prio);

        fu6.get();
        // update size
        ++m_size;
        ++m_data->size_;

        return begin() + distance_begin_pos;
    }

    iterator insert( const_iterator orgpos, size_type count, const T& value )
    {
        auto distance_begin_pos = orgpos-cbegin();
        // check if we need to reallocate
        if (size()+count > capacity())
        {
            auto new_capacity = calc_new_capacity(size()+count);
            reallocate_helper(new_capacity);
        }
        iterator pos = begin() + distance_begin_pos;
        // we need move twice data after pos as parallel_move does not support overlapping ranges
        // first we create a temporary buffer and move our data after pos to temporary buffer
        auto n = cend() - pos;
        boost::shared_array<char> raw (new char[n * sizeof(T)]);
        auto cutoff = m_cutoff;
        auto task_name = m_task_name;
        auto prio = m_prio;
        auto fu = boost::asynchronous::post_future(m_scheduler,
        [n,raw,cutoff,task_name,prio]()mutable
        {
            return boost::asynchronous::parallel_placement<T,Job>(0,n,raw,cutoff,task_name+"_vector_insert_placement",prio);
        },
        task_name+"_vector_insert_placement_top",prio);
        // if exception, will be forwarded
        auto placement_result = fu.get();
        if (placement_result.first != boost::asynchronous::detail::parallel_placement_helper_enum::success)
        {
            boost::rethrow_exception(placement_result.second);
        }
        fu.get();
        // move to temporary buffer
        auto temp_begin = (iterator)raw.get();
        auto cur_begin = pos;
        auto cur_end = cend();
        auto fu2 = boost::asynchronous::post_future(m_scheduler,
        [cur_end,cur_begin,temp_begin,cutoff,task_name,prio]()mutable
        {
            return boost::asynchronous::parallel_move<iterator,iterator,Job>
                    ((iterator)cur_begin,(iterator)cur_end,temp_begin,cutoff,task_name+"_vector_insert_move",prio);
        },
        task_name+"_vector_insert_move_top",prio);
        // if exception, will be forwarded
        fu2.get();
        // we need to add the missing placement new's
        auto to_add = count;
        auto where = ((iterator)m_data->data())+ size();
        auto fu3 = boost::asynchronous::post_future(m_scheduler,
        [to_add,where,cutoff,task_name,prio]()mutable
        {
            return boost::asynchronous::parallel_placement<T,Job>(0,to_add,(char*)where,T(),cutoff,task_name+"_vector_insert_placement_2",prio);
        },
        task_name+"_vector_insert_placement_2_top",prio);
        // if exception, will be forwarded
        auto placement_result2 = fu3.get();
        if (placement_result2.first != boost::asynchronous::detail::parallel_placement_helper_enum::success)
        {
            boost::rethrow_exception(placement_result2.second);
        }

        // copy input
        std::vector<T> a(count,value);
        auto fu4 = boost::asynchronous::post_future(m_scheduler,
        [&a,pos,cutoff,task_name,prio]()mutable
        {
            return boost::asynchronous::parallel_copy<typename std::vector<T>::const_iterator,iterator,Job>
                    (a.cbegin(),a.cend(),(iterator)pos,cutoff,task_name+"_vector_insert_copy",prio);
        },
        task_name+"_vector_insert_copy_top",prio);
        fu4.get();
        // move back from temporary
        auto temp_end = temp_begin + n;
        auto beg_move_back = pos + count;
        auto fu5 = boost::asynchronous::post_future(m_scheduler,
        [temp_begin,temp_end,beg_move_back,cutoff,task_name,prio]()mutable
        {
            return boost::asynchronous::parallel_move<iterator,iterator,Job>
                    (temp_begin,temp_end,(iterator)beg_move_back,cutoff,task_name+"_vector_insert_move_2",prio);
        },
        task_name+"_vector_insert_move_2_top",prio);
        // if exception, will be forwarded
        fu5.get();
        // call dtors on temp
        boost::shared_ptr<boost::asynchronous::placement_deleter<T,Job>> temp_deleter =
                boost::make_shared<boost::asynchronous::placement_deleter<T,Job>>(n,std::move(raw),cutoff,task_name,prio);
        auto fu6 =
        boost::asynchronous::post_future(m_scheduler,
                                         boost::asynchronous::move_bind(
                                             [](boost::shared_ptr<boost::asynchronous::placement_deleter<T,Job>> data)mutable
                                             {
                                                return boost::asynchronous::top_level_callback_continuation_job<void,Job>(
                                                 boost::asynchronous::detail::move_only<boost::shared_ptr<boost::asynchronous::placement_deleter<T,Job>>>
                                                    (std::move(data)));
                                             },
                                             std::move(temp_deleter)),
                                         task_name+"_vector_erase_delete",prio);

        fu6.get();
        // update size
        m_size += count;
        m_data->size_ += count;

        return begin() + distance_begin_pos;
    }
    iterator insert( const_iterator pos, std::initializer_list<T> ilist )
    {
        return this->insert(pos,ilist.begin(),ilist.end());
    }

    // only for use by asynchronous algorithms
    void set_internal_data(internal_data_type data, std::size_t capacity)
    {
        m_data = std::move(data);
        m_size = m_data->size_;
        m_capacity = capacity;
    }


    std::size_t calc_new_capacity(size_type hint) const
    {
        return (hint *2) + 10;
    }

    // asynchronous members
    boost::asynchronous::detail::callback_continuation<internal_data_type,Job>
    async_reallocate(size_type new_memory, size_type new_size)
    {
        return boost::asynchronous::top_level_callback_continuation_job<internal_data_type,Job>
            (async_reallocate_task(new_memory,m_data,begin(),end(),m_allocator,new_size,m_cutoff,m_task_name,m_prio));

    }
    // needed for vectors resulting from async calls and having therefore no scheduler
    void set_scheduler(boost::asynchronous::any_shared_scheduler_proxy<Job> scheduler)
    {
        m_scheduler = scheduler;
    }
    // Releasing the scheduler means the vector cannot call any members needing to execute in parallel
    // This includes destruction!
    // In exchange it can be posted in its scheduler's thread or in a post_callback.
    // After being used in the threadpool, the scheduler should be reset using set_scheduler()
    void release_scheduler()
    {
        m_scheduler.reset();
    }

    long get_cutoff()const
    {
        return m_cutoff;
    }

    std::string get_name() const
    {
        return m_task_name;
    }

    std::size_t get_prio()const
    {
        return m_prio;
    }
    // only for internal algorithm use
    internal_data_type get_internal_data() const
    {
        return m_data;
    }

private:

    std::size_t reallocate_helper(size_type new_memory)
    {
        boost::shared_ptr<T> raw (m_allocator.allocate(new_memory),[this,new_memory](T* p){this->m_allocator.deallocate(p,new_memory);});

        // create our current number of objects with placement new
        auto n = m_size;
        auto cutoff = m_cutoff;
        auto task_name = m_task_name;
        auto prio = m_prio;
        auto fu = boost::asynchronous::post_future(m_scheduler,
        [n,raw,cutoff,task_name,prio]()mutable
        {
            return boost::asynchronous::parallel_placement<T,Job>(0,n,(char*)raw.get(),T(),cutoff,task_name+"vector_reallocate_placement",prio);
        },
        task_name+"_vector_reallocate_placement_top",prio);
        // if exception, will be forwarded
        auto placement_result = fu.get();
        if (placement_result.first != boost::asynchronous::detail::parallel_placement_helper_enum::success)
        {
            boost::rethrow_exception(placement_result.second);
        }

        auto new_data =  boost::make_shared<boost::asynchronous::placement_deleter<T,Job,boost::shared_ptr<T>>>
                (n,std::move(raw),cutoff,task_name,prio);

        auto beg_it = begin();
        auto end_it = end();

        // move our data to new memory
        auto fu2 = boost::asynchronous::post_future(m_scheduler,
        [new_data,beg_it,end_it,cutoff,task_name,prio]()mutable
        {
            return boost::asynchronous::parallel_move<iterator,iterator,Job>(beg_it,end_it,(iterator)(new_data->data()),cutoff,task_name+"_vector_reallocate_move",prio);
        },
        task_name+"_reallocate_move_top",prio);
        // if exception, will be forwarded
        fu2.get();
        // destroy our old data (wait until done)
        auto fu3 =
        boost::asynchronous::post_future(m_scheduler,
                                         boost::asynchronous::move_bind(
                                             [](internal_data_type data)mutable
                                             {
                                                return boost::asynchronous::top_level_callback_continuation_job<void,Job>(
                                                 boost::asynchronous::detail::move_only<internal_data_type>
                                                    (std::move(data)));
                                             },
                                             std::move(m_data)),
                                         m_task_name+"_vector_reallocate_delete",m_prio);

        fu3.get();
        std::swap(m_data,new_data);

        return new_memory;
    }

    struct async_reallocate_task: public boost::asynchronous::continuation_task<internal_data_type>
    {
        async_reallocate_task(size_type new_memory,
                              internal_data_type data,iterator beg, iterator end,
                              Alloc alloc,std::size_t size, long cutoff, std::string const& task_name, std::size_t prio )
            : boost::asynchronous::continuation_task<internal_data_type>(task_name)
            , m_new_memory(new_memory),m_data(data),m_begin(beg),m_end(end),m_allocator(alloc),m_size(size),m_cutoff(cutoff),m_prio(prio)
        {}
        void operator()()
        {
            boost::asynchronous::continuation_result<internal_data_type> task_res = this->this_task_result();
            try
            {
                auto old_data = std::move(m_data);
                auto new_memory = m_new_memory;
                auto alloc = m_allocator;
                auto size = m_size;
                auto cutoff = m_cutoff;
                auto task_name = this->get_name();
                auto prio = m_prio;
                auto beg = m_begin; auto end =m_end;

                boost::shared_ptr<T> raw (m_allocator.allocate(new_memory),[this,new_memory,alloc](T* p)mutable{alloc.deallocate(p,new_memory);});

                auto cont_p = boost::asynchronous::parallel_placement<T,Job>
                        (0,m_size,(char*)raw.get(),T(),m_cutoff,this->get_name()+"vector_reallocate_placement",m_prio);
                cont_p.on_done([task_res,old_data,raw,beg,end,size,cutoff,task_name,prio]
                               (std::tuple<boost::asynchronous::expected<boost::asynchronous::detail::parallel_placement_helper_result> >&& cres)mutable
                {
                    auto res =std::get<0>(cres).get();
                    if (res.first != boost::asynchronous::detail::parallel_placement_helper_enum::success)
                    {
                        task_res.set_exception(res.second);
                        return;
                    }
                    auto new_data =  boost::make_shared<boost::asynchronous::placement_deleter<T,Job,boost::shared_ptr<T>>>
                            (size,std::move(raw),cutoff,task_name,prio);
                    auto cont_m = boost::asynchronous::parallel_move<iterator,iterator,Job>
                                     (beg,end,(iterator)(new_data->data()),cutoff,task_name+"_vector_reallocate_move",prio);
                    cont_m.on_done([task_res,old_data,new_data,raw,beg,end,size,cutoff,task_name,prio]
                                   (std::tuple<boost::asynchronous::expected<void> >&& mres)mutable
                    {
                        try
                        {
                            // check for exceptions
                            std::get<0>(mres).get();
                            // delete old data
                            auto cont_d = boost::asynchronous::top_level_callback_continuation_job<void,Job>(
                                                                             boost::asynchronous::detail::move_only<internal_data_type>
                                                                                (std::move(old_data)));
                            cont_d.on_done([task_res,new_data](std::tuple<boost::asynchronous::expected<void>>&& dres)mutable
                            {
                                try
                                {
                                    // check for exceptions
                                    std::get<0>(dres).get();
                                    task_res.set_value(std::move(new_data));
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

                    });
                });

            }
            catch(std::exception& e)
            {
                task_res.set_exception(boost::copy_exception(e));
            }
        }
        size_type m_new_memory;
        internal_data_type m_data;
        iterator m_begin;
        iterator m_end;
        Alloc m_allocator;
        std::size_t m_size;
        long m_cutoff;
        std::size_t m_prio;
    };

private:
    template< class _T, class _Job, class _Alloc >
    friend bool operator==( const boost::asynchronous::vector<_T,_Job,_Alloc>&,
                            const boost::asynchronous::vector<_T,_Job,_Alloc>& );
    template< class _T, class _Job, class _Alloc >
    friend bool operator<( const boost::asynchronous::vector<_T,_Job,_Alloc>&,
                           const boost::asynchronous::vector<_T,_Job,_Alloc>& );

    // only used from "outside" (i.e. not algorithms)
    boost::asynchronous::any_shared_scheduler_proxy<Job> m_scheduler;
    internal_data_type m_data;
    long m_cutoff;
    std::string m_task_name;
    std::size_t m_prio;
    std::size_t m_size;
    std::size_t m_capacity;
    Alloc m_allocator;
};

// comparion operators
template< class T, class Job, class Alloc >
bool operator==( const boost::asynchronous::vector<T,Job,Alloc>& lhs,
                 const boost::asynchronous::vector<T,Job,Alloc>& rhs )
{
    if (lhs.size() != rhs.size())
    {
        return false;
    }
    auto lhs_beg = lhs.begin();
    auto lhs_end = lhs.end();
    auto rhs_beg = rhs.begin();
    auto cutoff = lhs.m_cutoff;
    auto task_name = lhs.m_task_name;
    auto prio = lhs.m_prio;
    auto fu = boost::asynchronous::post_future(lhs.m_scheduler,
    [lhs_beg,lhs_end,rhs_beg,cutoff,task_name,prio]()mutable
    {
        return boost::asynchronous::parallel_equal<typename boost::asynchronous::vector<T,Job,Alloc>::const_iterator,
                                                   typename boost::asynchronous::vector<T,Job,Alloc>::const_iterator,
                                                   Job>(lhs_beg,lhs_end,rhs_beg,cutoff,task_name+"_vector_equal",prio);
    },
    task_name+"_vector_equal_top",prio);
    return fu.get();
}
template< class T, class Job, class Alloc >
bool operator!=( const boost::asynchronous::vector<T,Job,Alloc>& lhs,
                 const boost::asynchronous::vector<T,Job,Alloc>& rhs )
{
    return !(lhs == rhs);
}
template< class T, class Job, class Alloc >
bool operator<( const boost::asynchronous::vector<T,Job,Alloc>& lhs,
                const boost::asynchronous::vector<T,Job,Alloc>& rhs )
{
    auto lhs_beg = lhs.begin();
    auto lhs_end = lhs.end();
    auto rhs_beg = rhs.begin();
    auto rhs_end = rhs.end();
    auto cutoff = lhs.m_cutoff;
    auto task_name = lhs.m_task_name;
    auto prio = lhs.m_prio;

    auto fu = boost::asynchronous::post_future(lhs.m_scheduler,
    [lhs_beg,lhs_end,rhs_beg,rhs_end,cutoff,task_name,prio]()mutable
    {
        return boost::asynchronous::parallel_lexicographical_compare<
                typename boost::asynchronous::vector<T,Job,Alloc>::const_iterator,
                typename boost::asynchronous::vector<T,Job,Alloc>::const_iterator,
                Job>
                (lhs_beg,lhs_end,rhs_beg,rhs_end,cutoff,task_name+"_vector_operator_less",prio);
    },
    task_name+"_vector_operator_less_top",prio);
    return fu.get();
}
template< class T, class Job, class Alloc >
bool operator<=( const boost::asynchronous::vector<T,Job,Alloc>& lhs,
                 const boost::asynchronous::vector<T,Job,Alloc>& rhs )
{
    return (lhs < rhs) || (lhs == rhs);
}
template< class T, class Job, class Alloc >
bool operator>=( const boost::asynchronous::vector<T,Job,Alloc>& lhs,
                 const boost::asynchronous::vector<T,Job,Alloc>& rhs )
{
    return !(lhs < rhs);
}
template< class T, class Job, class Alloc >
bool operator>( const boost::asynchronous::vector<T,Job,Alloc>& lhs,
                const boost::asynchronous::vector<T,Job,Alloc>& rhs )
{
    return (lhs != rhs) && !(lhs < rhs);
}

}} // boost::async



#endif /* BOOST_ASYNCHRONOUS_CONTAINER_VECTOR_HPP */
