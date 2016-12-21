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
#include <QCoreApplication>

#include <cstddef>
#include <map>
#include <functional>

#include <boost/mpl/eval_if.hpp>
#include <boost/mpl/identity.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/asynchronous/detail/metafunctions.hpp>
#include <boost/asynchronous/callable_any.hpp>
#include <boost/asynchronous/post.hpp>
#include <boost/asynchronous/checks.hpp>
#include <boost/asynchronous/scheduler/tss_scheduler.hpp>
#include <boost/asynchronous/detail/function_traits.hpp>
#include <boost/asynchronous/detail/move_bind.hpp>
#include <boost/asynchronous/extensions/qt/qt_post_helper.hpp>

#include <boost/system/error_code.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/function.hpp>
#include <boost/make_shared.hpp>
#include <boost/thread.hpp>

namespace boost { namespace asynchronous
{
namespace detail {
struct qt_track{};


// a stupid scheduler which just calls the functor passed. It doesn't even post as the functor already contains a post call.
struct dummy_qt_scheduler
{
    typedef boost::asynchronous::any_callable job_type;
    void post(boost::asynchronous::any_callable&& c,std::size_t) const
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
template <class WJOB = BOOST_ASYNCHRONOUS_DEFAULT_JOB>
class qt_servant
{
public:
    qt_servant(boost::asynchronous::any_shared_scheduler_proxy<WJOB> w=boost::asynchronous::any_shared_scheduler_proxy<WJOB>())
        : m_tracking(boost::make_shared<boost::asynchronous::detail::qt_track>())
        , m_worker(w)
        , m_next_helper_id(0)
    {}
    // copy-ctor and operator= are needed for correct tracking
    qt_servant(qt_servant const& rhs)
        : m_tracking(boost::make_shared<boost::asynchronous::detail::qt_track>())
        , m_worker(rhs.m_worker)
        , m_next_helper_id(0)
    {
    }
    virtual ~qt_servant()
    {
    }

    qt_servant& operator= (qt_servant const& rhs)
    {
        m_tracking = boost::make_shared<boost::asynchronous::detail::qt_track>();
        m_worker = rhs.m_worker;
        m_next_helper_id = rhs.m_next_helper_id;
    }

