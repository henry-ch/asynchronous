// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2015
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
template <class Job>
struct scheduler_diagnostics
{
    typedef std::map<std::string,std::list<typename boost::asynchronous::job_traits<Job>::diagnostic_item_type>> total_type;
    typedef std::vector<std::pair<std::string,typename boost::asynchronous::job_traits<Job>::diagnostic_item_type>> current_type;

    scheduler_diagnostics(total_type const& t, current_type const& c)
        : m_totals(t), m_current(c)
    {}
    scheduler_diagnostics() = default;
    scheduler_diagnostics (scheduler_diagnostics&&)=default;
    scheduler_diagnostics (scheduler_diagnostics const&)=default;
    scheduler_diagnostics& operator=(scheduler_diagnostics const&)=default;
    scheduler_diagnostics& operator=(scheduler_diagnostics&&)=default;

    total_type totals() const
    {
        return m_totals;
    }
    current_type current() const
    {
        return m_current;
    }
    void merge(scheduler_diagnostics<Job>&& other)
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

}}
#endif // BOOST_ASYNCHRONOUS_SCHEDULER_DIAGNOSTICS_HPP

