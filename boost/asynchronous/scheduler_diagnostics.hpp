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

// Stores total, average, maximum and minimum values, and the most recent value
template <typename T>
struct diagnostics_stats
{
    T total;
    T average;
    T max;
    T min;
    T recent;
};

// Only stores minimum and maximum and the most recent value
template <typename T>
struct diagnostics_stats_unaccumulated
{
    T max;
    T min;
    T recent;
};

// Stores overview statistics for each possible job stage
template <typename T>
struct diagnostics_data
{
    T scheduling;
    T success;
    T failure;
    T interruption;
    T total;
};

// Stores data for an individual running job. This is merely a helper type and does not provide 'merge'.
struct simple_diagnostic_item
{
    std::string job_name;
    boost::chrono::nanoseconds scheduling;
    boost::chrono::nanoseconds execution;
    boost::chrono::nanoseconds total;
    bool failed;
    bool interrupted;
};

// Stores the results of all jobs with the same name.
struct summary_diagnostic_item
{
    using duration_stats = diagnostics_stats<boost::chrono::nanoseconds>;
    using time_stats = diagnostics_stats_unaccumulated<boost::chrono::high_resolution_clock::time_point>;

    diagnostics_data<duration_stats> durations;
    diagnostics_data<time_stats> last_times;
    diagnostics_data<std::size_t> count = {0, 0, 0, 0, 0};

    std::string job_name;

    void merge(diagnostics_data<duration_stats> const& ds, diagnostics_data<time_stats> const& ts, diagnostics_data<std::size_t> const& cs, bool is_new)
    {
        // Update counts
        count.scheduling   += cs.scheduling;
        count.success      += cs.success;
        count.failure      += cs.failure;
        count.interruption += cs.interruption;
        count.total        += cs.total;

        // Update totals

#define UPDATE_TOTAL(key) durations.key.total += ds.key.total;

        UPDATE_TOTAL(scheduling)
        UPDATE_TOTAL(success)
        UPDATE_TOTAL(failure)
        UPDATE_TOTAL(interruption)
        UPDATE_TOTAL(total)

#undef UPDATE_TOTAL

        // Update min, max and last times

#define UPDATE_IF(key, minmax, op) if (is_new || ds.key.minmax op durations.key.minmax) { durations.key.minmax = ds.key.minmax; last_times.key.minmax = ts.key.minmax; }
#define UPDATE_MIN_MAX(key) UPDATE_IF(key, max, >) UPDATE_IF(key, min, <)

        UPDATE_MIN_MAX(scheduling)
        UPDATE_MIN_MAX(success)
        UPDATE_MIN_MAX(failure)
        UPDATE_MIN_MAX(interruption)
        UPDATE_MIN_MAX(total)

#undef UPDATE_MIN_MAX
#undef UPDATE_IF

        // Update recent times

#define UPDATE_RECENT(key) if (is_new || ts.key.recent > last_times.key.recent) { durations.key.recent = ds.key.recent; last_times.key.recent = ts.key.recent; }

        UPDATE_RECENT(scheduling)
        UPDATE_RECENT(success)
        UPDATE_RECENT(failure)
        UPDATE_RECENT(interruption)
        UPDATE_RECENT(total)

#undef UPDATE_RECENT

        // Recalculate averages

#define UPDATE_AVERAGE(key) if (count.key > 0) { durations.key.average = durations.key.total / count.key; } else { durations.key.average = boost::chrono::nanoseconds(0); }

        UPDATE_AVERAGE(scheduling)
        UPDATE_AVERAGE(success)
        UPDATE_AVERAGE(failure)
        UPDATE_AVERAGE(interruption)
        UPDATE_AVERAGE(total)

#undef UPDATE_AVERAGE
    }
};

// Stores summary results without individual job details
struct summary_diagnostics
{
    using duration_stats = diagnostics_stats<boost::chrono::nanoseconds>;
    using time_stats = diagnostics_stats_unaccumulated<boost::chrono::high_resolution_clock::time_point>;

    diagnostics_data<duration_stats> maxima; // We do not use .recent here.
    diagnostics_data<bool> maxima_present;

    bool has_fails = false;
    bool has_interrupts = false;

    std::map<std::string, summary_diagnostic_item> items;

    summary_diagnostics() = default;
    summary_diagnostics(summary_diagnostics &&) = default;
    summary_diagnostics(summary_diagnostics const&) = default;
    summary_diagnostics& operator=(summary_diagnostics &&) = default;
    summary_diagnostics& operator=(summary_diagnostics const&) = default;

