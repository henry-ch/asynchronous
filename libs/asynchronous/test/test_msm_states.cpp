
// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2013
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org
#include <boost/asynchronous/scheduler/single_thread_scheduler.hpp>
#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include <boost/asynchronous/scheduler/threadpool_scheduler.hpp>
#include <boost/asynchronous/queue/guarded_deque.hpp>
#include <boost/asynchronous/servant_proxy.hpp>
#include <boost/asynchronous/trackable_servant.hpp>
#include <boost/asynchronous/helpers/recursive_future_get.hpp>
#include <boost/asynchronous/extensions/msm/state.hpp>

#include <boost/msm/front/states.hpp>
// back-end
#include <boost/msm/back11/state_machine.hpp>
//front-end
#include <boost/msm/front/state_machine_def.hpp>
// functors
#include <boost/msm/front/functor_row.hpp>



#include "test_common.hpp"

#include <boost/test/unit_test.hpp>
using namespace boost::asynchronous::test;
namespace msm = boost::msm;
namespace mpl = boost::mpl;
using namespace msm::front;

namespace
{
// main thread id
boost::thread::id main_thread_id;
bool servant_dtor = false;


struct event1 {};
struct event2 {};
struct fsm_ : public boost::asynchronous::msm::state<msm::front::state_machine_def<fsm_>>
{
    struct publish_event2
    {
        template <class EVT, class FSM, class SourceState, class TargetState>
        void operator()(EVT const&, FSM&, SourceState& src, TargetState&)
        {
            src.publish(event2{});
        }
    };

    template <class Event, class FSM>
    void on_entry(Event const&, FSM& fsm)
    {
        this->subscribe(
            [&fsm](event1 const& e) mutable
            {
                fsm.process_event(e);
            });
    }

    struct State1 : public boost::asynchronous::msm::state<boost::msm::front::state<>>
    {
        template <class Event, class FSM>
        void on_entry(Event const&, FSM&) { ++entry_counter; }
        template <class Event, class FSM>
        void on_exit(Event const&, FSM&) { ++exit_counter; }
        int entry_counter=0;
        int exit_counter=0;
    };
    struct State2 : public boost::asynchronous::msm::state<boost::msm::front::state<>>
    {
        template <class Event, class FSM>
        void on_entry(Event const&, FSM& fsm) 
        { 
            ++entry_counter; 
            m_token = this->subscribe(
                [&fsm](event2 const& e) mutable
                {
                    fsm.process_event(e);
                });
        }
        template <class Event, class FSM>
        void on_exit(Event const&, FSM&) 
        {
            unsubscribe<event1>(m_token);
            ++exit_counter; 
        }
        int entry_counter=0;
        int exit_counter=0;
        boost::asynchronous::subscription_token m_token;
    };
    typedef State1 initial_state;

    // Transition table for player
        struct transition_table : boost::fusion::vector<
            //    Start     Event         Next      Action               Guard
            //  +---------+-------------+---------+---------------------+----------------------+
            Row < State1  , event1      , State2  , none                , none                 >,
            Row < State2  , event1      , none    , publish_event2      , none                 >,
            Row < State2  , event2      , State1  , none                , none                 >
            //  +---------+-------------+---------+---------------------+----------------------+
        > {};
        // Replaces the default no-transition response.
        template <class FSM,class Event>
        void no_transition(Event const&, FSM&,int)
        {
            BOOST_FAIL("no_transition called!");
        }
};
using fsm = msm::back11::state_machine<fsm_>;

struct Servant : boost::asynchronous::trackable_servant<>
{
    template <class Threadpool>
    Servant(boost::asynchronous::any_weak_scheduler<> scheduler, Threadpool p)
        : boost::asynchronous::trackable_servant<>(scheduler, p)
    {
    }

    auto process_event(auto ev)
    {
        return m_fsm.process_event(ev);
    }

    auto publish_event(auto ev)
    {
        return m_fsm.publish(ev);
    }

    void start()
    {
        m_fsm.start();
    }

    void test()
    {
        BOOST_CHECK_MESSAGE(m_fsm.current_state()[0] == 0, "State1 should be active");
        BOOST_CHECK_MESSAGE(m_fsm.get_state<fsm_::State1&>().exit_counter == 1, "State1 exit not called correctly");
        BOOST_CHECK_MESSAGE(m_fsm.get_state<fsm_::State1&>().entry_counter == 2, "State1 entry not called correctly");
        BOOST_CHECK_MESSAGE(m_fsm.get_state<fsm_::State2&>().entry_counter == 1, "State2 entry not called correctly");
    }

    fsm m_fsm;
};

class ServantProxy : public boost::asynchronous::servant_proxy<ServantProxy, Servant>
{
public:
    template <class Scheduler, class Threadpool>
    ServantProxy(Scheduler s, Threadpool p) :
        boost::asynchronous::servant_proxy<ServantProxy, Servant>(s, p)
    {
    }
    BOOST_ASYNC_FUTURE_MEMBER(process_event)
    BOOST_ASYNC_FUTURE_MEMBER(publish_event)
    BOOST_ASYNC_FUTURE_MEMBER(start)
    BOOST_ASYNC_FUTURE_MEMBER(test)
};

}

BOOST_AUTO_TEST_CASE( test_servant_states )
{
    auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::single_thread_scheduler<
        boost::asynchronous::guarded_deque<>>>();
    auto pool = boost::asynchronous::make_shared_scheduler_proxy<boost::asynchronous::threadpool_scheduler<
        boost::asynchronous::guarded_deque<>>>(3);
    
    ServantProxy proxy(scheduler, pool);
    proxy.start();
    proxy.process_event(event1{}).get();
    proxy.publish_event(event1{}).get();
    proxy.test();
}

