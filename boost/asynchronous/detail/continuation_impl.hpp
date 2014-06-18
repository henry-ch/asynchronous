// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2013
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNCHRONOUS_CONTINUATION_IMPL_HPP
#define BOOST_ASYNCHRONOUS_CONTINUATION_IMPL_HPP

#include <vector>
#include <tuple>
#include <utility>

#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/chrono.hpp>

#include <boost/asynchronous/post.hpp>
#include <boost/asynchronous/any_scheduler.hpp>
#include <boost/asynchronous/scheduler/detail/interrupt_state.hpp>

BOOST_MPL_HAS_XXX_TRAIT_DEF(state)
BOOST_MPL_HAS_XXX_TRAIT_DEF(iterator)

namespace boost { namespace asynchronous { namespace detail {

// the continuation task implementation
template <class Return, typename Job, typename Tuple=std::tuple<boost::future<Return> > , typename Duration = boost::chrono::milliseconds >
struct continuation
{
    typedef int is_continuation_task;
    typedef Return return_type;
    typedef Tuple tuple_type;

    continuation(continuation&&)=default;
    continuation(continuation const&)=default;
    template <typename... Args>
    continuation(boost::shared_ptr<boost::asynchronous::detail::interrupt_state> state,boost::shared_ptr<Tuple> t, Duration d, Args&&... args)
    : m_futures(t)
    , m_ready_futures(std::tuple_size<Tuple>::value,false)
    , m_state(state)
    , m_timeout(d)
    {
        // remember when we started
        m_start = boost::chrono::high_resolution_clock::now();

        boost::asynchronous::any_weak_scheduler<Job> weak_scheduler = boost::asynchronous::get_thread_scheduler<Job>();
        boost::asynchronous::any_shared_scheduler<Job> locked_scheduler = weak_scheduler.lock();
        std::vector<boost::asynchronous::any_interruptible> interruptibles;
        if (locked_scheduler.is_valid())
        {
            continuation_ctor_helper(locked_scheduler,interruptibles,std::forward<Args>(args)...);
        }
        if (m_state)
            m_state->add_subs(interruptibles.begin(),interruptibles.end());
    }
    // version where we already get futures
    continuation(boost::shared_ptr<boost::asynchronous::detail::interrupt_state> state,boost::shared_ptr<Tuple> t, Duration d)
    : m_futures(t)
    , m_ready_futures(std::tuple_size<Tuple>::value,false)
    , m_state(state)
    , m_timeout(d)
    {
        // remember when we started
        m_start = boost::chrono::high_resolution_clock::now();
        //TODO interruptible
    }
    template <typename T,typename Interruptibles,typename Last>
    void continuation_ctor_helper(T& sched, Interruptibles& interruptibles,Last&& l)
    {
        std::string n(std::move(l.get_name()));
        if (!m_state)
        {
            // no interruptible requested
            boost::asynchronous::post_future(sched,std::forward<Last>(l),n);
        }
        else if(!m_state->is_interrupted())
        {
            // interruptible requested
            interruptibles.push_back(std::get<1>(boost::asynchronous::interruptible_post_future(sched,std::forward<Last>(l),n)));
        }
    }

    template <typename T,typename Interruptibles,typename... Tail, typename Front>
    void continuation_ctor_helper(T& sched, Interruptibles& interruptibles,Front&& front,Tail&&... tail)
    {
        std::string n(std::move(front.get_name()));
        if (!m_state)
        {
            // no interruptible requested
            boost::asynchronous::post_future(sched,std::forward<Front>(front),n);
        }
        else
        {
            interruptibles.push_back(std::get<1>(boost::asynchronous::interruptible_post_future(sched,std::forward<Front>(front),n)));
        }
        continuation_ctor_helper(sched,interruptibles,std::forward<Tail>(tail)...);
    }

    void operator()()
    {
        m_done(std::move(*m_futures));
    }

    template <class Func>
    void on_done(Func f)
    {
        m_done = f;
    }
    bool is_ready()
    {
        if (!!m_state && m_state->is_interrupted())
        {
            // we are interrupted => we are ready
            return true;
        }
        // if timeout, we are ready too
        typename boost::chrono::high_resolution_clock::time_point time_now = boost::chrono::high_resolution_clock::now();
        if (m_timeout.count() != 0 && (time_now - m_start >= m_timeout))
        {
            return true;
        }

        bool ready=true;
        std::size_t index = 0;
        check_ready(ready,index,*m_futures,m_ready_futures);
        return ready;
    }
//TODO temporary
//private:
    boost::shared_ptr<Tuple> m_futures;
    std::vector<bool> m_ready_futures;
    std::function<void(Tuple&&)> m_done;
    boost::shared_ptr<boost::asynchronous::detail::interrupt_state> m_state;
    Duration m_timeout;
    typename boost::chrono::high_resolution_clock::time_point m_start;

