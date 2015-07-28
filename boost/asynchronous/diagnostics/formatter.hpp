// Boost.Asynchronous library
//  Copyright (C) Tobias Holl 2015
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNC_FORMATTER_HPP
#define BOOST_ASYNC_FORMATTER_HPP

#include <functional>
#include <stdexcept>
#include <vector>

#include <boost/asynchronous/scheduler_diagnostics.hpp>
#include <boost/asynchronous/servant_proxy.hpp>
#include <boost/asynchronous/trackable_servant.hpp>
#include <boost/asynchronous/diagnostics/scheduler_interface.hpp>

// Requirements for formatters:
//  - Must have a default- and move-constructible 'parameter_type' member type or typedef / using-directive
//  - Must implement 'std::string format()', formatting and returning all diagnostics
//  - Must implement 'void clear_schedulers()', clearing all schedulers' diagnostics
//  - Must implement 'void clear_current()', clearing current diagnostics
//  - Must implement 'void clear_all()', clearing all persistent diagnostics (if available, otherwise this method does nothing)
//  - Must implement 'void register_scheduler(scheduler_interface interface)', registering an additional scheduler with the formatter
//  - Must implement 'formatter(std::vector<scheduler_interface> interfaces, formatter::parameter_type params = DEFAULT)'
//  - May implement 'std::string format(Args...)', formatting diagnostics like 'format' without using the integrated interfaces. The specific interface is implementation-defined
//  - May attach to the schedulers' diagnostics functor (called on shutdown with the remaining diagnostics)

namespace boost { namespace asynchronous {

typedef boost::asynchronous::any_loggable formatter_job;

// Formatter servant
template <typename Formatter>
class formatter_servant : public boost::asynchronous::trackable_servant<formatter_job, formatter_job> {
private:
    Formatter formatter;
public:
    formatter_servant(boost::asynchronous::any_weak_scheduler<formatter_job> scheduler,
                      boost::asynchronous::any_shared_scheduler_proxy<formatter_job> pool,
                      std::vector<boost::asynchronous::scheduler_interface> interfaces,
                      typename Formatter::parameter_type params = typename Formatter::parameter_type())
            : boost::asynchronous::trackable_servant<formatter_job, formatter_job>(scheduler, pool)
            , formatter(std::move(interfaces), std::move(params)) {}

    std::string format()
    {
        return formatter.format();
    }

    template <typename Func>
    void format_callback(Func callback)
    {
        callback(format());
    }

    template <typename... Args>
    std::string format_diagnostics(Args... args)
    {
        return formatter.format(std::move(args)...);
    }

    template <typename Func, typename... Args>
    void format_diagnostics_callback(Func callback, Args... args)
    {
        callback(formatter.format(std::move(args)...));
    }

    void clear_schedulers()
    {
        formatter.clear_schedulers();
    }

    void clear_current()
    {
        formatter.clear_current();
    }

    void clear_all()
    {
        formatter.clear_all();
    }

    void register_scheduler(boost::asynchronous::scheduler_interface interface)
    {
        formatter.register_scheduler(std::move(interface));
    }
};

// Proxy for a formatter
template <typename Formatter>
class formatter_proxy : public boost::asynchronous::servant_proxy<formatter_proxy<Formatter>, formatter_servant<Formatter>, formatter_job> {
public:
    template <typename Scheduler, typename Pool>
    formatter_proxy(Scheduler s,
                    Pool p,
                    std::vector<scheduler_interface> interfaces,
                    typename Formatter::parameter_type params = typename Formatter::parameter_type())
        : boost::asynchronous::servant_proxy<formatter_proxy<Formatter>,
                                             formatter_servant<Formatter>,
                                             formatter_job>(std::move(s),
                                                            std::move(p),
                                                            std::move(interfaces),
                                                            std::move(params))
    {}

    using servant_type = typename boost::asynchronous::servant_proxy<formatter_proxy<Formatter>, formatter_servant<Formatter>, formatter_job>::servant_type;
    using callable_type = typename boost::asynchronous::servant_proxy<formatter_proxy<Formatter>, formatter_servant<Formatter>, formatter_job>::callable_type;

    BOOST_ASYNC_FUTURE_MEMBER_LOG(format, "formatter::format", 1);
    BOOST_ASYNC_POST_MEMBER_LOG(format_callback, "formatter::format_callback", 1);

    BOOST_ASYNC_FUTURE_MEMBER_LOG(format_diagnostics, "formatter::format_diagnostics", 1);
    BOOST_ASYNC_POST_MEMBER_LOG(format_diagnostics_callback, "formatter::format_diagnostics_callback", 1);

    BOOST_ASYNC_POST_MEMBER_LOG(clear_schedulers, "formatter::clear_schedulers", 1);
    BOOST_ASYNC_POST_MEMBER_LOG(clear_current, "formatter::clear_current", 1);
    BOOST_ASYNC_POST_MEMBER_LOG(clear_all, "formatter::clear_all", 1);

    BOOST_ASYNC_POST_MEMBER_LOG(register_scheduler, "formatter::register_scheduler", 1);

    BOOST_ASYNC_SERVANT_POST_CTOR_LOG("formatter::[ctor]", 1);
    BOOST_ASYNC_SERVANT_POST_DTOR_LOG("formatter::[dtor]", 1);
};

}}

#endif // BOOST_ASYNC_FORMATTER_HPP
