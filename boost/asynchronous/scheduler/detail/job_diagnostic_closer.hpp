// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2013
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNCHRON_SCHEDULER_JOB_DIAGNOSTIC_CLOSER_HPP
#define BOOST_ASYNCHRON_SCHEDULER_JOB_DIAGNOSTIC_CLOSER_HPP

#include <boost/asynchronous/job_traits.hpp>

namespace boost { namespace asynchronous { namespace detail
{
// upon destruction, will ensure that a job is correctly logged
template <class Job, class Diag>
struct job_diagnostic_closer
{
    job_diagnostic_closer(Job* job, Diag* diag): m_job(job), m_diag(diag){}
    ~job_diagnostic_closer()
    {
        boost::asynchronous::job_traits<Job>::set_finished_time(*m_job);
        boost::asynchronous::job_traits<Job>::add_diagnostic(*m_job,m_diag);
    }

private:
    Job*  m_job;
    Diag* m_diag;
};
}}} // boost::asynchronous::detail

#endif // BOOST_ASYNCHRON_SCHEDULER_JOB_DIAGNOSTIC_CLOSER_HPP
