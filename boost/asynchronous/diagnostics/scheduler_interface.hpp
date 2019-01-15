// Boost.Asynchronous library
//  Copyright (C) Tobias Holl 2015
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNC_SCHEDULER_INTERFACE_HPP
#define BOOST_ASYNC_SCHEDULER_INTERFACE_HPP

#include <functional>
#include <utility>
#include <vector>

#include <boost/asynchronous/any_shared_scheduler_proxy.hpp>
#include <boost/asynchronous/scheduler_diagnostics.hpp>

namespace boost { namespace asynchronous {

// Allows calling a weak_scheduler's diagnostics methods without knowing its exact type
struct scheduler_interface {
    std::function<boost::asynchronous::scheduler_diagnostics()> get_diagnostics;
    std::function<std::vector<std::size_t>()> get_queue_sizes;
    std::function<boost::asynchronous::scheduler_diagnostics()> clear;
    std::string name;

    template <typename Job>
    scheduler_interface(std::string name_, boost::asynchronous::any_weak_scheduler<Job> const& weak_scheduler)
    {
        // Get diagnostics
        get_diagnostics = [weak_scheduler]() -> scheduler_diagnostics {
            auto locked_scheduler = weak_scheduler.lock();
            if (locked_scheduler.is_valid()) {
                return locked_scheduler.get_diagnostics();
            }
            return scheduler_diagnostics();
        };

        // Get queue sizes
        get_queue_sizes = [weak_scheduler]() -> std::vector<std::size_t>
        {
            auto locked_scheduler = weak_scheduler.lock();
            if (locked_scheduler.is_valid()) {
                return locked_scheduler.get_queue_size();
            }
            return std::vector<std::size_t>();
        };

        // Clear diagnostics
        clear = [weak_scheduler]() -> scheduler_diagnostics
        {
            auto locked_scheduler = weak_scheduler.lock();
            if (locked_scheduler.is_valid()) {
                //TODO: integrate this into clear_diagnostics to reduce the chances of diagnostics being lost inbetween
                scheduler_diagnostics diagnostics = std::forward<scheduler_diagnostics>(locked_scheduler.get_diagnostics());
                locked_scheduler.clear_diagnostics();
                return diagnostics;
            }
            return scheduler_diagnostics();
        };

        // Get name
        name = name_;
    }

    template <typename Job>
    scheduler_interface(boost::asynchronous::any_shared_scheduler_proxy<Job> const& shared_scheduler_proxy)
        : scheduler_interface(shared_scheduler_proxy.get_name(), shared_scheduler_proxy.get_weak_scheduler())
    {}

    scheduler_interface() = default;

    scheduler_interface(scheduler_interface const&) = default;
    scheduler_interface(scheduler_interface && other) = default;
};

namespace detail {

inline void emplace_scheduler_interfaces(std::vector<scheduler_interface> & /* interfaces */) {}

template <typename Scheduler, typename... Args>
inline void emplace_scheduler_interfaces(std::vector<scheduler_interface> & interfaces, Scheduler const& sched, Args const&... args)
{
    interfaces.emplace_back(boost::asynchronous::scheduler_interface(sched));
    boost::asynchronous::detail::emplace_scheduler_interfaces(interfaces, args...);
}

}

template <typename... Args>
inline std::vector<scheduler_interface> make_scheduler_interfaces(Args const&... args)
{
    std::vector<scheduler_interface> interfaces;
    boost::asynchronous::detail::emplace_scheduler_interfaces(interfaces, args...);
    return interfaces;
}

}}

#endif // BOOST_ASYNC_SCHEDULER_INTERFACE_HPP