    explicit summary_diagnostics(scheduler_diagnostics diagnostics) : summary_diagnostics()
    {
        merge(std::move(diagnostics));
    }

    summary_diagnostics(scheduler_diagnostics diagnostics, std::map<std::string, std::vector<simple_diagnostic_item>> & simple_items) : summary_diagnostics()
    {
        merge(std::move(diagnostics), simple_items);
    }

    // Merge scheduler_diagnostics into this summary
    void merge(scheduler_diagnostics diagnostics, std::map<std::string, std::vector<simple_diagnostic_item>> & simple_items)
    {
        // Only use the diagnostics' totals.
        for (auto it = diagnostics.totals().begin(); it != diagnostics.totals().end(); ++it)
        {
            // Unnamed jobs cannot be logged
            if (it->first.empty()) continue;

            // Calculate totals, minima and maxima for this job
            diagnostics_data<duration_stats> job_durations;
            diagnostics_data<time_stats> job_last_times;
            diagnostics_data<std::size_t> job_count = {0, 0, 0, 0, 0};

            // Collect statistics for this job:
            //  - calculate scheduling time
            //  - calculate time until successful / failed / interrupted execution
            //  - calculate total time
            for (auto& item : it->second)
            {
                boost::chrono::nanoseconds scheduling = item.get_started_time() - item.get_posted_time();
                if (job_count.scheduling == 0 || scheduling > job_durations.scheduling.max) { job_durations.scheduling.max = scheduling; job_last_times.scheduling.max = item.get_started_time(); }
                if (job_count.scheduling == 0 || scheduling < job_durations.scheduling.min) { job_durations.scheduling.min = scheduling; job_last_times.scheduling.min = item.get_started_time(); }
                if (job_count.scheduling == 0 || item.get_posted_time() > job_last_times.scheduling.recent) { job_durations.scheduling.recent = scheduling; job_last_times.scheduling.recent = item.get_started_time(); }
                job_durations.scheduling.total += scheduling;
                ++job_count.scheduling;

                boost::chrono::nanoseconds execution = item.get_finished_time() - item.get_started_time();

                boost::chrono::nanoseconds total = scheduling + execution;
                if (job_count.total == 0 || total > job_durations.total.max) { job_durations.total.max = total; job_last_times.total.max = item.get_finished_time(); }
                if (job_count.total == 0 || total < job_durations.total.min) { job_durations.total.min = total; job_last_times.total.min = item.get_finished_time(); }
                if (job_count.total == 0 || item.get_finished_time() > job_last_times.total.recent) { job_durations.total.recent = total; job_last_times.total.recent = item.get_finished_time(); }
                job_durations.total.total += total;
                ++job_count.total;

                if (item.is_failed())
                {
                    if (job_count.failure == 0 || execution > job_durations.failure.max) { job_durations.failure.max = execution; job_last_times.failure.max = item.get_finished_time(); }
                    if (job_count.failure == 0 || execution < job_durations.failure.min) { job_durations.failure.min = execution; job_last_times.failure.min = item.get_finished_time(); }
                    if (job_count.failure == 0 || item.get_finished_time() > job_last_times.failure.recent) { job_durations.failure.recent = total; job_last_times.failure.recent = item.get_finished_time(); }
                    job_durations.failure.total += execution;
                    ++job_count.failure;
                }
                else if (item.is_interrupted())
                {
                    if (job_count.interruption == 0 || execution > job_durations.interruption.max) { job_durations.interruption.max = execution; job_last_times.interruption.max = item.get_finished_time(); }
                    if (job_count.interruption == 0 || execution < job_durations.interruption.min) { job_durations.interruption.min = execution; job_last_times.interruption.min = item.get_finished_time(); }
                    if (job_count.interruption == 0 || item.get_finished_time() > job_last_times.interruption.recent) { job_durations.interruption.recent = total; job_last_times.interruption.recent = item.get_finished_time(); }
                    job_durations.interruption.total += execution;
                    ++job_count.interruption;
                }
                else
                {
                    if (job_count.success == 0 || execution > job_durations.success.max) { job_durations.success.max = execution; job_last_times.success.max = item.get_finished_time(); }
                    if (job_count.success == 0 || execution < job_durations.success.min) { job_durations.success.min = execution; job_last_times.success.min = item.get_finished_time(); }
                    if (job_count.success == 0 || item.get_finished_time() > job_last_times.success.recent) { job_durations.success.recent = total; job_last_times.success.recent = item.get_finished_time(); }
                    job_durations.success.total += execution;
                    ++job_count.success;
                }

                simple_items[it->first].push_back(simple_diagnostic_item { it->first, scheduling, execution, total, item.is_failed(), item.is_interrupted() });
            }

            // Add to storage
            summary_diagnostic_item& entry = items[it->first];

            // New entries do not have a name yet.
            bool is_new = entry.job_name.empty();

            // Set name
            if (is_new) entry.job_name = it->first;

            // Merge
            entry.merge(job_durations, job_last_times, job_count, is_new);

            // Update global statistics

            has_fails |= (entry.count.failure > 0);
            has_interrupts |= (entry.count.interruption > 0);

            if (entry.count.scheduling > 0)
            {
                if (!maxima_present.scheduling || entry.durations.scheduling.max > maxima.scheduling.max) maxima.scheduling.max = entry.durations.scheduling.max;
                if (!maxima_present.scheduling || entry.durations.scheduling.min > maxima.scheduling.min) maxima.scheduling.min = entry.durations.scheduling.min;
                if (!maxima_present.scheduling || entry.durations.scheduling.average > maxima.scheduling.average) maxima.scheduling.average = entry.durations.scheduling.average;
                if (!maxima_present.scheduling || entry.durations.scheduling.total > maxima.scheduling.total) maxima.scheduling.total = entry.durations.scheduling.total;
                maxima_present.scheduling = true;
            }

            if (entry.count.success > 0)
            {
                if (!maxima_present.success || entry.durations.success.max > maxima.success.max) maxima.success.max = entry.durations.success.max;
                if (!maxima_present.success || entry.durations.success.min > maxima.success.min) maxima.success.min = entry.durations.success.min;
                if (!maxima_present.success || entry.durations.success.average > maxima.success.average) maxima.success.average = entry.durations.success.average;
                if (!maxima_present.success || entry.durations.success.total > maxima.success.total) maxima.success.total = entry.durations.success.total;
                maxima_present.success = true;
            }

            if (entry.count.failure > 0)
            {
                if (!maxima_present.failure || entry.durations.failure.max > maxima.failure.max) maxima.failure.max = entry.durations.failure.max;
                if (!maxima_present.failure || entry.durations.failure.min > maxima.failure.min) maxima.failure.min = entry.durations.failure.min;
                if (!maxima_present.failure || entry.durations.failure.average > maxima.failure.average) maxima.failure.average = entry.durations.failure.average;
                if (!maxima_present.failure || entry.durations.failure.total > maxima.failure.total) maxima.failure.total = entry.durations.failure.total;
                maxima_present.failure = true;
            }

            if (entry.count.interruption > 0)
            {
                if (!maxima_present.interruption || entry.durations.interruption.max > maxima.interruption.max) maxima.interruption.max = entry.durations.interruption.max;
                if (!maxima_present.interruption || entry.durations.interruption.min > maxima.interruption.min) maxima.interruption.min = entry.durations.interruption.min;
                if (!maxima_present.interruption || entry.durations.interruption.average > maxima.interruption.average) maxima.interruption.average = entry.durations.interruption.average;
                if (!maxima_present.interruption || entry.durations.interruption.total > maxima.interruption.total) maxima.interruption.total = entry.durations.interruption.total;
                maxima_present.interruption = true;
            }

            if (entry.count.total > 0)
            {
                if (!maxima_present.total || entry.durations.total.max > maxima.total.max) maxima.total.max = entry.durations.total.max;
                if (!maxima_present.total || entry.durations.total.min > maxima.total.min) maxima.total.min = entry.durations.total.min;
                if (!maxima_present.total || entry.durations.total.average > maxima.total.average) maxima.total.average = entry.durations.total.average;
                if (!maxima_present.total || entry.durations.total.total > maxima.total.total) maxima.total.total = entry.durations.total.total;
                maxima_present.total = true;
            }
        }
    }

    inline void merge(scheduler_diagnostics diagnostics)
    {
        std::map<std::string, std::vector<simple_diagnostic_item>> simple_items;
        merge(std::move(diagnostics), simple_items);
    }

};

// Stores no diagnostics at all. Useful to disable persistent statistics in formatters
struct disable_diagnostics
{
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

