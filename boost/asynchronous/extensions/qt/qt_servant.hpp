// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2013
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNCHRONOUS_QT_SERVANT_HPP
#define BOOST_ASYNCHRONOUS_QT_SERVANT_HPP

#include <QObject>
#include <QApplication>

#include <cstddef>
#include <map>

#include <boost/shared_ptr.hpp>
#include <boost/asynchronous/callable_any.hpp>
#include <boost/asynchronous/post.hpp>
#include <boost/asynchronous/checks.hpp>
#include <boost/asynchronous/scheduler/tss_scheduler.hpp>
#include <boost/system/error_code.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/function.hpp>
#include <boost/make_shared.hpp>

namespace boost { namespace asynchronous
{
namespace detail {

template <class T>
class qt_async_custom_event : public QEvent
{
public:
    qt_async_custom_event( T f)
        : QEvent(static_cast<QEvent::Type>(QEvent::registerEventType()))
        , m_future(std::move(f))
    {}

    virtual ~qt_async_custom_event()
    {}
    T m_future;
};

class connect_functor_helper : public QObject
{
    Q_OBJECT
public:
    connect_functor_helper(unsigned long id, const boost::function<void(QEvent*)> &f)
        : QObject(0)
        , m_id(id)
        , m_function(f)
    {}
    connect_functor_helper(connect_functor_helper const& rhs):QObject(0), m_id(rhs.m_id),m_function(rhs.m_function){}

    unsigned long get_id()const
    {
        return m_id;
    }
    void customEvent(QEvent* event)
    {
        m_function(event);
        QObject::customEvent(event);
    }

private:
    unsigned long m_id;
    boost::function<void(QEvent*)> m_function;
};

class qt_post_helper : public QObject
{
    Q_OBJECT
public:
    typedef boost::asynchronous::any_callable job_type;
    qt_post_helper(connect_functor_helper* c)
    : QObject(0)
    , m_connect(c)
    {}
    qt_post_helper(qt_post_helper const& rhs)
    : QObject(0)
    , m_connect(rhs.m_connect)
    {}

    template <class Future>
    void operator()(Future f)
    {
        QApplication::postEvent(m_connect,new qt_async_custom_event<Future>(std::move(f)));
    }

private:
   connect_functor_helper*              m_connect;
};

// a stupid scheduler which just calls the functor passed. It doesn't even post as the functor already contains a post call.
struct dummy_qt_scheduler
{
    void post(boost::asynchronous::any_callable&& c, const std::string&,std::size_t) const
    {
        c();
    }
    bool is_valid()const
    {
        return true;
    }
};
struct dymmy_weak_qt_scheduler
{
    typedef boost::asynchronous::any_callable job_type;
    dummy_qt_scheduler lock() const
    {
        return dummy_qt_scheduler();
    }
};

}
// servant for using a Qt event loop for callback dispatching.
// hides threadpool, adds automatic trackability for callbacks and tasks
// inherit from it to get functionality
template <class WJOB = boost::asynchronous::any_callable>
class qt_servant
{
public:
    qt_servant(boost::asynchronous::any_shared_scheduler_proxy<WJOB> w=boost::asynchronous::any_shared_scheduler_proxy<WJOB>())
        : m_worker(w)
        , m_next_helper_id(0)
    {}
    // copy-ctor and operator= are needed for correct tracking
    qt_servant(qt_servant const& rhs)
        : m_worker(rhs.m_worker)
        , m_next_helper_id(0)
    {
    }
    ~qt_servant()
    {
    }

    qt_servant& operator= (qt_servant const& rhs)
    {
        m_worker = rhs.m_worker;
        m_next_helper_id = rhs.m_next_helper_id;
    }
                
    template <class F1, class F2>
    void post_callback(F1&& func,F2&& cb_func, std::string const& task_name="", std::size_t post_prio=0, std::size_t cb_prio=0)
    {
        typedef decltype(func()) f1_result_type;
        unsigned long connect_id = m_next_helper_id;
        boost::shared_ptr<F2> cbptr(boost::make_shared<F2>(std::forward<F2>(cb_func)));

        boost::shared_ptr<boost::asynchronous::detail::connect_functor_helper> c =
                boost::make_shared<boost::asynchronous::detail::connect_functor_helper>
                (m_next_helper_id,
                 [this,connect_id,cbptr](QEvent* e)
                 {
                    detail::qt_async_custom_event<boost::future<f1_result_type> >* ce =
                            static_cast<detail::qt_async_custom_event<boost::future<f1_result_type> >* >(e);
                    (*cbptr)(std::move(ce->m_future));
                    this->m_waiting_callbacks.erase(this->m_waiting_callbacks.find(connect_id));
                 }
               );

        m_waiting_callbacks[m_next_helper_id] = c;
        ++m_next_helper_id;
        // we want to log if possible
        boost::asynchronous::post_callback(m_worker,
                                        std::forward<F1>(func),
                                        boost::asynchronous::detail::dymmy_weak_qt_scheduler(),
                                        boost::asynchronous::detail::qt_post_helper(c.get()),
                                        task_name,
                                        post_prio,
                                        cb_prio);
    }
    template <class F1, class F2>
    boost::asynchronous::any_interruptible interruptible_post_callback(F1&& func,F2&& cb_func, std::string const& task_name="",
                                                                    std::size_t post_prio=0, std::size_t cb_prio=0)
    {
        typedef decltype(func()) f1_result_type;
        unsigned long connect_id = m_next_helper_id;
        boost::shared_ptr<F2> cbptr(boost::make_shared<F2>(std::forward<F2>(cb_func)));
        boost::shared_ptr<boost::asynchronous::detail::connect_functor_helper> c =
                boost::make_shared<boost::asynchronous::detail::connect_functor_helper>
                (m_next_helper_id,
                 [this,connect_id,cbptr](QEvent* e)
                 {
                    detail::qt_async_custom_event<boost::future<f1_result_type> >* ce =
                            static_cast<detail::qt_async_custom_event<boost::future<f1_result_type> >* >(e);
                    (*cbptr)(std::move(ce->m_future));
                    this->m_waiting_callbacks.erase(this->m_waiting_callbacks.find(connect_id));
                 }
                );
        m_waiting_callbacks[m_next_helper_id] = c;
        ++m_next_helper_id;
        return boost::asynchronous::interruptible_post_callback(
                                        m_worker,
                                        std::forward<F1>(func),
                                        boost::asynchronous::detail::dymmy_weak_qt_scheduler(),
                                        boost::asynchronous::detail::qt_post_helper(c.get()),
                                        task_name,
                                        post_prio,
                                        cb_prio);
    }
protected:
    boost::asynchronous::any_shared_scheduler_proxy<WJOB> const& get_worker()const
    {
        return m_worker;
    }
    void set_worker(boost::asynchronous::any_shared_scheduler_proxy<WJOB> w)
    {
        m_worker=w;
    }
private:
    // our worker pool
    boost::asynchronous::any_shared_scheduler_proxy<WJOB> m_worker;
    unsigned long m_next_helper_id;
    std::map<unsigned long, boost::shared_ptr<boost::asynchronous::detail::connect_functor_helper> > m_waiting_callbacks;
};

}}

#endif // BOOST_ASYNCHRONOUS_QT_SERVANT_HPP
