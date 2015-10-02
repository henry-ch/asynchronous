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

#include <boost/asynchronous/callable_any.hpp>
#include <boost/asynchronous/post.hpp>
#include <boost/asynchronous/any_scheduler.hpp>
#include <boost/asynchronous/scheduler/detail/interrupt_state.hpp>
#include <boost/asynchronous/expected.hpp>
#include <boost/asynchronous/scheduler/tss_scheduler.hpp>

BOOST_MPL_HAS_XXX_TRAIT_DEF(state)
BOOST_MPL_HAS_XXX_TRAIT_DEF(iterator)

namespace boost { namespace asynchronous {

// what has to be set when a task is ready
template <class Return>
struct continuation_result
{
public:
    typedef Return return_type;
    continuation_result(boost::shared_ptr<boost::promise<Return> > p,std::function<void(boost::asynchronous::expected<Return>)> f)
        : m_promise(p),m_done_func(f){}
    continuation_result(continuation_result&& rhs)noexcept
        : m_promise(std::move(rhs.m_promise))
        , m_done_func(std::move(rhs.m_done_func))
    {}
    continuation_result(continuation_result const& rhs)noexcept
        : m_promise(rhs.m_promise)
        , m_done_func(rhs.m_done_func)
    {}
    continuation_result& operator= (continuation_result&& rhs)noexcept
    {
        std::swap(m_promise,rhs.m_promise);
        std::swap(m_done_func,rhs.m_done_func);
        return *this;
    }
    continuation_result& operator= (continuation_result const& rhs)noexcept
    {
        m_promise = rhs.m_promise;
        m_done_func = rhs.m_done_func;
        return *this;
    }
    void set_value(Return val)const
    {
        // inform caller if any
        if (m_done_func)
        {
            (m_done_func)(boost::asynchronous::expected<Return>(std::move(val)));
        }
        else
        {
            m_promise->set_value(std::move(val));
        }
    }
    void set_exception(boost::exception_ptr p)const
    {
        // inform caller if any
        if (m_done_func)
        {
            (m_done_func)(boost::asynchronous::expected<Return>(p));
        }
        else
        {
           m_promise->set_exception(p);
        }
    }

private:
    boost::shared_ptr<boost::promise<Return> > m_promise;
    std::function<void(boost::asynchronous::expected<Return>)> m_done_func;
};
template <>
struct continuation_result<void>
{
public:
    typedef void return_type;
    continuation_result(boost::shared_ptr<boost::promise<void> > p,std::function<void(boost::asynchronous::expected<void>)> f)
        :m_promise(p),m_done_func(f){}
    continuation_result(continuation_result&& rhs)noexcept
        : m_promise(std::move(rhs.m_promise))
        , m_done_func(std::move(rhs.m_done_func))
    {}
    continuation_result(continuation_result const& rhs)noexcept
        : m_promise(rhs.m_promise)
        , m_done_func(rhs.m_done_func)
    {}
    continuation_result& operator= (continuation_result&& rhs)noexcept
    {
        std::swap(m_promise,rhs.m_promise);
        std::swap(m_done_func,rhs.m_done_func);
        return *this;
    }
    continuation_result& operator= (continuation_result const& rhs)noexcept
    {
        m_promise = rhs.m_promise;
        m_done_func = rhs.m_done_func;
        return *this;
    }
    void set_value()const
    {
        // inform caller if any
        if (m_done_func)
        {
            (m_done_func)(boost::asynchronous::expected<void>());
        }
        else
        {
            m_promise->set_value();
        }
    }
    void set_exception(boost::exception_ptr p)const
    {

        // inform caller if any
        if (m_done_func)
        {
            (m_done_func)(boost::asynchronous::expected<void>(p));
        }
        else
        {
            m_promise->set_exception(p);
        }
    }

private:
    boost::shared_ptr<boost::promise<void> > m_promise;
    std::function<void(boost::asynchronous::expected<void>)> m_done_func;
};

// the base class of continuation tasks Provides typedefs and hides promise.
template <class Return>
struct continuation_task
{
public:
    typedef Return return_type;

#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
    continuation_task(const std::string& name):m_promise(),m_name(name){}
#else
    continuation_task(const std::string& name=""):m_promise(),m_name(name){}
#endif

