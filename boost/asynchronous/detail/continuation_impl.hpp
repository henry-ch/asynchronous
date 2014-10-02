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
#include <atomic>

#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/chrono.hpp>
#include <boost/thread/mutex.hpp>

#include <boost/asynchronous/post.hpp>
#include <boost/asynchronous/any_scheduler.hpp>
#include <boost/asynchronous/scheduler/detail/interrupt_state.hpp>
#include <boost/asynchronous/expected.hpp>
#include <boost/asynchronous/scheduler/tss_scheduler.hpp>

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
    // metafunction telling if we are future or expected based sind
    template <class T>
    struct continuation_args
    {
        typedef boost::future<T> type;
    };
    // workaround for compiler crash (gcc 4.7)
    boost::future<Return> get_continuation_args()const
    {
        return boost::future<Return>();
    }

    continuation(continuation&& rhs)noexcept
        : m_futures(std::move(rhs.m_futures))
        , m_ready_futures(std::move(rhs.m_ready_futures))
        , m_done(std::move(rhs.m_done))
        , m_state(std::move(rhs.m_state))
        , m_timeout(std::move(rhs.m_timeout))
        , m_start(std::move(rhs.m_start))
    {
    }
    continuation(continuation const& rhs)noexcept
        : m_futures(std::move((const_cast<continuation&>(rhs)).m_futures))
        , m_ready_futures(std::move((const_cast<continuation&>(rhs)).m_ready_futures))
        , m_done(std::move((const_cast<continuation&>(rhs)).m_done))
        , m_state(std::move((const_cast<continuation&>(rhs)).m_state))
        , m_timeout(std::move((const_cast<continuation&>(rhs)).m_timeout))
        , m_start(std::move((const_cast<continuation&>(rhs)).m_start))
    {
    }

    continuation& operator= (continuation&& rhs)noexcept
    {
        std::swap(m_futures,rhs.m_futures);
        std::swap(m_ready_futures,rhs.m_ready_futures);
        std::swap(m_done,rhs.m_done);
        std::swap(m_state,rhs.m_state);
        std::swap(m_timeout,rhs.m_timeout);
        std::swap(m_start,rhs.m_start);
        return *this;
    }
    continuation& operator= (continuation const& rhs)noexcept
    {
        std::swap(m_futures,(const_cast<continuation&>(rhs)).m_futures);
        std::swap(m_ready_futures,(const_cast<continuation&>(rhs)).m_ready_futures);
        std::swap(m_done,(const_cast<continuation&>(rhs)).m_done);
        std::swap(m_state,(const_cast<continuation&>(rhs)).m_state);
        std::swap(m_timeout,(const_cast<continuation&>(rhs)).m_timeout);
        std::swap(m_start,(const_cast<continuation&>(rhs)).m_start);
        return *this;
    }

    template <typename... Args>
    continuation(boost::shared_ptr<boost::asynchronous::detail::interrupt_state> state,Tuple t, Duration d, Args&&... args)
    : m_futures(std::move(t))
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
    continuation(boost::shared_ptr<boost::asynchronous::detail::interrupt_state> state,Tuple t, Duration d)
    : m_futures(std::move(t))
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
        m_done(std::move(m_futures));
    }

    template <class Func>
    void on_done(Func f)
    {
        m_done = std::move(f);
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
        check_ready(ready,index,m_futures,m_ready_futures);
        return ready;
    }
//TODO temporary
//private:
    Tuple m_futures;
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

template <class Return, typename Job, typename Tuple=std::tuple<boost::asynchronous::expected<Return> > , typename Duration = boost::chrono::milliseconds >
struct callback_continuation
{
    typedef int is_continuation_task;
    typedef int is_callback_continuation_task;
    typedef Return return_type;
    typedef Tuple tuple_type;
    // metafunction telling if we are future or expected based
    template <class T>
    struct continuation_args
    {
        typedef boost::asynchronous::expected<T> type;
    };
    // workaround for compiler crash (gcc 4.7)
    boost::asynchronous::expected<Return> get_continuation_args()const
    {
        return boost::asynchronous::expected<Return>();
    }

    callback_continuation(callback_continuation&& rhs)noexcept
        : m_state(std::move(rhs.m_state))
        , m_timeout(std::move(rhs.m_timeout))
        , m_start(std::move(rhs.m_start))
        , m_finished(rhs.m_finished)
    {
    }
    callback_continuation(callback_continuation const& rhs)noexcept
        : m_state(std::move((const_cast<callback_continuation&>(rhs)).m_state))
        , m_timeout(std::move((const_cast<callback_continuation&>(rhs)).m_timeout))
        , m_start(std::move((const_cast<callback_continuation&>(rhs)).m_start))
        , m_finished(rhs.m_finished)
    {
    }

