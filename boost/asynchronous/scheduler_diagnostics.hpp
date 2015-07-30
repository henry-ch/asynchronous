// Boost.Asynchronous library
//  Copyright (C) Christophe Henry, Tobias Holl 2015
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNCHRONOUS_SCHEDULER_DIAGNOSTICS_HPP
#define BOOST_ASYNCHRONOUS_SCHEDULER_DIAGNOSTICS_HPP

#include <map>
#include <list>
#include <boost/asynchronous/diagnostics/diagnostic_item.hpp>
#include <boost/asynchronous/job_traits.hpp>

namespace boost { namespace asynchronous
{
struct scheduler_diagnostics
{
    typedef std::map<std::string,std::list<boost::asynchronous::diagnostic_item>> total_type;
    typedef std::vector<std::pair<std::string, boost::asynchronous::diagnostic_item>> current_type;

    scheduler_diagnostics(total_type const& t, current_type const& c)
        : m_totals(t), m_current(c)
    {}
    scheduler_diagnostics() = default;
    scheduler_diagnostics (scheduler_diagnostics&&)=default;
    scheduler_diagnostics (scheduler_diagnostics const&)=default;
    scheduler_diagnostics& operator=(scheduler_diagnostics const&)=default;
    scheduler_diagnostics& operator=(scheduler_diagnostics&&)=default;

    total_type const& totals() const
    {
        return m_totals;
    }
    current_type const& current() const
    {
        return m_current;
    }
    void merge(scheduler_diagnostics other)
    {
        for (auto const& diag : other.m_totals)
        {
            auto it = m_totals.find(diag.first);
            if (it == m_totals.end())
            {
                // we don't have this one, add it
                m_totals.insert(diag);
            }
            else
            {
                // merge results for this job
                (*it).second.insert((*it).second.end(),diag.second.begin(),diag.second.end());
            }
        }
        m_current.insert(m_current.end(),other.m_current.begin(),other.m_current.end());
    }

private:
    total_type m_totals;
    current_type m_current;
};

struct register_diagnostics_type
{
    // placeholder for later (register atshutdown, or intervals)
};

// Stores data for an individual running job. This is merely a helper type and does not provide 'merge'.
struct simple_diagnostic_item {
    std::string job_name;
    boost::chrono::nanoseconds scheduling;
    boost::chrono::nanoseconds execution;
    boost::chrono::nanoseconds total;
    bool failed;
    bool interrupted;
};

// Stores the results of all jobs with the same name.
struct summary_diagnostic_item {
    std::string job_name;

    boost::chrono::nanoseconds scheduling_total;
    boost::chrono::nanoseconds scheduling_average;
    boost::chrono::nanoseconds scheduling_max;
    boost::chrono::nanoseconds scheduling_min;

    boost::chrono::nanoseconds execution_total;
    boost::chrono::nanoseconds execution_average;
    boost::chrono::nanoseconds execution_max;
    boost::chrono::nanoseconds execution_min;

    boost::chrono::nanoseconds failure_total;
    boost::chrono::nanoseconds failure_average;
    boost::chrono::nanoseconds failure_max;
    boost::chrono::nanoseconds failure_min;

    boost::chrono::nanoseconds interrupted_total;
    boost::chrono::nanoseconds interrupted_average;
    boost::chrono::nanoseconds interrupted_max;
    boost::chrono::nanoseconds interrupted_min;

    boost::chrono::nanoseconds total_total;
    boost::chrono::nanoseconds total_average;
    boost::chrono::nanoseconds total_max;
    boost::chrono::nanoseconds total_min;

    std::size_t scheduled;
    std::size_t successful;
    std::size_t interrupted;
    std::size_t failed;
    std::size_t count;
};

// Stores summary results without individual job details
struct summary_diagnostics {
    boost::chrono::nanoseconds max_scheduling_total;
    boost::chrono::nanoseconds max_scheduling_average;
    boost::chrono::nanoseconds max_scheduling_max;
    boost::chrono::nanoseconds max_scheduling_min;
    bool scheduling_maxima_set = false;

    boost::chrono::nanoseconds max_execution_total;
    boost::chrono::nanoseconds max_execution_average;
    boost::chrono::nanoseconds max_execution_max;
    boost::chrono::nanoseconds max_execution_min;
    bool execution_maxima_set = false;

    boost::chrono::nanoseconds max_failure_total;
    boost::chrono::nanoseconds max_failure_average;
    boost::chrono::nanoseconds max_failure_max;
    boost::chrono::nanoseconds max_failure_min;
    bool failure_maxima_set = false;

    boost::chrono::nanoseconds max_interrupted_total;
    boost::chrono::nanoseconds max_interrupted_average;
    boost::chrono::nanoseconds max_interrupted_max;
    boost::chrono::nanoseconds max_interrupted_min;
    bool interrupted_maxima_set = false;

    boost::chrono::nanoseconds max_total_total;
    boost::chrono::nanoseconds max_total_average;
    boost::chrono::nanoseconds max_total_max;
    boost::chrono::nanoseconds max_total_min;
    bool total_maxima_set = false;