    continuation_task(continuation_task&& rhs)noexcept
        : m_promise(std::move(rhs.m_promise))
        , m_name(std::move(rhs.m_name))
        , m_done_func(std::move(rhs.m_done_func))
    {}
    continuation_task(continuation_task const& rhs)noexcept
        : m_promise(rhs.m_promise)
        , m_name(rhs.m_name)
        , m_done_func(rhs.m_done_func)
    {}
    continuation_task& operator= (continuation_task&& rhs)noexcept
    {
        std::swap(m_promise,rhs.m_promise);
        std::swap(m_name,rhs.m_name);
        std::swap(m_done_func,rhs.m_done_func);
        return *this;
    }
    continuation_task& operator= (continuation_task const& rhs)noexcept
    {
        m_promise = rhs.m_promise;
        m_name = rhs.m_name;
        m_done_func = rhs.m_done_func;
        return *this;
    }

    boost::future<Return> get_future()const
    {
        // create only when asked
        if (!m_promise)
            const_cast<continuation_task<Return>&>(*this).m_promise = boost::make_shared<boost::promise<Return>>();
        return m_promise->get_future();
    }

    boost::shared_ptr<boost::promise<Return> > get_promise()const
    {
        // create only when asked
        if (!m_promise)
            const_cast<continuation_task<Return>&>(*this).m_promise = boost::make_shared<boost::promise<Return>>();
        return m_promise;
    }
    std::string get_name()const
    {
        return m_name;
    }

    boost::asynchronous::continuation_result<Return> this_task_result()const
    {
        return continuation_result<Return>(m_promise,m_done_func);
    }
    // called in case task is stolen by some client and only the result is returned
    template <class Archive,class InternalArchive>
    void as_result(Archive & ar, const unsigned int /*version*/)
    {
        boost::asynchronous::tcp::client_request::message_payload payload;
        ar >> payload;
        if (!payload.m_has_exception)
        {
            std::istringstream archive_stream(payload.m_data);
            InternalArchive archive(archive_stream);

            Return res;
            archive >> res;
            if (m_done_func)
                (m_done_func)(boost::asynchronous::expected<Return>(std::move(res)));
            else
                get_promise()->set_value(res);
        }
        else
        {
            if (m_done_func)
                (m_done_func)(boost::asynchronous::expected<Return>(boost::copy_exception(payload.m_exception)));
            else
                get_promise()->set_exception(boost::copy_exception(payload.m_exception));
        }
    }
    void set_done_func(std::function<void(boost::asynchronous::expected<Return>)> f)
    {
        m_done_func=std::move(f);
    }
private:
    boost::shared_ptr<boost::promise<Return> > m_promise;
    std::string m_name;
    std::function<void(boost::asynchronous::expected<Return>)> m_done_func;
};
template <>
struct continuation_task<void>
{
public:
    typedef void return_type;

    continuation_task(continuation_task&& rhs)noexcept
        : m_promise(std::move(rhs.m_promise))
        , m_name(std::move(rhs.m_name))
        , m_done_func(std::move(rhs.m_done_func))
    {}
    continuation_task(continuation_task const& rhs)noexcept
        : m_promise(rhs.m_promise)
        , m_name(rhs.m_name)
        , m_done_func(rhs.m_done_func)
    {}
    continuation_task& operator= (continuation_task&& rhs)noexcept
    {
        std::swap(m_promise,rhs.m_promise);
        std::swap(m_name,rhs.m_name);
        std::swap(m_done_func,rhs.m_done_func);
        return *this;
    }
    continuation_task& operator= (continuation_task const& rhs)noexcept
    {
        m_promise = rhs.m_promise;
        m_name = rhs.m_name;
        m_done_func = rhs.m_done_func;
        return *this;
    }
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
    continuation_task(const std::string& name):m_promise(),m_name(name){}
#else
    continuation_task(const std::string& name=""):m_promise(),m_name(name){}
#endif

    boost::future<void> get_future()const
    {
        // create only when asked
        if (!m_promise)
            const_cast<continuation_task<void>&>(*this).m_promise = boost::make_shared<boost::promise<void>>();
        return m_promise->get_future();
    }