    callback_continuation& operator= (callback_continuation&& rhs)noexcept
    {
        std::swap(m_state,rhs.m_state);
        std::swap(m_timeout,rhs.m_timeout);
        std::swap(m_start,rhs.m_start);
        m_finished=rhs.m_finished;
        return *this;
    }
    callback_continuation& operator= (callback_continuation const& rhs)noexcept
    {
        std::swap(m_state,(const_cast<callback_continuation&>(rhs)).m_state);
        std::swap(m_timeout,(const_cast<callback_continuation&>(rhs)).m_timeout);
        std::swap(m_start,(const_cast<callback_continuation&>(rhs)).m_start);
        m_finished=rhs.m_finished;
        return *this;
    }

    template <typename... Args>
    callback_continuation(boost::shared_ptr<boost::asynchronous::detail::interrupt_state> state,Tuple t, Duration d, Args&&... args)
    : m_state(state)
    , m_timeout(d)
    , m_finished(boost::make_shared<subtask_finished>(std::move(t),!!m_state,(m_timeout.count() != 0)))
    {
        // remember when we started
        m_start = boost::chrono::high_resolution_clock::now();

        boost::asynchronous::any_weak_scheduler<Job> weak_scheduler = boost::asynchronous::get_thread_scheduler<Job>();
        boost::asynchronous::any_shared_scheduler<Job> locked_scheduler = weak_scheduler.lock();
        std::vector<boost::asynchronous::any_interruptible> interruptibles;
        if (locked_scheduler.is_valid())
        {
            continuation_ctor_helper<0>
                (locked_scheduler,interruptibles,std::forward<Args>(args)...);
        }
        if (m_state)
            m_state->add_subs(interruptibles.begin(),interruptibles.end());
    }

    template <int I,typename T,typename Interruptibles,typename Last>
    void continuation_ctor_helper(T& sched, Interruptibles& interruptibles,Last&& l)
    {
        std::string n(std::move(l.get_name()));
        auto finished = m_finished;
        l.set_done_func([finished](boost::asynchronous::expected<typename Last::res_type> r)
                        {
                           std::get<I>((*finished).m_futures) = std::move(r);
                           finished->done();
                        });
        if (!m_state)
        {

            // we execute one task ourselves to save one post
            l();
        }
        else if(!m_state->is_interrupted())
        {
            // interruptible requested
            interruptibles.push_back(std::get<1>(
                                         boost::asynchronous::interruptible_post_future(sched,std::forward<Last>(l),n,
                                                                                        boost::asynchronous::get_own_queue_index<>())));
        }
    }

    template <int I,typename T,typename Interruptibles,typename... Tail, typename Front>
    void continuation_ctor_helper(T& sched, Interruptibles& interruptibles,Front&& front,Tail&&... tail)
    {
        auto finished = m_finished;
        front.set_done_func([finished](boost::asynchronous::expected<typename Front::res_type> r)
                            {
                               std::get<I>((*finished).m_futures) = std::move(r);
                               finished->done();
                            });
        std::string n(std::move(front.get_name()));
        if (!m_state)
        {
            // no interruptible requested
            boost::asynchronous::post_future(sched,std::forward<Front>(front),n,boost::asynchronous::get_own_queue_index<>());
        }
        else
        {
            interruptibles.push_back(std::get<1>(
                                         boost::asynchronous::interruptible_post_future(sched,std::forward<Front>(front),n,
                                                                                        boost::asynchronous::get_own_queue_index<>())));
        }
        continuation_ctor_helper<I+1>(sched,interruptibles,std::forward<Tail>(tail)...);
    }


