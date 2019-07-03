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
#include <type_traits>
#include <future>

#include <functional>
#include <memory>
#include <boost/type_erasure/is_empty.hpp>

#include <boost/asynchronous/callable_any.hpp>
#include <boost/asynchronous/post.hpp>
#include <boost/asynchronous/any_scheduler.hpp>
#include <boost/asynchronous/scheduler/detail/interrupt_state.hpp>
#include <boost/asynchronous/expected.hpp>
#include <boost/asynchronous/scheduler/tss_scheduler.hpp>


namespace boost { namespace asynchronous {
BOOST_MPL_HAS_XXX_TRAIT_DEF(state)
BOOST_MPL_HAS_XXX_TRAIT_DEF(iterator)

// policy telling us how continuations should post. Default is fastest for most tasks(post all but one, execute one directly in continuation)
enum class continuation_post_policy
{
    post_all_but_one,
    post_all
};

// what has to be set when a task is ready
template <class Return>
struct continuation_result
{
public:
    typedef Return return_type;
    continuation_result(std::shared_ptr<std::promise<Return> > p,std::function<void(boost::asynchronous::expected<Return>)> f)
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
    void set_exception(std::exception_ptr p)const
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
    std::shared_ptr<std::promise<Return> > m_promise;
    std::function<void(boost::asynchronous::expected<Return>)> m_done_func;
};
template <>
struct continuation_result<void>
{
public:
    typedef void return_type;
    continuation_result(std::shared_ptr<std::promise<void> > p,std::function<void(boost::asynchronous::expected<void>)> f)
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
    void set_exception(std::exception_ptr p)const
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
    std::shared_ptr<std::promise<void> > m_promise;
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

    std::future<Return> get_future()const
    {
        // create only when asked
        if (!m_promise)
            const_cast<continuation_task<Return>&>(*this).m_promise = std::make_shared<std::promise<Return>>();
        return m_promise->get_future();
    }

    std::shared_ptr<std::promise<Return> > get_promise()const
    {
        // create only when asked
        if (!m_promise)
            const_cast<continuation_task<Return>&>(*this).m_promise = std::make_shared<std::promise<Return>>();
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
                (m_done_func)(boost::asynchronous::expected<Return>(std::make_exception_ptr(payload.m_exception)));
            else
                get_promise()->set_exception(std::make_exception_ptr(payload.m_exception));
        }
    }
    void set_done_func(std::function<void(boost::asynchronous::expected<Return>)> f)
    {
        m_done_func=std::move(f);
    }
private:
    std::shared_ptr<std::promise<Return> > m_promise;
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

    std::future<void> get_future()const
    {
        // create only when asked
        if (!m_promise)
            const_cast<continuation_task<void>&>(*this).m_promise = std::make_shared<std::promise<void>>();
        return m_promise->get_future();
    }

    std::shared_ptr<std::promise<void> > get_promise()const
    {
        // create only when asked
        if (!m_promise)
            const_cast<continuation_task<void>&>(*this).m_promise = std::make_shared<std::promise<void>>();
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
                (m_done_func)(boost::asynchronous::expected<void>(std::make_exception_ptr(payload.m_exception)));
            else
                get_promise()->set_exception(std::make_exception_ptr(payload.m_exception));
        }
    }
    void set_done_func(std::function<void(boost::asynchronous::expected<void>)> f)
    {
        m_done_func=std::move(f);
    }

private:
    std::shared_ptr<std::promise<void> > m_promise;
    std::string m_name;
    std::function<void(boost::asynchronous::expected<void>)> m_done_func;
};

namespace detail
{

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
        catch (...)
        {
            task_res.set_exception(std::current_exception());
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
        catch (...)
        {
            task_res.set_exception(std::current_exception());
        }
    }

    Func myFunc;
};
}
template <class Func>
auto make_lambda_continuation_wrapper(Func f,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                                      std::string const& name)
#else
                                      std::string const& name="")
#endif
-> boost::asynchronous::detail::lambda_continuation_wrapper<Func,decltype(f())>
{
    return boost::asynchronous::detail::lambda_continuation_wrapper<Func,decltype(f())>(std::move(f),name);
}