    // helper for continuations
    template <class T>
    struct get_continuation_return
    {
        typedef typename T::return_type type;
    };    
    template <class F1, class F2>
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
    void post_callback(F1&& func,F2&& cb_func, std::string const& task_name, std::size_t post_prio, std::size_t cb_prio)
#else
    void post_callback(F1&& func,F2&& cb_func, std::string const& task_name="", std::size_t post_prio=0, std::size_t cb_prio=0)
#endif
    {
        typedef typename ::boost::mpl::eval_if<
            typename boost::asynchronous::detail::has_is_continuation_task<decltype(func())>::type,
            get_continuation_return<decltype(func())>,
            ::boost::mpl::identity<decltype(func())>
        >::type f1_result_type;

        unsigned long connect_id = m_next_helper_id;
        boost::shared_ptr<F2> cbptr(boost::make_shared<F2>(std::forward<F2>(cb_func)));
        boost::weak_ptr<boost::asynchronous::detail::qt_track> tracking (m_tracking);

        boost::shared_ptr<boost::asynchronous::detail::connect_functor_helper> c =
                boost::make_shared<boost::asynchronous::detail::connect_functor_helper>
                (m_next_helper_id,
                 [this,connect_id,cbptr,tracking](QEvent* e)
                 {
                    detail::qt_async_custom_event<boost::asynchronous::expected<f1_result_type> >* ce =
                            static_cast<detail::qt_async_custom_event<boost::asynchronous::expected<f1_result_type> >* >(e);
                    (*cbptr)(std::move(ce->m_future));
                    if (!tracking.expired())
                    {
                        this->m_waiting_callbacks.erase(this->m_waiting_callbacks.find(connect_id));
                    }
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
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
    boost::asynchronous::any_interruptible interruptible_post_callback(F1&& func,F2&& cb_func, std::string const& task_name,
                                                                    std::size_t post_prio, std::size_t cb_prio)
#else
    boost::asynchronous::any_interruptible interruptible_post_callback(F1&& func,F2&& cb_func, std::string const& task_name="",
                                                                    std::size_t post_prio=0, std::size_t cb_prio=0)
#endif
    {
        typedef decltype(func()) f1_result_type;
        unsigned long connect_id = m_next_helper_id;
        boost::weak_ptr<boost::asynchronous::detail::qt_track> tracking (m_tracking);

        boost::shared_ptr<F2> cbptr(boost::make_shared<F2>(std::forward<F2>(cb_func)));
        boost::shared_ptr<boost::asynchronous::detail::connect_functor_helper> c =
                boost::make_shared<boost::asynchronous::detail::connect_functor_helper>
                (m_next_helper_id,
                 [this,connect_id,cbptr,tracking](QEvent* e)
                 {
                    detail::qt_async_custom_event<boost::asynchronous::expected<f1_result_type> >* ce =
                            static_cast<detail::qt_async_custom_event<boost::asynchronous::expected<f1_result_type> >* >(e);
                    (*cbptr)(std::move(ce->m_future));
                    if (!tracking.expired())
                    {
                        this->m_waiting_callbacks.erase(this->m_waiting_callbacks.find(connect_id));
                    }
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


    template <class F1>
    auto post_self(F1&& func,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                   std::string const& task_name, std::size_t post_prio)
#else
                   std::string const& task_name="", std::size_t post_prio=0)
#endif
     -> boost::future<typename boost::asynchronous::detail::get_return_type_if_possible_continuation<decltype(func())>::type>
    {
        typedef typename ::boost::mpl::eval_if<
            typename boost::asynchronous::detail::has_is_continuation_task<decltype(func())>::type,
            get_continuation_return<decltype(func())>,
            ::boost::mpl::identity<decltype(func())>
        >::type f1_result_type;

        unsigned long connect_id = m_next_helper_id;
        boost::weak_ptr<boost::asynchronous::detail::qt_track> tracking (m_tracking);

        boost::shared_ptr<boost::asynchronous::detail::connect_functor_helper> c =
                boost::make_shared<boost::asynchronous::detail::connect_functor_helper>
                (m_next_helper_id,
                 [this,connect_id,tracking](QEvent* e)
                 {
                    detail::qt_async_safe_callback_custom_event* ce =
                            static_cast<detail::qt_async_safe_callback_custom_event* >(e);
                    ce->m_cb();
                    if (!tracking.expired())
                    {
                        this->m_waiting_callbacks.erase(this->m_waiting_callbacks.find(connect_id));
                    }
                 }
               );

        m_waiting_callbacks[m_next_helper_id] = c;
        ++m_next_helper_id;
        // we want to log if possible
        return boost::asynchronous::post_future(
                                        boost::asynchronous::detail::dummy_qt_scheduler(),
                                        boost::asynchronous::detail::qt_safe_callback_helper(c.get(),std::forward<F1>(func)),
                                        task_name,
                                        post_prio);
    }

    // make a callback, which posts if not the correct thread, and call directly otherwise
    // in any case, check if this object is still alive
    template<class T>
    auto make_safe_callback(T func,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                                                     const std::string& task_name, std::size_t prio)
#else
                                                     const std::string& task_name="", std::size_t prio=0)
#endif
    -> decltype(boost::asynchronous::make_function(std::move(func)))
    {
        return this->make_safe_callback_helper(boost::asynchronous::make_function(std::move(func)),task_name,prio);
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
    template<typename... Args>
    std::function<void(Args... )> make_safe_callback_helper(std::function<void(Args... )> func,
#ifdef BOOST_ASYNCHRONOUS_REQUIRE_ALL_ARGUMENTS
                                                     const std::string& task_name, std::size_t prio)
#else
                                                     const std::string& task_name="", std::size_t prio=0)
#endif
    {
        boost::weak_ptr<boost::asynchronous::detail::qt_track> tracking (m_tracking);
        // now we are sure about our thread id
        m_own_thread_id = boost::this_thread::get_id();
        auto thread_id=m_own_thread_id;
        boost::shared_ptr<std::function<void(Args... )>> func_ptr =
                boost::make_shared<std::function<void(Args... )>>(std::move(func));

        unsigned long connect_id = m_next_helper_id;

        boost::shared_ptr<boost::asynchronous::detail::connect_functor_helper> c =
                boost::make_shared<boost::asynchronous::detail::connect_functor_helper>
                (m_next_helper_id,
                 [this,connect_id,tracking](QEvent* e)mutable
                 {
                    detail::qt_async_safe_callback_custom_event* ce =
                            static_cast<detail::qt_async_safe_callback_custom_event* >(e);
                    ce->m_cb();
                    // not possible to erase connect_id as safe callback can be called again and again
                 }
                );
        m_waiting_callbacks[m_next_helper_id] = c;
        ++m_next_helper_id;

        std::function<void(Args...)> res = [this,tracking,func_ptr,task_name,prio,thread_id,c,connect_id](Args... as)mutable
        {
            if (thread_id == boost::this_thread::get_id())
            {
                // our thread, call if servant alive
                boost::asynchronous::move_bind( boost::asynchronous::check_alive([func_ptr](Args... args){(*func_ptr)(std::move(args)...);},tracking),
                                                std::move(as)...)();
                // not possible to erase connect_id as safe callback can be called again and again
            }
            else
            {
                // not in our thread, post
                std::function<void()> safe_cb = boost::asynchronous::move_bind( boost::asynchronous::check_alive([func_ptr](Args... args){(*func_ptr)(std::move(args)...);},tracking),
                        std::move(as)...);
                boost::asynchronous::post_future(boost::asynchronous::detail::dummy_qt_scheduler(),
                                                 boost::asynchronous::detail::qt_safe_callback_helper(c.get(),std::move(safe_cb)),
                                                 task_name,prio);
            }
        };
        return res;
    }
    // tracking object for callbacks / tasks
    boost::shared_ptr<boost::asynchronous::detail::qt_track> m_tracking;
private:
    // our worker pool
    boost::asynchronous::any_shared_scheduler_proxy<WJOB> m_worker;
    unsigned long m_next_helper_id;
    std::map<unsigned long, boost::shared_ptr<boost::asynchronous::detail::connect_functor_helper> > m_waiting_callbacks;
    // we support just one id in Qt
    boost::thread::id m_own_thread_id;
};

}}

#endif // BOOST_ASYNCHRONOUS_QT_SERVANT_HPP