    template<std::size_t I = 0, typename... Tp>
    inline typename std::enable_if<I == sizeof...(Tp), void>::type
    check_ready(bool& ,std::size_t&, std::tuple<Tp...> const& , std::vector<bool>& )const
    { }

    template<std::size_t I = 0, typename... Tp>
    inline typename std::enable_if<I < sizeof...(Tp), void>::type
    check_ready(bool& ready,std::size_t& index,std::tuple<Tp...> const& t,std::vector<bool>& ready_futures)const
    {
        if (ready_futures[index])
        {
            ++index;
            check_ready<I + 1, Tp...>(ready,index,t,ready_futures);
        }
        else if (((std::get<I>(t)).has_value() || (std::get<I>(t)).has_exception()) )
        {
            ready_futures[index]=true;
            ++index;
            check_ready<I + 1, Tp...>(ready,index,t,ready_futures);
        }
        else
        {
            ready=false;
            return;
        }
    }

};

// version for sequences of futures
template <class Return, typename Job, typename Seq, typename Duration = boost::chrono::milliseconds>
struct continuation_as_seq
{
    typedef int is_continuation_task;
    typedef Return return_type;

    continuation_as_seq(boost::shared_ptr<boost::asynchronous::detail::interrupt_state> state, Duration d,Seq&& seq)
    : m_futures(new Seq(std::forward<Seq>(seq)))
    , m_ready_futures(m_futures->size(),false)
    , m_state(state)
    , m_timeout(d)
    {
        // remember when we started
        m_start = boost::chrono::high_resolution_clock::now();
        //TODO interruptible
    }

    void operator()()
    {
        m_done(std::move(*m_futures));
    }

    template <class Func>
    void on_done(Func f)
    {
        m_done = f;
    }
    bool is_ready()
    {
        if (!!m_state && m_state->is_interrupted())
        {
            // we are interrupted => we are ready
            return true;
        }
        // if timeout, we are ready too
        typename boost::chrono::high_resolution_clock::time_point time_now = boost::chrono::high_resolution_clock::now();
        if (m_timeout.count() != 0 && (time_now - m_start >= m_timeout))
        {
            return true;
        }
        std::size_t index = 0;
        for (typename Seq::const_iterator it = m_futures->begin(); it != m_futures->end() ;++it,++index)
        {
            if (m_ready_futures[index])
                continue;
            else if (((*it).is_ready() || (*it).has_exception() ) )
                m_ready_futures[index]=true;
            else return false;
        }
        return true;
    }
//TODO temporary
//private:
    boost::shared_ptr<Seq> m_futures;
    std::vector<bool> m_ready_futures;
    std::function<void(Seq&&)> m_done;
    boost::shared_ptr<boost::asynchronous::detail::interrupt_state> m_state;
    Duration m_timeout;
    typename boost::chrono::high_resolution_clock::time_point m_start;
};

// used by variadic make_future_tuple
template <typename T>
auto call_get_future(T& t) -> decltype(t.get_future())
{
    return t.get_future();
}

//create a tuple of futures
template <typename... Args>
auto make_future_tuple(Args&... args) -> boost::shared_ptr<decltype(std::make_tuple(call_get_future(args)...))>
{
    boost::shared_ptr<decltype(std::make_tuple(call_get_future(args)...))> sp =
            boost::make_shared<decltype(std::make_tuple(call_get_future(args)...))>
                 (std::make_tuple(call_get_future(args)...));
    return sp;
}

template <typename Front,typename... Tail>
struct has_future_args_helper
{
    typedef typename has_state<Front>::type type;
};

template <typename... Args>
struct has_future_args
{
    typedef typename has_future_args_helper<Args...>::type type;
};

template <typename Front,typename... Tail>
struct has_iterator_args_helper
{
    typedef typename has_iterator<Front>::type type;
};

template <typename... Args>
struct has_iterator_args
{
    typedef typename has_iterator_args_helper<Args...>::type type;
};

}}}

#endif // BOOST_ASYNCHRONOUS_CONTINUATION_IMPL_HPP
