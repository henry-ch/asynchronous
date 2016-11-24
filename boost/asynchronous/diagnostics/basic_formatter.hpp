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

// Convert time (boost::chrono::nanoseconds) to string
std::string format_duration(boost::chrono::nanoseconds const& d)
{
    // Get microsecond ticks
    boost::chrono::microseconds casted = boost::chrono::duration_cast<boost::chrono::microseconds>(d);
    boost::int_least64_t ticks = casted.count();

    // Extract values
    boost::int_least64_t seconds = ticks / 1000000;
    boost::int_least16_t milliseconds = (ticks % 1000000) / 1000;
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
                               std::vector<std::string> && /* names */,
                               std::vector<std::vector<std::size_t>> && /* queue_sizes */,
                               std::vector<scheduler_diagnostics::current_type> && /* running */,
                               std::vector<Current> && /* current */,
                               std::vector<All> && /* all */) = 0;

    std::string format() {
        // Fetch new diagnostics from the current schedulers
        std::vector<scheduler_diagnostics> diagnostics(m_interfaces.size());
        std::vector<std::vector<std::size_t>> queue_sizes(m_interfaces.size());
        std::vector<std::string> names(m_interfaces.size());

        for (std::size_t index = 0; index < m_interfaces.size(); ++index) {
            diagnostics[index] = m_interfaces[index].get_diagnostics();
            queue_sizes[index] = m_interfaces[index].get_queue_sizes();
            names[index] = m_interfaces[index].name;
        }

        // Clone the current diagnostics and merge them with the new data.
        // Also, extract the information on running jobs
        std::vector<Current> current(diagnostics.size());
        std::vector<All> all(diagnostics.size());
        std::vector<scheduler_diagnostics::current_type> running(diagnostics.size());

        for (std::size_t index = 0; index < m_current_diagnostics.size(); ++index) {
            current[index] = m_current_diagnostics[index];
            all[index] = m_all_diagnostics[index];
        }
        for (std::size_t index = 0; index < current.size(); ++index) {
            running[index] = diagnostics[index].current();
            current[index].merge(diagnostics[index]);
            all[index].merge(std::move(diagnostics[index]));
        }

        // Format 'running', 'current' and 'all'
        return format(diagnostics.size(), std::move(names), std::move(queue_sizes), std::move(running), std::move(current), std::move(all));
    }

    // Clearing data

    void clear_schedulers() {
        std::vector<scheduler_diagnostics> diagnostics(m_interfaces.size());
        // Separate loops to make sure the diagnostics are fetched as closely to one another as possible
        // Of course, this does not prohibit the compiler from joining the loops, but it may serve as a hint...
        for (std::size_t index = 0; index < m_interfaces.size(); ++index) {
            diagnostics[index] = m_interfaces[index].clear();
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