    bool has_fails = false;
    bool has_interrupts = false;

    std::map<std::string, summary_diagnostic_item> items;

    summary_diagnostics() = default;
    summary_diagnostics(summary_diagnostics &&) = default;
    summary_diagnostics(summary_diagnostics const&) = default;
    summary_diagnostics& operator=(summary_diagnostics &&) = default;
    summary_diagnostics& operator=(summary_diagnostics const&) = default;

    explicit summary_diagnostics(scheduler_diagnostics diagnostics) : summary_diagnostics() {
        merge(std::move(diagnostics));
    }

    summary_diagnostics(scheduler_diagnostics diagnostics, std::map<std::string, std::vector<simple_diagnostic_item>> & simple_items) : summary_diagnostics() {
        merge(std::move(diagnostics), simple_items);
    }

    // Merge scheduler_diagnostics into this summary
    void merge(scheduler_diagnostics diagnostics, std::map<std::string, std::vector<simple_diagnostic_item>> & simple_items) {
        // Only use the diagnostics' totals.
        for (auto it = diagnostics.totals().begin(); it != diagnostics.totals().end(); ++it) {
            // Unnamed jobs cannot be logged
            if (it->first.empty()) continue;

            // Calculate totals, minima and maxima for this job
            bool extrema_set = false;

            boost::chrono::nanoseconds scheduling_total;
            boost::chrono::nanoseconds scheduling_max;
            boost::chrono::nanoseconds scheduling_min;

            boost::chrono::nanoseconds execution_total;
            boost::chrono::nanoseconds execution_max;
            boost::chrono::nanoseconds execution_min;

            boost::chrono::nanoseconds failure_total;
            boost::chrono::nanoseconds failure_max;
            boost::chrono::nanoseconds failure_min;

            boost::chrono::nanoseconds interrupted_total;
            boost::chrono::nanoseconds interrupted_max;
            boost::chrono::nanoseconds interrupted_min;

            boost::chrono::nanoseconds total_total;
            boost::chrono::nanoseconds total_max;
            boost::chrono::nanoseconds total_min;

            std::size_t scheduled = 0;
            std::size_t successful = 0;
            std::size_t failed = 0;
            std::size_t interrupted = 0;

            // Collect statistics for this job:
            //  - calculate scheduling time
            //  - calculate time until successful / failed / interrupted execution
            //  - calculate total time
            for (auto& item : it->second) {
                boost::chrono::nanoseconds scheduling = item.get_started_time() - item.get_posted_time();
                if (!extrema_set || scheduling > scheduling_max) scheduling_max = scheduling;
                if (!extrema_set || scheduling < scheduling_min) scheduling_min = scheduling;
                scheduling_total += scheduling;
                ++scheduled;

                boost::chrono::nanoseconds execution = item.get_finished_time() - item.get_started_time();

                boost::chrono::nanoseconds total = scheduling + execution;
                if (!extrema_set || total > total_max) total_max = total;
                if (!extrema_set || total < total_min) total_min = total;
                total_total += total;

                if (item.is_failed()) {
                    if (!extrema_set || execution > failure_max) failure_max = execution;
                    if (!extrema_set || execution < failure_min) failure_min = execution;
                    failure_total += execution;
                    ++failed;
                } else if (item.is_interrupted()) {
                    if (!extrema_set || execution > interrupted_max) interrupted_max = execution;
                    if (!extrema_set || execution < interrupted_min) interrupted_min = execution;
                    interrupted_total += execution;
                    ++interrupted;
                } else {
                    if (!extrema_set || execution > execution_max) execution_max = execution;
                    if (!extrema_set || execution < execution_min) execution_min = execution;
                    execution_total += execution;
                    ++successful;
                }

                simple_items[it->first].push_back(simple_diagnostic_item { it->first, scheduling, execution, total, item.is_failed(), item.is_interrupted() });

                extrema_set = true;
            }

            // Add to storage

            summary_diagnostic_item& entry = items[it->first];

            // New entries do not have a name yet.
            bool is_new = entry.job_name.empty();

            // Set name and values

            if (is_new) entry.job_name = it->first;

            entry.scheduling_total += scheduling_total;
            if (is_new || scheduling_max > entry.scheduling_max) entry.scheduling_max = scheduling_max;
            if (is_new || scheduling_min < entry.scheduling_min) entry.scheduling_min = scheduling_min;

            entry.execution_total += execution_total;
            if (is_new || execution_max > entry.execution_max) entry.execution_max = execution_max;
            if (is_new || execution_min < entry.execution_min) entry.execution_min = execution_min;

            entry.failure_total += failure_total;
            if (is_new || failure_max > entry.failure_max) entry.failure_max = failure_max;
            if (is_new || failure_min < entry.failure_min) entry.failure_min = failure_min;

            entry.interrupted_total += interrupted_total;
            if (is_new || interrupted_max > entry.interrupted_max) entry.interrupted_max = interrupted_max;
            if (is_new || interrupted_min < entry.interrupted_min) entry.interrupted_min = interrupted_min;

            entry.total_total += total_total;
            if (is_new || total_max > entry.total_max) entry.total_max = total_max;
            if (is_new || total_min < entry.total_min) entry.total_min = total_min;

            entry.scheduled += scheduled;
            entry.count += scheduled;
            entry.successful += successful;
            entry.failed += failed;
            entry.interrupted += interrupted;

            // Recalculate averages

            if (entry.scheduled != 0) {
                entry.scheduling_average = entry.scheduling_total / entry.scheduled;
                entry.total_average = entry.total_total / entry.scheduled;
            } else continue; // There were no scheduled jobs of this type. This is impossible, some sort of error must have occurred. Skip this job.

            if (entry.successful != 0) entry.execution_average = entry.execution_total / entry.successful;
            else entry.execution_average = boost::chrono::nanoseconds(0);

            if (entry.failed != 0) entry.failure_average = entry.failure_total / entry.failed;
            else entry.failure_average = boost::chrono::nanoseconds(0);

            if (entry.interrupted != 0) entry.interrupted_average = entry.interrupted_total / entry.interrupted;
            else entry.interrupted_average = boost::chrono::nanoseconds(0);

            // Update global statistics

            has_fails |= (entry.failed > 0);
            has_interrupts |= (entry.interrupted > 0);

            if (entry.scheduled > 0) {
                if (!scheduling_maxima_set || entry.scheduling_max > max_scheduling_max) max_scheduling_max = entry.scheduling_max;
                if (!scheduling_maxima_set || entry.scheduling_min > max_scheduling_min) max_scheduling_min = entry.scheduling_min;
                if (!scheduling_maxima_set || entry.scheduling_average > max_scheduling_average) max_scheduling_average = entry.scheduling_average;
                if (!scheduling_maxima_set || entry.scheduling_total > max_scheduling_total) max_scheduling_total = entry.scheduling_total;
                scheduling_maxima_set = true;
            }

            if (entry.successful > 0) {
                if (!execution_maxima_set || entry.execution_max > max_execution_max) max_execution_max = entry.execution_max;
                if (!execution_maxima_set || entry.execution_min > max_execution_min) max_execution_min = entry.execution_min;
                if (!execution_maxima_set || entry.execution_average > max_execution_average) max_execution_average = entry.execution_average;
                if (!execution_maxima_set || entry.execution_total > max_execution_total) max_execution_total = entry.execution_total;
                execution_maxima_set = true;
            }

            if (entry.failed > 0) {
                if (!failure_maxima_set || entry.failure_max > max_failure_max) max_failure_max = entry.failure_max;
                if (!failure_maxima_set || entry.failure_min > max_failure_min) max_failure_min = entry.failure_min;
                if (!failure_maxima_set || entry.failure_average > max_failure_average) max_failure_average = entry.failure_average;
                if (!failure_maxima_set || entry.failure_total > max_failure_total) max_failure_total = entry.failure_total;
                failure_maxima_set = true;
            }

            if (entry.interrupted > 0) {
                if (!interrupted_maxima_set || entry.interrupted_max > max_interrupted_max) max_interrupted_max = entry.interrupted_max;
                if (!interrupted_maxima_set || entry.interrupted_min > max_interrupted_min) max_interrupted_min = entry.interrupted_min;
                if (!interrupted_maxima_set || entry.interrupted_average > max_interrupted_average) max_interrupted_average = entry.interrupted_average;
                if (!interrupted_maxima_set || entry.interrupted_total > max_interrupted_total) max_interrupted_total = entry.interrupted_total;
                interrupted_maxima_set = true;
            }

            if (entry.count > 0) {
                if (!total_maxima_set || entry.total_max > max_total_max) max_total_max = entry.total_max;
                if (!total_maxima_set || entry.total_min > max_total_min) max_total_min = entry.total_min;
                if (!total_maxima_set || entry.total_average > max_total_average) max_total_average = entry.total_average;
                if (!total_maxima_set || entry.total_total > max_total_total) max_total_total = entry.total_total;
                total_maxima_set = true;
            }
        }
    }

    inline void merge(scheduler_diagnostics diagnostics) {
        std::map<std::string, std::vector<simple_diagnostic_item>> simple_items;
        merge(std::move(diagnostics), simple_items);
    }

};

// Stores no diagnostics at all. Useful to disable persistent statistics in formatters
struct disable_diagnostics {
    disable_diagnostics() = default;
    disable_diagnostics(disable_diagnostics &&) = default;
    disable_diagnostics(disable_diagnostics const&) = default;
    disable_diagnostics& operator=(disable_diagnostics &&) = default;
    disable_diagnostics& operator=(disable_diagnostics const&) = default;

    explicit disable_diagnostics(scheduler_diagnostics) {}

    void merge(scheduler_diagnostics) {}
};

}}
#endif // BOOST_ASYNCHRONOUS_SCHEDULER_DIAGNOSTICS_HPP