    boost::shared_ptr<boost::promise<void> > get_promise()const
    {
        // create only when asked
        if (!m_promise)
            const_cast<continuation_task<void>&>(*this).m_promise = boost::make_shared<boost::promise<void>>();
        return m_promise;
    }
    std::string get_name()const
    {
        return m_name;
    }

    boost::asynchronous::continuation_result<void> this_task_result()const
    {
        return continuation_result<void>(m_promise,m_done_func);
    }
    // called in case task is stolen by some client and only the result is returned
    template <class Archive,class InternalArchive>
    void as_result(Archive & ar, const unsigned int /*version*/)
    {
        boost::asynchronous::tcp::client_request::message_payload payload;
        ar >> payload;
        if (!payload.m_has_exception)
        {
            if (m_done_func)
                (m_done_func)(boost::asynchronous::expected<void>());
            else
                get_promise()->set_value();
        }
        else
        {
            if (m_done_func)
                (m_done_func)(boost::asynchronous::expected<void>(boost::copy_exception(payload.m_exception)));
            else
                get_promise()->set_exception(boost::copy_exception(payload.m_exception));
        }
    }
    void set_done_func(std::function<void(boost::asynchronous::expected<void>)> f)
    {
        m_done_func=std::move(f);
    }

private:
    boost::shared_ptr<boost::promise<void> > m_promise;
    std::string m_name;
    std::function<void(boost::asynchronous::expected<void>)> m_done_func;
};

template <class Func, class ReturnType>
struct lambda_continuation_wrapper : public boost::asynchronous::continuation_task<ReturnType>
{
    lambda_continuation_wrapper(Func f, std::string const& name)
        : boost::asynchronous::continuation_task<ReturnType>(name)
        , myFunc(std::move(f))
    {}
    void operator()()
    {
        boost::asynchronous::continuation_result<ReturnType> task_res = this->this_task_result();
        try
        {
            task_res.set_value(myFunc());
        }
        catch (std::exception& e)
        {
            task_res.set_exception(boost::copy_exception(e));
        }
    }

    Func myFunc;
};
template <class Func>
struct lambda_continuation_wrapper<Func,void> : public boost::asynchronous::continuation_task<void>
{
    lambda_continuation_wrapper(Func f, std::string const& name)
        : boost::asynchronous::continuation_task<void>(name)
        , myFunc(std::move(f))
    {}
    void operator()()
    {
        boost::asynchronous::continuation_result<void> task_res = this->this_task_result();
        try
        {
            myFunc();
            task_res.set_value();
        }
        catch (std::exception& e)
        {
            task_res.set_exception(boost::copy_exception(e));
        }
    }

    Func myFunc;
};

template <class Func>
auto make_lambda_continuation_wrapper(Func f, std::string const& name) -> lambda_continuation_wrapper<Func,decltype(f())>
{
    return lambda_continuation_wrapper<Func,decltype(f())>(std::move(f),name);
}

namespace detail {

// the continuation task implementation
template <class Return, typename Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB, typename Tuple=std::tuple<boost::future<Return> > , typename Duration = boost::chrono::milliseconds >
struct continuation
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
        // TODO what if not valid?
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
            boost::asynchronous::post_future(sched,std::forward<Last>(l),n,boost::asynchronous::get_own_queue_index<>());
        }
        else if(!m_state->is_interrupted())
        {
            // interruptible requested
            interruptibles.push_back(std::get<1>(boost::asynchronous::interruptible_post_future(sched,std::forward<Last>(l),n,boost::asynchronous::get_own_queue_index<>())));
        }
    }