    template <typename... Args>
    callback_continuation(boost::shared_ptr<boost::asynchronous::detail::interrupt_state> state,Tuple t, Duration d,
                          std::tuple<Args...> args)
    : m_state(state)
    , m_timeout(d)
    , m_finished(boost::make_shared<subtask_finished>(std::move(t),!!m_state,(m_timeout.count() != 0)))
    {
        // remember when we started
        m_start = boost::chrono::high_resolution_clock::now();

        boost::asynchronous::any_weak_scheduler<Job> weak_scheduler = boost::asynchronous::get_thread_scheduler<Job>();
        boost::asynchronous::any_shared_scheduler<Job> locked_scheduler = weak_scheduler.lock();
        std::vector<boost::asynchronous::any_interruptible> interruptibles;
        if (locked_scheduler.is_valid())
        {
            continuation_ctor_helper_tuple<0>
                (locked_scheduler,interruptibles,args);
        }
        if (m_state)
            m_state->add_subs(interruptibles.begin(),interruptibles.end());
    }
    template <int I,typename T,typename Interruptibles,typename... ArgsTuple>
    typename boost::enable_if_c<I == sizeof...(ArgsTuple)-1,void>::type
    continuation_ctor_helper_tuple(T& sched, Interruptibles& interruptibles,std::tuple<ArgsTuple...>& l)
    {
        std::string n(std::move(std::get<I>(l).get_name()));
        auto finished = m_finished;
        std::get<I>(l).set_done_func([finished](boost::asynchronous::expected<
                                                    typename std::tuple_element<I, std::tuple<ArgsTuple...>>::type::res_type> r)
                        {
                           std::get<I>((*finished).m_futures) = std::move(r);
                           finished->done();
                        });
        if (!m_state)
        {
            // we execute one task ourselves to save one post
            std::get<I>(l)();
        }
        else if(!m_state->is_interrupted())
        {
            // interruptible requested
            interruptibles.push_back(std::get<1>(
                                         boost::asynchronous::interruptible_post_future(sched,std::move(std::get<I>(l)),n,
                                                                                        boost::asynchronous::get_own_queue_index<>())));
        }
    }

    template <int I,typename T,typename Interruptibles,typename... ArgsTuple>
    typename boost::disable_if_c<I == sizeof...(ArgsTuple)-1,void>::type
    continuation_ctor_helper_tuple(T& sched, Interruptibles& interruptibles,std::tuple<ArgsTuple...>& front)
    {
        auto finished = m_finished;
        std::get<I>(front).set_done_func([finished](boost::asynchronous::expected<
                                                            typename std::tuple_element<I, std::tuple<ArgsTuple...>>::type::res_type> r)
                            {
                               std::get<I>((*finished).m_futures) = std::move(r);
                               finished->done();
                            });
        std::string n(std::move(std::get<I>(front).get_name()));
        if (!m_state)
        {
            // no interruptible requested
            boost::asynchronous::post_future(sched,std::move(std::get<I>(front)),n,boost::asynchronous::get_own_queue_index<>());
        }
        else
        {
            interruptibles.push_back(std::get<1>(
                                         boost::asynchronous::interruptible_post_future(sched,std::move(std::get<I>(front)),n,
                                                                                        boost::asynchronous::get_own_queue_index<>())));
        }
        continuation_ctor_helper_tuple<I+1>(sched,interruptibles,front);
    }
    void operator()()
    {
    }

    template <class Func>
    void on_done(Func f)
    {
        m_finished->on_done(std::move(f));
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
            m_finished->set_interrupted();
            return true;
        }
        return m_finished->is_ready();
    }
    // called each time a subtask gives us a ready future. When all are here, we are done
    struct subtask_finished
    {
        subtask_finished(Tuple t, bool interruptible, bool supports_timeout)
            :m_futures(std::move(t)),m_ready_futures(0),m_done(),m_interruptible(interruptible)
            ,m_interrupted(false),m_supports_timeout(supports_timeout){}
        void done()
        {
            if (++m_ready_futures == std::tuple_size<Tuple>::value)
            {
                if (!m_interrupted && m_done)
                {
                    return_result();
                }
            }
        }
        template <class Func>
        void on_done(Func f)
        {
            m_done = std::move(f);
            if (!m_interrupted && (m_ready_futures.load() == std::tuple_size<Tuple>::value))
            {
                 return_result();
            }
        }
        bool is_ready()
        {
            return (m_ready_futures == std::tuple_size<Tuple>::value);
        }
        void set_interrupted()
        {
            m_interrupted=true;
            return_result();
        }
        void return_result()
        {
            if (!m_interruptible && !m_supports_timeout)
            {
                // not interruptible => we need not fear a race coming form an interruption thread
                if (m_done)
                {
                    m_done(std::move(m_futures));
                    m_done= std::function<void(Tuple&&)>();
                }
            }
            else
            {
                boost::mutex::scoped_lock lock(m_mutex);
                if (m_done)
                {
                    m_done(std::move(m_futures));
                    m_done= std::function<void(Tuple&&)>();
                }
            }
        }

        Tuple m_futures;
        std::atomic<std::size_t> m_ready_futures;
        std::function<void(Tuple&&)> m_done;
        const bool m_interruptible;
        std::atomic<bool> m_interrupted;
        const bool m_supports_timeout;
        // protects m_done in case of an interruption
        mutable boost::mutex m_mutex;
    };

    boost::shared_ptr<boost::asynchronous::detail::interrupt_state> m_state;
    Duration m_timeout;
    typename boost::chrono::high_resolution_clock::time_point m_start;
    boost::shared_ptr<subtask_finished> m_finished;

};

