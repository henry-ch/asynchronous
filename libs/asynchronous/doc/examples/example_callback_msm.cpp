// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2016
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org
#include <iostream>
#include <thread>
#include <boost/asynchronous/scheduler/single_thread_scheduler.hpp>
#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include <boost/asynchronous/scheduler/multiqueue_threadpool_scheduler.hpp>

#include <boost/asynchronous/servant_proxy.hpp>
#include <boost/asynchronous/trackable_servant.hpp>

// msm back-end
#include <boost/msm/back/state_machine.hpp>
// msm front-end
#include <boost/msm/front/state_machine_def.hpp>
// functors
#include <boost/msm/front/functor_row.hpp>
#include <boost/msm/front/euml/common.hpp>

using namespace std;
namespace msm = boost::msm;
namespace mpl = boost::mpl;
using namespace msm::front;
using namespace boost::msm::front::euml;

namespace
{
// state machine definition
// events
struct toggle {};
struct init {};
struct part2 {};

// actions
struct start_first_task
{
    template <class FSM,class EVT,class SourceState,class TargetState>
    void operator()(EVT const& , FSM& fsm,SourceState& ,TargetState& )
    {
        cout << "post first task to threadpool" << endl;
        // start long tasks in threadpool (first lambda) and callback in our thread
        fsm.post_callback(
                []()
                {
                     cout << "execute first task" << endl;
                     // simulate long task
                     boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
                },
                // callback
                [&fsm](boost::asynchronous::expected<void> /*res*/)
                {
                    // we are in the fsm thread, so it's safe to use it
                    // this event will try to start second part (if SecondTaskOk is active)
                    fsm.template process_event(part2());
                }
        );
    }
};
struct start_second_task
{
    template <class FSM,class EVT,class SourceState,class TargetState>
    void operator()(EVT const& , FSM& fsm,SourceState& ,TargetState& )
    {
        // this will probably never be called as our example cancels immediately
        fsm.post_callback(
                []()
                {
                     cout << "execute second task" << endl;
                     // simulate long task
                     boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
                },
                // callback
                [&fsm](boost::asynchronous::expected<void> /*res*/)
                {
                    // ignore expected as no exception will be thrown
                    cout << "shutdown servant after completion" <<endl;
                    fsm.m_promise.set_value();
                }
        );
    }
};
struct done_no_second_task
{
    template <class FSM,class EVT,class SourceState,class TargetState>
    void operator()(EVT const& , FSM& fsm,SourceState& ,TargetState& )
    {
        // force shutdown (inform main)
        cout << "shutdown servant without executing second task" <<endl;
        fsm.m_promise.set_value();
    }
};
// The list of FSM states
struct Init : public msm::front::state<>{};
struct SecondTaskOk : public msm::front::state<>{};
struct SecondTaskNok : public msm::front::state<>{};

// fsm front-end
struct Fsm_ : public msm::front::state_machine_def<Fsm_>, public boost::asynchronous::trackable_servant<>
{
    Fsm_(boost::asynchronous::any_weak_scheduler<> scheduler,
         boost::asynchronous::any_shared_scheduler_proxy<> worker)
        : boost::asynchronous::trackable_servant<>(scheduler,worker){}

    // the initial state of the player SM. Must be defined
    typedef Init initial_state;

    // Transition table for player
    struct transition_table : mpl::vector5<
      //    Start             Event         Target           Action                     Guard
      //  +------------------+-------------+----------------+--------------------------+-------------------+
      Row < Init             , init        , SecondTaskNok  , start_first_task         , none             >,
      Row < SecondTaskNok    , toggle      , SecondTaskOk   , none                     , none             >,
      Row < SecondTaskNok    , part2       , none           , done_no_second_task      , none             >,
      Row < SecondTaskOk     , toggle      , SecondTaskNok  , none                     , none             >,
      Row < SecondTaskOk     , part2       , none           , start_second_task        , none             >
      //  +------------------+-------------+----------------+--------------------------+-------------------+
    >{};
    // Replaces the default no-transition response.
    template <class FSM,class Event>
    void no_transition(Event const& , FSM&,int state)
    {
        std::cout << "no transition from state " << state << " on event " << typeid(Event).name() << std::endl;
    }
    // to signal that we shutdown
    boost::promise<void> m_promise;
};
struct Fsm : public msm::back::state_machine<Fsm_>
{
    Fsm(boost::asynchronous::any_weak_scheduler<> scheduler,
        boost::asynchronous::any_shared_scheduler_proxy<> worker)
       : msm::back::state_machine<Fsm_>(scheduler,worker)
    {
    }
};

// Manager using state machine
struct Manager : boost::asynchronous::trackable_servant<>
{
    Manager(boost::asynchronous::any_weak_scheduler<> scheduler, int threads)
        : boost::asynchronous::trackable_servant<>(scheduler,
                                               // threadpool and a simple lockfree_queue queue
                                               boost::asynchronous::make_shared_scheduler_proxy<
                                                   boost::asynchronous::multiqueue_threadpool_scheduler<
                                                           boost::asynchronous::lockfree_queue<>>>(threads))
        , fsm(scheduler,get_worker())
    {
        fsm.start();
    }
    void count()
    {
        // to show our race-free code, we use a counter (made by fsm with 2 states), set by external threads.
        // an even number in the counter will cause second task to not be executed
        fsm.process_event(toggle());
    }

    // call to this is posted and executes in our (safe) single-thread scheduler
    boost::future<void> start()
    {
        fsm.process_event(init());
        return fsm.m_promise.get_future();
    }
private:
    Fsm fsm;
};
class ManagerProxy : public boost::asynchronous::servant_proxy<ManagerProxy,Manager>
{
public:
    template <class Scheduler>
    ManagerProxy(Scheduler s, int threads):
        boost::asynchronous::servant_proxy<ManagerProxy,Manager>(s,threads)
    {}
    // caller will get a future
    BOOST_ASYNC_FUTURE_MEMBER(start)
    BOOST_ASYNC_FUTURE_MEMBER(count)
};

}

void example_callback_msm()
{
    std::cout << "example_callback_msm" << std::endl;
    {
        // a single-threaded world, where Manager will live.
        auto scheduler = boost::asynchronous::make_shared_scheduler_proxy<
                                boost::asynchronous::single_thread_scheduler<
                                     boost::asynchronous::lockfree_queue<>>>();
        ManagerProxy proxy(scheduler,boost::thread::hardware_concurrency());
        boost::future<boost::future<void> > fu = proxy.start();
        // show that we have in our ManagerProxy a completely safe thread world which can be accessed by several threads
        // add more threads for fun if needed. An odd number of them will cause second task to be called.
        std::thread t1([proxy](){proxy.count();});
        std::thread t2([proxy](){proxy.count();});
        // our proxy is now shared by 3 external threads, has no lifecycle or race issue
        t1.join();
        t2.join();

        // wait for shutdown
        fu.get().get();
    }
    std::cout << "end example_callback_msm \n" << std::endl;
}