    template <typename T,typename Interruptibles,typename... Tail, typename Front>
    void continuation_ctor_helper(T& sched, Interruptibles& interruptibles,Front&& front,Tail&&... tail)
    {
        std::string n(std::move(front.get_name()));
        if (!m_state)
        {
            // no interruptible requested
            boost::asynchronous::post_future(sched,std::forward<Front>(front),n,boost::asynchronous::get_own_queue_index<>());
        }
        else
        {
            interruptibles.push_back(std::get<1>(boost::asynchronous::interruptible_post_future(sched,std::forward<Front>(front),n,boost::asynchronous::get_own_queue_index<>())));
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
    std::function<void(Tuple)> m_done;
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

template <class Return, typename Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB, typename Tuple=std::tuple<boost::asynchronous::expected<Return> > , typename Duration = boost::chrono::milliseconds >
struct callback_continuation
{
    typedef int is_continuation_task;
    typedef int is_callback_continuation_task;
    typedef Return return_type;
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

    template <typename Func, typename... Args>
    callback_continuation(boost::shared_ptr<boost::asynchronous::detail::interrupt_state> state,Tuple t, Duration d, Func f,
                          Args&&... args)
    : m_state(state)
    , m_timeout(d)
    , m_finished(boost::make_shared<subtask_finished>(std::move(t),!!m_state,(m_timeout.count() != 0)))
    {
        // remember when we started
        m_start = boost::chrono::high_resolution_clock::now();

        on_done(std::move(f));

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
    // version where done functor is set later
    template <typename... Args>
    callback_continuation(boost::shared_ptr<boost::asynchronous::detail::interrupt_state> state,Tuple t, Duration d, bool,
                          Args&&... args)
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
    template <typename Func,typename... Args>
    callback_continuation(boost::shared_ptr<boost::asynchronous::detail::interrupt_state> state,Tuple t, Duration d,Func f,
                          std::tuple<Args...> args)
    : m_state(state)
    , m_timeout(d)
    , m_finished(boost::make_shared<subtask_finished>(std::move(t),!!m_state,(m_timeout.count() != 0)))
    {
        // remember when we started
        m_start = boost::chrono::high_resolution_clock::now();

        on_done(std::move(f));

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
    // handles the difference between normal tasks and continuations given as normal tasks
    template <int I,class Task,class Enable=void>
    struct task_type_selector
    {
        template <typename S,typename Interruptibles,typename F>
        static void construct(S& sched,Interruptibles& interruptibles,F& func,
                              boost::shared_ptr<boost::asynchronous::detail::interrupt_state> state,bool last_task, Task&& t)
        {
            std::string n(std::move(t.get_name()));
            auto finished = func;
            t.set_done_func([finished](boost::asynchronous::expected<typename Task::return_type> r)
                            {
                               std::get<I>((*finished).m_futures) = std::move(r);
                               finished->done();
                            });
            if (!state)
            {
                if (last_task)
                {
                    // we execute one task ourselves to save one post
                    try
                    {
                        t();
                    }
                    catch(std::exception& e)
                    {
                        boost::asynchronous::expected<typename Task::return_type> r;
                        r.set_exception(boost::copy_exception(e));
                        std::get<I>((*finished).m_futures) = std::move(r);
                        finished->done();
                    }
                }
                else
                {
                    // no interruptible requested
                    boost::asynchronous::post_future(sched,std::forward<Task>(t),n,boost::asynchronous::get_own_queue_index<>());
                }
            }
            else if(!state->is_interrupted())
            {
                // interruptible requested
                interruptibles.push_back(std::get<1>(
                                             boost::asynchronous::interruptible_post_future(sched,std::forward<Task>(t),n,
                                                                                            boost::asynchronous::get_own_queue_index<>())));
            }
        }
    };
    template<int I,class Task>
    struct task_type_selector<I,Task,typename ::boost::enable_if<has_is_callback_continuation_task<Task> >::type>
    {
        template <typename S,typename Interruptibles,typename F>
        static void construct(S& ,Interruptibles&,F& func, boost::shared_ptr<boost::asynchronous::detail::interrupt_state>,bool, Task&& t)
        {
            auto finished = func;
            t.on_done([finished](std::tuple<boost::asynchronous::expected<typename Task::return_type>>&& r)
                            {
                               std::get<I>((*finished).m_futures) = std::move(std::get<0>(r));
                               finished->done();
                            });
        }
    };

    template <int I,typename T,typename Interruptibles,typename Last>
    void continuation_ctor_helper(T& sched, Interruptibles& interruptibles,Last&& l)
    {
        task_type_selector<I,Last>::construct(sched,interruptibles,m_finished,m_state,true,std::forward<Last>(l));
    }

    template <int I,typename T,typename Interruptibles,typename... Tail, typename Front>
    void continuation_ctor_helper(T& sched, Interruptibles& interruptibles,Front&& front,Tail&&... tail)
    {
        task_type_selector<I,Front>::construct(sched,interruptibles,m_finished,m_state,false,std::forward<Front>(front));
        continuation_ctor_helper<I+1>(sched,interruptibles,std::forward<Tail>(tail)...);
    }

    template <int I,typename T,typename Interruptibles,typename... ArgsTuple>
    typename boost::enable_if_c<I == sizeof...(ArgsTuple)-1,void>::type
    continuation_ctor_helper_tuple(T& sched, Interruptibles& interruptibles,std::tuple<ArgsTuple...>& l)
    {
        std::string n(std::move(std::get<I>(l).get_name()));
        auto finished = m_finished;
        std::get<I>(l).set_done_func([finished](boost::asynchronous::expected<
                                                    typename std::tuple_element<I, std::tuple<ArgsTuple...>>::type::return_type> r)
                        {
                           std::get<I>((*finished).m_futures) = std::move(r);
                           finished->done();
                        });
        if (!m_state)
        {
            // we execute one task ourselves to save one post
            try
            {
                std::get<I>(l)();
            }
            catch(std::exception& e)
            {
                boost::asynchronous::expected<typename std::tuple_element<I, std::tuple<ArgsTuple...>>::type::return_type> r;
                r.set_exception(boost::copy_exception(e));
                std::get<I>((*finished).m_futures) = std::move(r);
                finished->done();
            }
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
                                                            typename std::tuple_element<I, std::tuple<ArgsTuple...>>::type::return_type> r)
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
            ,m_interrupted(false),m_supports_timeout(supports_timeout)
        {
        }
        void done()
        {
            if (++m_ready_futures == (std::tuple_size<Tuple>::value + 1))
            {
                if (!m_interrupted)
                {
                    return_result();
                }
            }
        }
        template <class Func>
        void on_done(Func f)
        {
            if (m_interruptible || m_supports_timeout)
            {
                bool interrupted = false;
                {
                    boost::mutex::scoped_lock lock(m_mutex);
                    ++m_ready_futures;
                    m_done = std::move(f);
                    interrupted = m_interrupted;
                }
                if (interrupted)
                    return_result();
                else if (m_ready_futures ==(std::tuple_size<Tuple>::value + 1))
                    return_result();
            }
            else
            {
                m_done = std::move(f);
                if (++m_ready_futures == (std::tuple_size<Tuple>::value + 1))
                {
                    if (!m_interrupted)
                    {
                        return_result();
                    }
                }
            }
        }
        bool is_ready()
        {
            return (m_ready_futures == (std::tuple_size<Tuple>::value+1));
        }
        void set_interrupted()
        {
            bool done_fct_set=false;
            {
                boost::mutex::scoped_lock lock(m_mutex);
                m_interrupted=true;
                done_fct_set = !!m_done;
            }
            if (done_fct_set)
                return_result();
        }
        void return_result()
        {
            m_done(std::move(m_futures));
        }

        Tuple m_futures;
        std::atomic<std::size_t> m_ready_futures;
        std::function<void(Tuple)> m_done;
        const bool m_interruptible;
        bool m_interrupted;
        const bool m_supports_timeout;
        // only needed in case interrupt capability is needed
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

template <class Return, typename Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB, typename Duration = boost::chrono::milliseconds >
struct callback_continuation_as_seq
{
    typedef int is_continuation_task;
    typedef int is_callback_continuation_task;
    typedef Return return_type;
    // metafunction telling if we are future or expected based
    template <class T>
    struct continuation_args
    {
        typedef boost::asynchronous::expected<T> type;
    };
    // wrapper in case we only get a callback_continuation
    template <class Continuation>
    struct callback_continuation_wrapper : public boost::asynchronous::continuation_task<Return>
    {
        callback_continuation_wrapper(Continuation cont)
        : boost::asynchronous::continuation_task<Return>("callback_continuation_wrapper")
        , m_cont(std::move(cont))
        {}
        void operator()()
        {
            boost::asynchronous::continuation_result<Return> task_res = this->this_task_result();
            m_cont.on_done([task_res](std::tuple<boost::asynchronous::expected<Return> >&& res)
            {
                try
                {
                    task_res.set_value(std::move(std::get<0>(res).get()));
                }
                catch(std::exception& e)
                {
                    task_res.set_exception(boost::copy_exception(e));
                }
            });
        }
        Continuation m_cont;
    };

    // workaround for compiler crash (gcc 4.7)
    boost::asynchronous::expected<Return> get_continuation_args()const
    {
        return boost::asynchronous::expected<Return>();
    }

    callback_continuation_as_seq(callback_continuation_as_seq&& rhs)noexcept
        : m_state(std::move(rhs.m_state))
        , m_timeout(std::move(rhs.m_timeout))
        , m_start(std::move(rhs.m_start))
        , m_finished(rhs.m_finished)
    {
    }
    callback_continuation_as_seq(callback_continuation_as_seq const& rhs)noexcept
        : m_state(std::move((const_cast<callback_continuation_as_seq&>(rhs)).m_state))
        , m_timeout(std::move((const_cast<callback_continuation_as_seq&>(rhs)).m_timeout))
        , m_start(std::move((const_cast<callback_continuation_as_seq&>(rhs)).m_start))
        , m_finished(rhs.m_finished)
    {
    }

    callback_continuation_as_seq& operator= (callback_continuation_as_seq&& rhs)noexcept
    {
        std::swap(m_state,rhs.m_state);
        std::swap(m_timeout,rhs.m_timeout);
        std::swap(m_start,rhs.m_start);
        m_finished=rhs.m_finished;
        return *this;
    }
    callback_continuation_as_seq& operator= (callback_continuation_as_seq const& rhs)noexcept
    {
        std::swap(m_state,(const_cast<callback_continuation_as_seq&>(rhs)).m_state);
        std::swap(m_timeout,(const_cast<callback_continuation_as_seq&>(rhs)).m_timeout);
        std::swap(m_start,(const_cast<callback_continuation_as_seq&>(rhs)).m_start);
        m_finished=rhs.m_finished;
        return *this;
    }

    template <typename Func,typename Args>
    callback_continuation_as_seq(boost::shared_ptr<boost::asynchronous::detail::interrupt_state> state, Duration d,Func f,
                          std::vector<Args> args)
    : m_state(state)
    , m_timeout(d)
    , m_finished(boost::make_shared<subtask_finished>(args.size(),!!m_state,(m_timeout.count() != 0)))
    {
        // remember when we started
        m_start = boost::chrono::high_resolution_clock::now();

        on_done(std::move(f));

        boost::asynchronous::any_weak_scheduler<Job> weak_scheduler = boost::asynchronous::get_thread_scheduler<Job>();
        boost::asynchronous::any_shared_scheduler<Job> locked_scheduler = weak_scheduler.lock();
        std::vector<boost::asynchronous::any_interruptible> interruptibles;
        if (locked_scheduler.is_valid())
        {
            continuation_ctor_helper(locked_scheduler,interruptibles,args);
        }
        if (m_state)
            m_state->add_subs(interruptibles.begin(),interruptibles.end());
    }

    template <typename Func>
    callback_continuation_as_seq(boost::shared_ptr<boost::asynchronous::detail::interrupt_state> state, Duration d,Func f,
                          std::vector<boost::asynchronous::detail::callback_continuation<Return,Job>> args)
    : m_state(state)
    , m_timeout(d)
    , m_finished(boost::make_shared<subtask_finished>(args.size(),!!m_state,(m_timeout.count() != 0)))
    {
        // remember when we started
        m_start = boost::chrono::high_resolution_clock::now();

        on_done(std::move(f));

        boost::asynchronous::any_weak_scheduler<Job> weak_scheduler = boost::asynchronous::get_thread_scheduler<Job>();
        boost::asynchronous::any_shared_scheduler<Job> locked_scheduler = weak_scheduler.lock();
        std::vector<boost::asynchronous::any_interruptible> interruptibles;
        if (locked_scheduler.is_valid())
        {
            // transform to continuation tasks
            using wrapper_type = callback_continuation_wrapper<boost::asynchronous::detail::callback_continuation<Return,Job>>;
            std::vector<wrapper_type> targs;
            targs.reserve((args.size()));
            for(auto const& c : args)
            {
                targs.emplace_back(std::move(c));
            }
            continuation_ctor_helper(locked_scheduler,interruptibles,targs);
        }
        if (m_state)
            m_state->add_subs(interruptibles.begin(),interruptibles.end());
    }

    template <typename T,typename Interruptibles,typename ArgsVec>
    void continuation_ctor_helper(T& sched, Interruptibles& interruptibles,std::vector<ArgsVec>& l)
    {
        unsigned index = 0;
        for(auto& elem : l)
        {
            std::string n(elem.get_name());
            auto finished = m_finished;
            elem.set_done_func([finished,index](boost::asynchronous::expected<typename ArgsVec::return_type> r)
                            {
                               (*finished).m_futures[index] = std::move(r);
                               finished->done();
                            });
            if (index == l.size()-1)
            {
                // we execute one task ourselves to save one post
                try
                {
                    elem();
                }
                catch(std::exception& e)
                {
                    boost::asynchronous::expected<typename ArgsVec::return_type> r;
                    r.set_exception(boost::copy_exception(e));
                    (*finished).m_futures[index] = std::move(r);
                    finished->done();
                }
            }
            else if (!m_state)
            {
                // no interruptible requested
                boost::asynchronous::post_future(sched,std::move(elem),n,boost::asynchronous::get_own_queue_index<>());
            }
            else if(!m_state->is_interrupted())
            {
                // interruptible requested
                interruptibles.push_back(std::get<1>(
                                             boost::asynchronous::interruptible_post_future(sched,std::move(elem),n,
                                                                                            boost::asynchronous::get_own_queue_index<>())));
            }
            ++index;
        }
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
        subtask_finished(std::size_t number_of_tasks, bool interruptible, bool supports_timeout)
            :m_futures(number_of_tasks),m_ready_futures(0),m_done(),m_interruptible(interruptible)
            ,m_interrupted(false),m_supports_timeout(supports_timeout)
        {
        }
        void done()
        {
            if (++m_ready_futures == m_futures.size()+1)
            {
                if (!m_interrupted)
                {
                    return_result();
                }
            }
        }
        template <class Func>
        void on_done(Func f)
        {
            if (m_interruptible || m_supports_timeout)
            {
                bool interrupted = false;
                {
                    boost::mutex::scoped_lock lock(m_mutex);
                    ++m_ready_futures;
                    m_done = std::move(f);
                    interrupted = m_interrupted;
                }
                if (interrupted)
                    return_result();
                else if (m_ready_futures == m_futures.size()+1)
                    return_result();
            }
            else
            {
                m_done = std::move(f);
                if (++m_ready_futures == m_futures.size()+1)
                {
                    if (!m_interrupted)
                    {
                        return_result();
                    }
                }
            }
        }
        bool is_ready()
        {
            return m_ready_futures == m_futures.size()+1;
        }
        void set_interrupted()
        {
            bool done_fct_set=false;
            {
                boost::mutex::scoped_lock lock(m_mutex);
                m_interrupted=true;
                done_fct_set = !!m_done;
            }
            if (done_fct_set)
                return_result();
        }
        void return_result()
        {
            m_done(std::move(m_futures));
        }

        std::vector<boost::asynchronous::expected<Return> > m_futures;
        std::atomic<std::size_t> m_ready_futures;
        std::function<void(std::vector<boost::asynchronous::expected<Return> >)> m_done;
        const bool m_interruptible;
        bool m_interrupted;
        const bool m_supports_timeout;
        // only needed in case interrupt capability is needed
        mutable boost::mutex m_mutex;
    };

    boost::shared_ptr<boost::asynchronous::detail::interrupt_state> m_state;
    Duration m_timeout;
    typename boost::chrono::high_resolution_clock::time_point m_start;
    boost::shared_ptr<subtask_finished> m_finished;

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
auto call_get_expected(T& ) -> boost::asynchronous::expected<typename T::return_type>
{
    // TODO not with empty ctor
    return boost::asynchronous::expected<typename T::return_type>();
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