// version for sequences of futures
template <class Return, typename Job, typename Seq, typename Duration = boost::chrono::milliseconds>
struct continuation_as_seq
{
    typedef int is_continuation_task;
    typedef Return return_type;
    // metafunction telling if we are future or expected based sind
    template <class T>
    struct continuation_args
    {
        typedef boost::future<T> type;
    };
    // workaround for compiler crash (gcc 4.7)
    boost::future<Return> get_continuation_args()const
    {
        return boost::future<Return>();
    }

    continuation_as_seq(boost::shared_ptr<boost::asynchronous::detail::interrupt_state> state, Duration d,Seq seq)
    : m_futures(std::move(seq))
    , m_ready_futures(m_futures.size(),false)
    , m_state(state)
    , m_timeout(d)
    {
        // remember when we started
        m_start = boost::chrono::high_resolution_clock::now();
        //TODO interruptible
    }
    continuation_as_seq(continuation_as_seq&& rhs)noexcept
        : m_futures(std::move(rhs.m_futures))
        , m_ready_futures(std::move(rhs.m_ready_futures))
        , m_done(std::move(rhs.m_done))
        , m_state(std::move(rhs.m_state))
        , m_timeout(std::move(rhs.m_timeout))
        , m_start(std::move(rhs.m_start))
    {
    }
    continuation_as_seq(continuation_as_seq const& rhs)noexcept
        : m_futures(std::move((const_cast<continuation_as_seq&>(rhs)).m_futures))
        , m_ready_futures(std::move((const_cast<continuation_as_seq&>(rhs)).m_ready_futures))
        , m_done(std::move((const_cast<continuation_as_seq&>(rhs)).m_done))
        , m_state(std::move((const_cast<continuation_as_seq&>(rhs)).m_state))
        , m_timeout(std::move((const_cast<continuation_as_seq&>(rhs)).m_timeout))
        , m_start(std::move((const_cast<continuation_as_seq&>(rhs)).m_start))
    {
    }

    continuation_as_seq& operator= (continuation_as_seq&& rhs)noexcept
    {
        std::swap(m_futures,rhs.m_futures);
        std::swap(m_ready_futures,rhs.m_ready_futures);
        std::swap(m_done,rhs.m_done);
        std::swap(m_state,rhs.m_state);
        std::swap(m_timeout,rhs.m_timeout);
        std::swap(m_start,rhs.m_start);
        return *this;
    }
    continuation_as_seq& operator= (continuation_as_seq const& rhs)noexcept
    {
        std::swap(m_futures,(const_cast<continuation_as_seq&>(rhs)).m_futures);
        std::swap(m_ready_futures,(const_cast<continuation_as_seq&>(rhs)).m_ready_futures);
        std::swap(m_done,(const_cast<continuation_as_seq&>(rhs)).m_done);
        std::swap(m_state,(const_cast<continuation_as_seq&>(rhs)).m_state);
        std::swap(m_timeout,(const_cast<continuation_as_seq&>(rhs)).m_timeout);
        std::swap(m_start,(const_cast<continuation_as_seq&>(rhs)).m_start);
        return *this;
    }
    void operator()()
    {
        m_done(std::move(m_futures));
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
        for (typename Seq::const_iterator it = m_futures.begin(); it != m_futures.end() ;++it,++index)
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
    Seq m_futures;
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
auto make_future_tuple(Args&... args) -> decltype(std::make_tuple(call_get_future(args)...))
{
    return std::make_tuple(call_get_future(args)...);
}
template <typename T>
auto call_get_expected(T& ) -> boost::asynchronous::expected<typename T::res_type>
{
    // TODO not with empty ctor
    return boost::asynchronous::expected<typename T::res_type>();
}

//create a tuple of futures
template <typename... Args>
auto make_expected_tuple(Args&... args) -> decltype(std::make_tuple(call_get_expected(args)...))
{
    return std::make_tuple(call_get_expected(args)...);
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
