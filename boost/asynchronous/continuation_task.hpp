#ifndef BOOST_ASYNCHRONOUS_CONTINUATION_TASK_HPP
#define BOOST_ASYNCHRONOUS_CONTINUATION_TASK_HPP

#include <string>
#include <boost/asynchronous/scheduler/tss_scheduler.hpp>
#include <boost/asynchronous/detail/continuation_impl.hpp>
#include <boost/asynchronous/scheduler/detail/any_continuation.hpp>
#include <boost/asynchronous/callable_any.hpp>
#include <boost/asynchronous/scheduler/detail/interrupt_state.hpp>

namespace boost { namespace asynchronous {

// what has to be set when a task is ready
template <class Return>
struct continuation_result
{
public:
    continuation_result(boost::shared_ptr<boost::promise<Return> > p):m_promise(p){}
    void set_value(Return const& val)const
    {
        m_promise->set_value(val);
    }
private:
    boost::shared_ptr<boost::promise<Return> > m_promise;
};

// the base class of continuation tasks Provides typedefs and hides promise.
template <class Return>
struct continuation_task
{
public:
    typedef Return res_type;

    continuation_task(const std::string& name=""):m_promise(new boost::promise<Return>()),m_name(name){}

    void set_value(Return const& val) const
    {
        m_promise->set_value(val);
    }

    boost::future<Return> get_future()const
    {
        return m_promise->get_future();
    }

    boost::shared_ptr<boost::promise<Return> > get_promise()const
    {
        return m_promise;
    }
    std::string get_name()const
    {
        return m_name;
    }

    boost::asynchronous::continuation_result<Return> this_task_result()const
    {
        return continuation_result<Return>(m_promise);
    }

private:
    boost::shared_ptr<boost::promise<Return> > m_promise;
    std::string m_name;
};

// inside a task, create a continuation handling any number of subtasks
template <class Return, class OnDone, typename... Args>
typename boost::disable_if< typename boost::asynchronous::detail::has_future_args<Args...>::type ,void >::type
create_continuation(OnDone&& on_done, Args&&... args)
{
    boost::shared_ptr<boost::asynchronous::detail::interrupt_state> state = boost::asynchronous::get_interrupt_state<>();
    typedef decltype(boost::asynchronous::detail::make_future_tuple(args...)) future_type;
    boost::asynchronous::detail::continuation<Return,boost::asynchronous::any_callable,typename future_type::element_type> c (
                state,boost::asynchronous::detail::make_future_tuple(args...),std::forward<Args>(args)...);
    c.on_done(on_done);
    boost::asynchronous::any_continuation a(c);
    boost::asynchronous::get_continuations().push_front(a);
}
template <class Return, class OnDone, typename... Args>
typename boost::enable_if< typename boost::asynchronous::detail::has_future_args<boost::future<Args>...>::type ,void >::type
create_continuation(OnDone&& on_done, boost::future<Args>&&... args)
{
    boost::shared_ptr<boost::asynchronous::detail::interrupt_state> state = boost::asynchronous::get_interrupt_state<>();

    typedef decltype(std::make_tuple(std::forward<boost::future<Args> >(args)...)) future_type;
    boost::shared_ptr<future_type> sp = boost::make_shared<future_type> (std::make_tuple( std::forward<boost::future<Args> >(args)...));

    boost::asynchronous::detail::continuation<Return,boost::asynchronous::any_callable,future_type> c (state,sp);
    c.on_done(on_done);
    boost::asynchronous::any_continuation a(c);
    boost::asynchronous::get_continuations().push_front(a);
}

template <class Return, typename Job, class OnDone, typename... Args>
typename boost::enable_if< typename boost::asynchronous::detail::has_future_args<boost::future<Args>...>::type ,void >::type
create_continuation_log(OnDone&& on_done, boost::future<Args>&&... args)
{
    boost::shared_ptr<boost::asynchronous::detail::interrupt_state> state = boost::asynchronous::get_interrupt_state<>();

    typedef decltype(std::make_tuple(std::forward<boost::future<Args> >(args)...)) future_type;
    boost::shared_ptr<future_type> sp = boost::make_shared<future_type> (std::make_tuple( std::forward<boost::future<Args> >(args)...));

    boost::asynchronous::detail::continuation<Return,Job,future_type> c (state,sp);
    c.on_done(on_done);
    boost::asynchronous::any_continuation a(c);
    boost::asynchronous::get_continuations().push_front(a);
}

//  create the first continuation in the serie
template <class Return, class FirstTask>
boost::asynchronous::detail::continuation<Return,boost::asynchronous::any_callable> top_level_continuation(FirstTask&& t)
{
    boost::shared_ptr<boost::asynchronous::detail::interrupt_state> state = boost::asynchronous::get_interrupt_state<>();
    return boost::asynchronous::detail::continuation<Return,boost::asynchronous::any_callable>(state,boost::make_shared<std::tuple<boost::future<Return> > >
                                                     (std::make_tuple(t.get_future())),std::forward<FirstTask>(t));
}
// version with logging
template <class Return, typename Job, class OnDone, typename... Args>
typename boost::disable_if< typename boost::asynchronous::detail::has_future_args<Args...>::type ,void >::type
create_continuation_log(OnDone&& on_done, Args&&... args)
{
    boost::shared_ptr<boost::asynchronous::detail::interrupt_state> state = boost::asynchronous::get_interrupt_state<>();

    typedef decltype(boost::asynchronous::detail::make_future_tuple(args...)) future_type;
    boost::asynchronous::detail::continuation<Return,Job,typename future_type::element_type> c (
                state,boost::asynchronous::detail::make_future_tuple(args...),std::forward<Args>(args)...);
    c.on_done(on_done);
    boost::asynchronous::any_continuation a(c);
    boost::asynchronous::get_continuations().push_front(a);
}

//  create the first continuation in the serie
template <class Return, typename Job, class FirstTask>
boost::asynchronous::detail::continuation<Return,Job> top_level_continuation_log(FirstTask&& t)
{
    boost::shared_ptr<boost::asynchronous::detail::interrupt_state> state = boost::asynchronous::get_interrupt_state<>();

    return boost::asynchronous::detail::continuation<Return,Job>(state,boost::make_shared<std::tuple<boost::future<Return> > >
                                                     (std::make_tuple(t.get_future())),std::forward<FirstTask>(t));
}

}}

#endif // BOOST_ASYNCHRONOUS_CONTINUATION_TASK_HPP