// same as before but getting a task_result as argument to make it easier to build a top-level task
// only to be used for simple case (no recursion possible)
namespace detail
{
template <class Func, class ReturnType>
struct lambda_top_level_continuation_wrapper : public boost::asynchronous::continuation_task<ReturnType>
{
    lambda_top_level_continuation_wrapper(Func f, std::string const& name)
        : boost::asynchronous::continuation_task<ReturnType>(name)
        , myFunc(std::move(f))
    {}
    void operator()()
    {
        boost::asynchronous::continuation_result<ReturnType> task_res = this->this_task_result();
        myFunc(std::move(task_res));
    }

    Func myFunc;
};
}
template <class ReturnType,class Func>
auto make_top_level_lambda_continuation(Func f,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                                      std::string const& name)
#else
                                      std::string const& name="")
#endif
-> boost::asynchronous::detail::lambda_top_level_continuation_wrapper<Func,ReturnType>
{
    return boost::asynchronous::detail::lambda_top_level_continuation_wrapper<Func,ReturnType>(std::move(f),name);
}


namespace detail {

#define BOOST_ASYNCHRONOUS_TRY_OTHER_JOB_TYPES0(Job)                        \
auto weak_scheduler = boost::asynchronous::get_thread_scheduler<Job>();     \
if (!boost::type_erasure::is_empty(weak_scheduler))                         \
{                                                                           \
    auto locked_scheduler = weak_scheduler.lock();                          \
    if (locked_scheduler.is_valid())                                        \
    {                                                                       \
        continuation_ctor_helper                                            \
            (locked_scheduler,interruptibles,std::forward<Args>(args)...);  \
    }                                                                       \
}

// the continuation task implementation
template <class Return, typename Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB, typename Tuple=std::tuple<std::future<Return> > , typename Duration = std::chrono::milliseconds >
struct continuation
{
    typedef int is_continuation_task;
    typedef Return return_type;
    typedef Job job_type;
    // metafunction telling if we are future or expected based sind
    template <class T>
    struct continuation_args
    {
        typedef std::future<T> type;
    };
    // workaround for compiler crash (gcc 4.7)
    std::future<Return> get_continuation_args()const
    {
        return std::future<Return>();
    }

