// Boost.Asynchronous library
//  Copyright (C) Tobias Holl 2016
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNCHRONOUS_BASIC_FORMATTER_HPP
#define BOOST_ASYNCHRONOUS_BASIC_FORMATTER_HPP

#include <sstream>
#include <vector>
#include <chrono>
#include <iomanip>

#include <boost/core/enable_if.hpp>

#include <boost/asynchronous/scheduler_diagnostics.hpp>
#include <boost/asynchronous/diagnostics/scheduler_interface.hpp>

namespace boost { namespace asynchronous {

namespace formatting {

// RGB colors
struct color
{
    std::uint8_t r;
    std::uint8_t g;
    std::uint8_t b;

    std::string to_hex() const
    {
        std::stringstream stream;
        stream << "#" << std::hex << std::setfill('0')
               << std::setw(2) << (int) r
               << std::setw(2) << (int) g
               << std::setw(2) << (int) b;
        return stream.str();
    }
};

// Convert time (std::chrono::nanoseconds) to string
inline std::string format_duration(std::chrono::nanoseconds const& d)
{
    // Get microsecond ticks
    std::chrono::microseconds casted = std::chrono::duration_cast<std::chrono::microseconds>(d);
    boost::int_least64_t ticks = casted.count();

    // Extract values
    boost::int_least64_t seconds = ticks / 1000000;
    boost::int_least16_t milliseconds = (boost::int_least16_t)((double)(ticks % 1000000) / 1000);
    boost::int_least16_t microseconds = ticks % 1000;

    // Convert to string and return
    std::stringstream stream;
    stream << std::setfill('0');
    if (seconds > 0) stream << seconds << "." << std::setw(3);
    if (seconds > 0 || milliseconds > 0) stream << milliseconds << "." << std::setw(3);
    stream << microseconds;
    return stream.str();
}

}

// Diagnostic types must be default-constructible and copy-assignable
// Diagnostic types must offer 'merge(boost::asynchronous::scheduler_diagnostics)'
//
template <typename Current = scheduler_diagnostics,
          typename All = summary_diagnostics>
class basic_formatter {
protected:
    std::vector<Current> m_current_diagnostics;
    std::vector<All> m_all_diagnostics;

    std::vector<scheduler_interface> m_interfaces;

public:

    // Constructors

    basic_formatter() {}
    basic_formatter(std::vector<boost::asynchronous::scheduler_interface> interfaces)
        : m_interfaces(std::move(interfaces))
    {}

    // Formatting

    virtual std::string format(std::size_t /* count */,
                               std::vector<std::string> const& /* names */,
                               std::vector<std::vector<std::size_t>> const& /* queue_sizes */,
                               std::vector<scheduler_diagnostics::current_type> const& /* running */,
                               std::vector<Current> const& /* current */,
                               std::vector<All> const& /* all */) = 0;


    std::string format() {
        // Fetch new diagnostics from the current schedulers
        std::vector<scheduler_diagnostics> diagnostics(m_interfaces.size());
        std::vector<scheduler_diagnostics::current_type> running(m_interfaces.size());
        std::vector<std::vector<std::size_t>> queue_sizes(m_interfaces.size());
        std::vector<std::string> names(m_interfaces.size());

        for (std::size_t index = 0; index < m_interfaces.size(); ++index) 
        {
            diagnostics[index] = m_interfaces[index].get_and_clear();
            // Also, extract the information on running jobs
            running[index] = diagnostics[index].current();

            // update internal
            if (m_current_diagnostics.size() < m_interfaces.size())
            {
                m_current_diagnostics.resize(m_interfaces.size());
            }
            if (m_all_diagnostics.size() < m_interfaces.size())
            {
                m_all_diagnostics.resize(m_interfaces.size());
            }
            // Merge the current diagnostics with the new data.
            m_current_diagnostics[index].merge(diagnostics[index]);
            m_all_diagnostics[index].merge(diagnostics[index]);
            queue_sizes[index] = m_interfaces[index].get_queue_sizes();
            names[index] = m_interfaces[index].name;
        }

        // Format 'running', 'current' and 'all'
        return format(diagnostics.size(), names, queue_sizes, running, m_current_diagnostics, m_all_diagnostics);
    }

    // Registers an additional scheduler with the formatter
    void register_scheduler(boost::asynchronous::scheduler_interface interface)
    {
        m_interfaces.push_back(std::move(interface));
    }

    // Clearing data

    void clear_schedulers() {
        std::vector<scheduler_diagnostics> diagnostics(m_interfaces.size());
        // Separate loops to make sure the diagnostics are fetched as closely to one another as possible
        // Of course, this does not prohibit the compiler from joining the loops, but it may serve as a hint...
        for (std::size_t index = 0; index < m_interfaces.size(); ++index) {
            diagnostics[index] = m_interfaces[index].get_and_clear();
        }
        // Resize storage as needed
        if (m_current_diagnostics.size() < diagnostics.size()) m_current_diagnostics.resize(diagnostics.size());
        if (m_all_diagnostics.size() < diagnostics.size()) m_all_diagnostics.resize(diagnostics.size());
        // Merge data into the local storage
        for (std::size_t index = 0; index < diagnostics.size(); ++index) {
            m_current_diagnostics[index].merge(diagnostics[index]);
            m_all_diagnostics[index].merge(std::move(diagnostics[index]));
        }
    }

    void clear_current() {
        m_current_diagnostics.clear();
    }

    void clear_all() {
        m_current_diagnostics.clear();
        m_all_diagnostics.clear();
    }
};

}}

#endif // BOOST_ASYNCHRONOUS_BASIC_FORMATTER_HPP

