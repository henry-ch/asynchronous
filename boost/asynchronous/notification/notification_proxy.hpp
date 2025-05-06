// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2024
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNCHRONOUS_NOTIFICATION_PROXY_HPP
#define BOOST_ASYNCHRONOUS_NOTIFICATION_PROXY_HPP

#include <vector>
#include <algorithm>

#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include <boost/asynchronous/servant_proxy.hpp>
#include <boost/asynchronous/trackable_servant.hpp>


// the task of notification servant and proxy is to distribute (de)registrations among schedulers within an application. 
// Using TLS, it can be accessed from within the schedulers.
// It is also possible to have several instances of them in order to create different event buses.

namespace boost { namespace asynchronous { namespace subscription
{
    // TODO make template with job type?
    struct notification_servant : boost::asynchronous::trackable_servant<>
    {
        struct scheduler_notify_entry_t
        {
            std::vector<boost::thread::id>                   m_scheduler_thread_ids;
            std::function<void(std::function<void()>)>       m_exec_in_scheduler_fct;
            std::function<void(
                std::vector<
                std::function<void(
                    std::function<void()>)>>)>               m_notify_me_for_new_schedulers;
        };

        template <class Threadpool>
        notification_servant(boost::asynchronous::any_weak_scheduler<> scheduler, Threadpool p)
            : boost::asynchronous::trackable_servant<>(scheduler, p)
        {
        }

        void add_scheduler(std::vector<boost::thread::id> sched_thread_ids, 
                           std::function<void(std::function<void()>)> exec_in_sched,
                           std::function<void(std::vector< std::function<void(std::function<void()>)>>)> notify_me_for_new_schedulers)
        {
            // add newcomer
            m_schedulers.emplace_back(std::move(sched_thread_ids), std::move(exec_in_sched), std::move(notify_me_for_new_schedulers));

            // send newcomer to all already known schedulers
            for (auto& s : m_schedulers)
            {
                // do not send scheduler to himself
                auto schedulers_minus_self = m_schedulers;
                schedulers_minus_self.erase(
                    std::remove_if(schedulers_minus_self.begin(), schedulers_minus_self.end(), [tids= s.m_scheduler_thread_ids](auto const& entry) 
                        {
                            return entry.m_scheduler_thread_ids == tids;
                        }),
                    schedulers_minus_self.end());
                // remove internal data
                std::vector< std::function<void(std::function<void()>)>> others(schedulers_minus_self.size());
                std::transform(schedulers_minus_self.cbegin(), schedulers_minus_self.cend(), others.begin(), [](auto const& os)
                    {
                        return os.m_exec_in_scheduler_fct;
                    });
                if (!others.empty())
                {
                    s.m_notify_me_for_new_schedulers(std::move(others));
                }
            }
        }

    private:
        std::vector<notification_servant::scheduler_notify_entry_t> m_schedulers;
    };

    // TODO make template with job type?
    class notification_proxy : public boost::asynchronous::servant_proxy<notification_proxy, notification_servant>
    {
    public:
        template <class Scheduler, class Threadpool>
        notification_proxy(Scheduler s, Threadpool p) :
            boost::asynchronous::servant_proxy<notification_proxy, notification_servant>(s, p)
        {}
        BOOST_ASYNC_POST_MEMBER_LOG(add_scheduler, "add_scheduler")
    };


    // register a scheduler to a notification proxy, effectively adding it to its "event bus"
    void register_scheduler_to_notification(auto wsched, auto notification_ptr)
    {
        auto f = [wsched](std::function<void()> fct)
            {
                auto sched = wsched.lock();
                if (sched.is_valid())
                {
                    sched.post([fct = std::move(fct)]()
                        {
                            fct();
                        });
                }
            };

        // TODO move to scheduler impl
        std::function<void(std::vector< std::function<void(std::function<void()>)>>)> notify_me_for_new_schedulers =
            [wsched](std::vector< std::function<void(std::function<void()>)>> others)
            {
                auto sched = wsched.lock();
                if (sched.is_valid())
                {
                    sched.post([others = std::move(others)]() mutable
                        {
                            boost::asynchronous::subscription::other_schedulers_ = std::move(others);
                        });
                }
            };
        notification_ptr->add_scheduler(wsched.lock().thread_ids(), f, notify_me_for_new_schedulers);
    }

}}}
#endif // BOOST_ASYNCHRONOUS_NOTIFICATION_PROXY_HPP