    continuation()=default;

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
    continuation(std::shared_ptr<boost::asynchronous::detail::interrupt_state> state,Tuple t, Duration d, Args&&... args)
    : m_futures(std::move(t))
    , m_ready_futures(std::tuple_size<Tuple>::value,false)
    , m_state(state)
    , m_timeout(d)
    {
        // remember when we started
        m_start = std::chrono::high_resolution_clock::now();
        std::vector<boost::asynchronous::any_interruptible> interruptibles;
        BOOST_ASYNCHRONOUS_TRY_OTHER_JOB_TYPES0(Job)
        else
        {
            BOOST_ASYNCHRONOUS_TRY_OTHER_JOB_TYPES0(boost::asynchronous::any_callable)
            else
            {
                BOOST_ASYNCHRONOUS_TRY_OTHER_JOB_TYPES0(boost::asynchronous::any_loggable)
            }
        }
        if (m_state)
            m_state->add_subs(interruptibles.begin(),interruptibles.end());
    }
    // version where we already get futures
    continuation(std::shared_ptr<boost::asynchronous::detail::interrupt_state> state,Tuple t, Duration d)
    : m_futures(std::move(t))
    , m_ready_futures(std::tuple_size<Tuple>::value,false)
    , m_state(state)
    , m_timeout(d)
    {
        // remember when we started
        m_start = std::chrono::high_resolution_clock::now();
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
        typename std::chrono::high_resolution_clock::time_point time_now = std::chrono::high_resolution_clock::now();
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
    std::shared_ptr<boost::asynchronous::detail::interrupt_state> m_state;
    Duration m_timeout;
    typename std::chrono::high_resolution_clock::time_point m_start;

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
        else if ((boost::asynchronous::is_ready(std::get<I>(t)) /*|| (std::get<I>(t)).has_exception()*/) )
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

#define BOOST_ASYNCHRONOUS_TRY_OTHER_JOB_TYPES(Job)                         \
auto weak_scheduler = boost::asynchronous::get_thread_scheduler<Job>();     \
if (!boost::type_erasure::is_empty(weak_scheduler))                         \
{                                                                           \
    auto locked_scheduler = weak_scheduler.lock();                          \
    if (locked_scheduler.is_valid())                                        \
    {                                                                       \
        continuation_ctor_helper<0>                                         \
            (locked_scheduler,interruptibles,std::forward<Args>(args)...);  \
    }                                                                       \
}
#define BOOST_ASYNCHRONOUS_TRY_OTHER_JOB_TYPES2(Job)                        \
auto weak_scheduler = boost::asynchronous::get_thread_scheduler<Job>();     \
if (!boost::type_erasure::is_empty(weak_scheduler))                         \
{                                                                           \
    auto locked_scheduler = weak_scheduler.lock();                          \
    if (locked_scheduler.is_valid())                                        \
    {                                                                       \
        continuation_ctor_helper_tuple<0>                                   \
            (locked_scheduler,interruptibles,args);                         \
    }                                                                       \
}
// for callback_continuation_as_seq
#define BOOST_ASYNCHRONOUS_TRY_OTHER_JOB_TYPES3(Job)                        \
auto weak_scheduler = boost::asynchronous::get_thread_scheduler<Job>();     \
if (!boost::type_erasure::is_empty(weak_scheduler))                         \
{                                                                           \
    auto locked_scheduler = weak_scheduler.lock();                          \
    if (locked_scheduler.is_valid())                                        \
    {                                                                       \
        continuation_ctor_helper                                            \
            (locked_scheduler,interruptibles,args);                         \
    }                                                                       \
}

template <class Return,
          typename Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB,
          typename Tuple=std::tuple<boost::asynchronous::expected<Return> > ,
          typename Duration = std::chrono::milliseconds >
struct callback_continuation
{
    typedef int is_continuation_task;
    typedef int is_callback_continuation_task;
    typedef Return return_type;
    typedef Job job_type;
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

    callback_continuation(callback_continuation&& rhs)=default;
    callback_continuation(callback_continuation const& rhs)=default;
    callback_continuation& operator= (callback_continuation&& rhs)=default;
    callback_continuation& operator= (callback_continuation const& rhs)=default;
    callback_continuation()=default;

    template <typename Func, typename... Args>
    callback_continuation(std::shared_ptr<boost::asynchronous::detail::interrupt_state> state,Tuple t, Duration d, Func f,
                          boost::asynchronous::continuation_post_policy post_policy ,
                          Args&&... args)
    : m_state(state)
    , m_timeout(d)
    , m_finished(std::make_shared<subtask_finished>(std::move(t),!!m_state,(m_timeout.count() != 0)))
    , m_post_policy(post_policy)
    {
        // remember when we started
        m_start = std::chrono::high_resolution_clock::now();

        on_done(std::move(f));
        std::vector<boost::asynchronous::any_interruptible> interruptibles;
        BOOST_ASYNCHRONOUS_TRY_OTHER_JOB_TYPES(Job)
        else
        {
            BOOST_ASYNCHRONOUS_TRY_OTHER_JOB_TYPES(boost::asynchronous::any_callable)
            else
            {
                BOOST_ASYNCHRONOUS_TRY_OTHER_JOB_TYPES(boost::asynchronous::any_loggable)
//                else
//                {
//                    BOOST_ASYNCHRONOUS_TRY_OTHER_JOB_TYPES(boost::asynchronous::any_serializable)
//                }
            }
        }
        if (m_state)
            m_state->add_subs(interruptibles.begin(),interruptibles.end());

    }
    // version where done functor is set later
    template <typename... Args>
    callback_continuation(std::shared_ptr<boost::asynchronous::detail::interrupt_state> state,Tuple t, Duration d, bool,
                          boost::asynchronous::continuation_post_policy post_policy ,
                          Args&&... args)
    : m_state(state)
    , m_timeout(d)
    , m_finished(std::make_shared<subtask_finished>(std::move(t),!!m_state,(m_timeout.count() != 0)))
    , m_post_policy(post_policy)
    {
        // remember when we started
        m_start = std::chrono::high_resolution_clock::now();

        std::vector<boost::asynchronous::any_interruptible> interruptibles;
        BOOST_ASYNCHRONOUS_TRY_OTHER_JOB_TYPES(Job)
        else
        {
            BOOST_ASYNCHRONOUS_TRY_OTHER_JOB_TYPES(boost::asynchronous::any_callable)
            else
            {
                BOOST_ASYNCHRONOUS_TRY_OTHER_JOB_TYPES(boost::asynchronous::any_loggable)
//                else
//                {
//                    BOOST_ASYNCHRONOUS_TRY_OTHER_JOB_TYPES(boost::asynchronous::any_serializable)
//                }
            }
        }
        if (m_state)
            m_state->add_subs(interruptibles.begin(),interruptibles.end());

    }
    template <typename Func,typename... Args>
    callback_continuation(std::shared_ptr<boost::asynchronous::detail::interrupt_state> state,Tuple t, Duration d,Func f,
                          boost::asynchronous::continuation_post_policy post_policy ,
                          std::tuple<Args...> args)
    : m_state(state)
    , m_timeout(d)
    , m_finished(std::make_shared<subtask_finished>(std::move(t),!!m_state,(m_timeout.count() != 0)))
    , m_post_policy(post_policy)
    {
        // remember when we started
        m_start = std::chrono::high_resolution_clock::now();

        on_done(std::move(f));
        std::vector<boost::asynchronous::any_interruptible> interruptibles;
        BOOST_ASYNCHRONOUS_TRY_OTHER_JOB_TYPES2(Job)
        else
        {
            BOOST_ASYNCHRONOUS_TRY_OTHER_JOB_TYPES2(boost::asynchronous::any_callable)
            else
            {
                BOOST_ASYNCHRONOUS_TRY_OTHER_JOB_TYPES2(boost::asynchronous::any_loggable)
//                else
//                {
//                    BOOST_ASYNCHRONOUS_TRY_OTHER_JOB_TYPES2(boost::asynchronous::any_serializable)
//                }
            }
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
                              std::shared_ptr<boost::asynchronous::detail::interrupt_state> state,bool last_task,
                              boost::asynchronous::continuation_post_policy post_policy,Task&& t)
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
                if (last_task && post_policy != boost::asynchronous::continuation_post_policy::post_all)
                {
                    // we execute one task ourselves to save one post
                    try
                    {
                        t();
                    }
                    catch(...)
                    {
                        boost::asynchronous::expected<typename Task::return_type> r;
                        r.set_exception(std::current_exception());
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
    struct task_type_selector<I,Task,typename std::enable_if< boost::asynchronous::detail::has_is_callback_continuation_task<Task>::value >::type>
    {
        template <typename S,typename Interruptibles,typename F>
        static void construct(S& ,Interruptibles&,F& func, std::shared_ptr<boost::asynchronous::detail::interrupt_state>,bool,
                              boost::asynchronous::continuation_post_policy ,Task&& t)
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
        task_type_selector<I,Last>::construct(sched,interruptibles,m_finished,m_state,true,m_post_policy,std::forward<Last>(l));
    }

    template <int I,typename T,typename Interruptibles,typename... Tail, typename Front>
    void continuation_ctor_helper(T& sched, Interruptibles& interruptibles,Front&& front,Tail&&... tail)
    {
        task_type_selector<I,Front>::construct(sched,interruptibles,m_finished,m_state,false,m_post_policy,std::forward<Front>(front));
        continuation_ctor_helper<I+1>(sched,interruptibles,std::forward<Tail>(tail)...);
    }

    template <int I,typename T,typename Interruptibles,typename... ArgsTuple>
    typename std::enable_if<I == sizeof...(ArgsTuple)-1,void>::type
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
            catch(...)
            {
                boost::asynchronous::expected<typename std::tuple_element<I, std::tuple<ArgsTuple...>>::type::return_type> r;
                r.set_exception(std::current_exception());
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
    typename std::enable_if<!(I == sizeof...(ArgsTuple)-1),void>::type
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
    std::string get_name() const
    {
        return m_name;
    }
    void set_name(std::string const& name)
    {
        m_name = name;
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
            m_finished->set_interrupted();
            return true;
        }
        // if timeout, we are ready too
        typename std::chrono::high_resolution_clock::time_point time_now = std::chrono::high_resolution_clock::now();
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
        subtask_finished(Tuple t, bool , bool )
            :m_futures(std::move(t)),m_ready_futures(0),m_done()
        {
        }
        void done()
        {
            if (++m_ready_futures == (std::tuple_size<Tuple>::value + 1))
            {
                return_result();
            }
        }
        template <class Func>
        void on_done(Func f)
        {
            m_done = std::move(f);
            if (++m_ready_futures == (std::tuple_size<Tuple>::value + 1))
            {
                return_result();
            }
        }
        bool is_ready()
        {
            return (m_ready_futures == (std::tuple_size<Tuple>::value+1));
        }
        void set_interrupted()
        {
            // not supported
        }
        void return_result()
        {
            m_done(std::move(m_futures));
        }

        Tuple m_futures;
        std::atomic<std::size_t> m_ready_futures;
        std::function<void(Tuple)> m_done;
    };

    std::shared_ptr<boost::asynchronous::detail::interrupt_state> m_state;
    Duration m_timeout;
    typename std::chrono::high_resolution_clock::time_point m_start;
    std::shared_ptr<subtask_finished> m_finished;
    boost::asynchronous::continuation_post_policy m_post_policy;
    std::string m_name="";

};

// version for sequences of futures
template <class Return, typename Job, typename Seq, typename Duration = std::chrono::milliseconds>
struct continuation_as_seq
{
    typedef int is_continuation_task;
    typedef Return return_type;
    typedef Job job_type;
    // metafunction telling if we are future or expected based sind
    template <class T>
    struct continuation_args
    {
        typedef std::future<T> type;
    };
    // workaround for compiler crash (gcc 4.7)
    std::future<Return> get_continuation_args()const
    {
        return std::future<Return>();
    }

    continuation_as_seq()=default;

    continuation_as_seq(std::shared_ptr<boost::asynchronous::detail::interrupt_state> state, Duration d,Seq seq)
    : m_futures(std::move(seq))
    , m_ready_futures(m_futures.size(),false)
    , m_state(state)
    , m_timeout(d)
    {
        // remember when we started
        m_start = std::chrono::high_resolution_clock::now();
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
        typename std::chrono::high_resolution_clock::time_point time_now = std::chrono::high_resolution_clock::now();
        if (m_timeout.count() != 0 && (time_now - m_start >= m_timeout))
        {
            return true;
        }
        std::size_t index = 0;
        for (typename Seq::const_iterator it = m_futures.begin(); it != m_futures.end() ;++it,++index)
        {
            if (m_ready_futures[index])
                continue;
            else if ((boost::asynchronous::is_ready(*it) /*|| (*it).has_exception()*/ ) )
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
    std::shared_ptr<boost::asynchronous::detail::interrupt_state> m_state;
    Duration m_timeout;
    typename std::chrono::high_resolution_clock::time_point m_start;
};

template <class Return, typename Job=BOOST_ASYNCHRONOUS_DEFAULT_JOB, typename Duration = std::chrono::milliseconds >
struct callback_continuation_as_seq
{
    typedef int is_continuation_task;
    typedef int is_callback_continuation_task;
    typedef Return return_type;
    typedef Job job_type;
    // metafunction telling if we are future or expected based
    template <class T>
    struct continuation_args
    {
        typedef boost::asynchronous::expected<T> type;
    };
    // wrapper in case we only get a callback_continuation
    template <class Continuation, class Enable =void>
    struct callback_continuation_wrapper : public boost::asynchronous::continuation_task<Return>
    {
        callback_continuation_wrapper(Continuation cont, std::string const& name)
        : boost::asynchronous::continuation_task<Return>(name)
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
                catch(...)
                {
                    task_res.set_exception(std::current_exception());
                }
            });
        }
        Continuation m_cont;
    };
    template <class Continuation>
    struct callback_continuation_wrapper<Continuation,
                                         typename std::enable_if<std::is_same<typename Continuation::return_type,void>::value >::type>
            : public boost::asynchronous::continuation_task<void>
    {
        callback_continuation_wrapper(Continuation cont, std::string const& name)
            : boost::asynchronous::continuation_task<void>(name)
        , m_cont(std::move(cont))
        {}
        void operator()()
        {
            boost::asynchronous::continuation_result<void> task_res = this->this_task_result();
            m_cont.on_done([task_res](std::tuple<boost::asynchronous::expected<void> >&& res)
            {
                try
                {
                    // check for exception
                    std::get<0>(res).get();
                    task_res.set_value();
                }
                catch(...)
                {
                    task_res.set_exception(std::current_exception());
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

    callback_continuation_as_seq()=default;

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
    callback_continuation_as_seq(std::shared_ptr<boost::asynchronous::detail::interrupt_state> state, Duration d,Func f,
                          std::vector<Args> args)
    : m_state(state)
    , m_timeout(d)
    , m_finished(std::make_shared<subtask_finished>(args.size(),!!m_state,(m_timeout.count() != 0)))
    {
        // remember when we started
        m_start = std::chrono::high_resolution_clock::now();

        on_done(std::move(f));

        std::vector<boost::asynchronous::any_interruptible> interruptibles;
        BOOST_ASYNCHRONOUS_TRY_OTHER_JOB_TYPES3(Job)
        else
        {
            BOOST_ASYNCHRONOUS_TRY_OTHER_JOB_TYPES3(boost::asynchronous::any_callable)
            else
            {
                BOOST_ASYNCHRONOUS_TRY_OTHER_JOB_TYPES3(boost::asynchronous::any_loggable)
//                else
//                {
//                    BOOST_ASYNCHRONOUS_TRY_OTHER_JOB_TYPES3(boost::asynchronous::any_serializable)
//                }
            }
        }
        if (m_state)
            m_state->add_subs(interruptibles.begin(),interruptibles.end());
    }

    template <typename Func>
    callback_continuation_as_seq(std::shared_ptr<boost::asynchronous::detail::interrupt_state> state, Duration d,Func f,
                          std::vector<boost::asynchronous::detail::callback_continuation<Return,Job>> args_)
    : m_state(state)
    , m_timeout(d)
    , m_finished(std::make_shared<subtask_finished>(args_.size(),!!m_state,(m_timeout.count() != 0)))
    {
        // remember when we started
        m_start = std::chrono::high_resolution_clock::now();

        on_done(std::move(f));

        std::vector<boost::asynchronous::any_interruptible> interruptibles;
        using wrapper_type = callback_continuation_wrapper<boost::asynchronous::detail::callback_continuation<Return,Job>>;
        std::vector<wrapper_type> args;
        args.reserve((args.size()));
        for(auto& c : args_)
        {
            args.emplace_back(std::move(c),c.get_name());
        }
        BOOST_ASYNCHRONOUS_TRY_OTHER_JOB_TYPES3(Job)
        else
        {
            BOOST_ASYNCHRONOUS_TRY_OTHER_JOB_TYPES3(boost::asynchronous::any_callable)
            else
            {
                BOOST_ASYNCHRONOUS_TRY_OTHER_JOB_TYPES3(boost::asynchronous::any_loggable)
            }
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
                catch(...)
                {
                    boost::asynchronous::expected<typename ArgsVec::return_type> r;
                    r.set_exception(std::current_exception());
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
        typename std::chrono::high_resolution_clock::time_point time_now = std::chrono::high_resolution_clock::now();
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
        subtask_finished(std::size_t number_of_tasks, bool , bool )
            :m_futures(number_of_tasks),m_ready_futures(0),m_done()
        {
        }
        void done()
        {
            if (++m_ready_futures == m_futures.size()+1)
            {
                return_result();
            }
        }
        template <class Func>
        void on_done(Func f)
        {
            m_done = std::move(f);
            if (++m_ready_futures == m_futures.size()+1)
            {
                return_result();
            }
        }
        bool is_ready()
        {
            return m_ready_futures == m_futures.size()+1;
        }
        void set_interrupted()
        {
            // not supported
        }
        void return_result()
        {
            m_done(std::move(m_futures));
        }

        std::vector<boost::asynchronous::expected<Return> > m_futures;
        std::atomic<std::size_t> m_ready_futures;
        std::function<void(std::vector<boost::asynchronous::expected<Return> >)> m_done;
    };

    std::shared_ptr<boost::asynchronous::detail::interrupt_state> m_state;
    Duration m_timeout;
    typename std::chrono::high_resolution_clock::time_point m_start;
    std::shared_ptr<subtask_finished> m_finished;

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
class is_future_helper
{
    typedef char one;
    typedef long two;

    template <typename C> static one test( decltype(&C::get) ) ;
    template <typename C> static two test(...);

public:
    enum { value = sizeof(test<Front>(0)) == sizeof(char) };
};

template <typename... Args>
struct is_future
{
    enum {value = is_future_helper<Args...>::value};
};


template <typename Front,typename... Tail>
struct has_iterator_args_helper
{
    typedef typename boost::asynchronous::has_iterator<Front>::type type;
    enum {value = type::value};
};

template <typename... Args>
struct has_iterator_args
{
    typedef typename has_iterator_args_helper<Args...>::type type;
    enum {value = type::value};
};

}}}

#endif // BOOST_ASYNCHRONOUS_CONTINUATION_IMPL_HPP
