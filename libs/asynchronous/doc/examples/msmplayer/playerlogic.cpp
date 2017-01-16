// Copyright 2008 Christophe Henry
// henry UNDERSCORE christophe AT hotmail DOT com
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
// we have more than the default 20 transitions, so we need to require more from Boost.MPL
#include "boost/mpl/vector/vector30.hpp"
#include <boost/utility/enable_if.hpp>
#include <boost/type_traits/is_same.hpp>
// back-end
#include <boost/msm/back/state_machine.hpp>
//front-end
#include <boost/msm/front/state_machine_def.hpp>
// functors
#include <boost/msm/front/functor_row.hpp>
#include <boost/msm/front/euml/common.hpp>
// for And_ operator
#include <boost/msm/front/euml/operator.hpp>

#include <boost/asynchronous/scheduler/threadpool_scheduler.hpp>
#include <boost/asynchronous/queue/guarded_deque.hpp>
#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include <idisplay.h>

#include "playerlogic.h"

namespace msm = boost::msm;
namespace mpl = boost::mpl;
using namespace msm::front;
using namespace boost::msm::front::euml;

namespace
{
    // events
    struct play {};
    // an event convertible from any other event (used for transitions originating from an exit pseudo state)
    struct stop
    {
        stop(){}
        template <class T>
        stop(T const&){}
    };
    struct pausing{};
    struct open_close{};
    // an event with data
    struct cd_detected
    {
        cd_detected():info_(0){} //not used
        cd_detected(DiskInfo const& info):info_(&info){}
        DiskInfo const* info_;
    };
    struct disc_in
    {
        disc_in()
            : info_(){} //not used
        disc_in(DiskInfo const& info,std::function<void(PossibleActions)> callback):info_(info),callback_(callback){}
        DiskInfo info_;
        std::function<void(PossibleActions)> callback_;
    };
    struct opened{};
    struct closed{};
    struct next_song{};
    struct prev_song{};
    struct volume_up{};
    struct volume_down{};
    struct volume_timeout{};
    struct update_song{};

    //flags. Allow us to be more flexible than asking in which state the FSM is.
    struct PlayPossible{};
    struct PausePossible{};
    struct StopPossible{};
    struct OpenPossible{};
    struct ClosePossible{};
    struct PrevNextPossible{};

    //helper for callbacks. Simple wrapper around process_event
    template <class FSM,class EVENT>
    struct Callback
    {
        Callback(FSM* fsm):fsm_(fsm){}
        void operator()()const
        {
            fsm_->process_event(evt_);
        }
    private:
        FSM* fsm_;
        EVENT evt_;
    };
    // The list of FSM states
    struct Init : public msm::front::state<>
    {
    };
    struct Empty : public msm::front::state<>
    {
        typedef mpl::vector1<OpenPossible>      flag_list;
        typedef mpl::vector1<play>         deferred_events;
    };
    struct CheckingDisc : public msm::front::state<>
    {
        typedef mpl::vector1<play>         deferred_events;
    };
    struct Open : public msm::front::state<>
    {
        typedef mpl::vector2<ClosePossible,PlayPossible>      flag_list;
        template <class Event,class FSM>
        void on_entry(Event const& ,FSM& fsm) {fsm.display_->setDisplayText("Open");}
    };
    struct Opening : public msm::front::state<>
    {
        template <class Event,class FSM>
        void on_entry(Event const&,FSM& fsm)
        {
            fsm.display_->setDisplayText("Opening");
        }
    };
    struct Closing : public msm::front::state<>
    {
        typedef mpl::vector1<PlayPossible>      flag_list;
        typedef mpl::vector1<play>         deferred_events;
        template <class Event,class FSM>
        void on_entry(Event const&,FSM& fsm)
        {
            fsm.display_->setDisplayText("Closing");
            // start a longer action and pass the desired callback for when action done
            fsm.hardware_->closeDrawer(Callback<FSM,closed>(&fsm));
        }
    };
    struct Stopped : public msm::front::state<>
    {
        typedef mpl::vector2<PlayPossible,OpenPossible>      flag_list;

        template <class Event,class FSM>
        void on_entry(Event const& ,FSM& fsm) {fsm.display_->setDisplayText("Stopped");}
    };
    struct Running : public msm::front::state<>
    {
        // flags are searched recursively
        typedef mpl::vector1<PrevNextPossible>      flag_list;
        template <class Event,class FSM>
        void on_entry(Event const&,FSM& fsm)
        {
            // we have the right to update display only if this fsm is active
            if (fsm.active_)
            {
                fsm.display_->setDisplayText(QString(fsm.songs_[fsm.currentSong_].c_str()));
            }
        }
    };
    struct NoVolumeChange : public msm::front::state<>
    {
    };
    struct VolumeChange : public msm::front::state<>
    {
        template <class Event,class FSM>
        void on_entry(Event const&,FSM& fsm)
        {
            // we have the right to update display only if this fsm is active
            if (fsm.active_)
            {
                QString vol = QString("Volume:%1").arg(fsm.volume_);
                fsm.display_->setDisplayText(vol);
                // start a timer and when expired, call callback
                fsm.hardware_->startTimer(2000,
                                          fsm.template make_safe_callback([&fsm]()mutable{fsm.process_event(volume_timeout());}));
            }
        }
    };
    struct PseudoExit : public msm::front::exit_pseudo_state<stop>
    {
    };

    // transition actions
    struct first_song
    {
        template <class FSM,class EVT,class SourceState,class TargetState>
        bool operator()(EVT const&,FSM& fsm,SourceState& ,TargetState& )
        {
            return (fsm.currentSong_ == 0);
        }
    };
    struct last_song
    {
        template <class FSM,class EVT,class SourceState,class TargetState>
        bool operator()(EVT const&,FSM& fsm,SourceState& ,TargetState& )
        {
            return (fsm.songs_.empty() || fsm.currentSong_ >= fsm.songs_.size()-1);
        }
    };
    struct inc_song
    {
        template <class FSM,class EVT,class SourceState,class TargetState>
        void operator()(EVT const& ,FSM& fsm,SourceState& ,TargetState& )
        {
            ++fsm.currentSong_;
            fsm.hardware_->nextSong();
        }
    };
    struct dec_song
    {
        template <class FSM,class EVT,class SourceState,class TargetState>
        void operator()(EVT const& ,FSM& fsm,SourceState& ,TargetState& )
        {
            --fsm.currentSong_;
            fsm.hardware_->previousSong();
        }
    };
    struct inc_volume
    {
        template <class FSM,class EVT,class SourceState,class TargetState>
        void operator()(EVT const& ,FSM& fsm,SourceState& ,TargetState& )
        {
            if (fsm.volume_ < std::numeric_limits<std::size_t>::max())
            {
                ++fsm.volume_;
                fsm.hardware_->volumeUp();
            }
        }
    };
    struct dec_volume
    {
        template <class FSM,class EVT,class SourceState,class TargetState>
        void operator()(EVT const& ,FSM& fsm,SourceState& ,TargetState& )
        {
            if (fsm.volume_ > 0)
            {
                --fsm.volume_;
                fsm.hardware_->volumeDown();
            }
        }
    };
    template <class ToProcessEvt>
    struct processing
    {
        template <class FSM,class EVT,class SourceState,class TargetState>
        void operator()(EVT const& ,FSM& fsm,SourceState& ,TargetState& )
        {
            fsm.process_event(ToProcessEvt());
        }
    };
    // submachine front-end
    struct Playing_ : public msm::front::state_machine_def<Playing_>, public boost::asynchronous::trackable_servant<>
    {
        typedef mpl::vector3<StopPossible,OpenPossible,PausePossible>      flag_list;

        //submachines also have entry/exit conditions
        template <class Event,class FSM>
        void on_entry(Event const&,FSM& fsm) {active_=true;fsm.hardware_->startPlaying();}

        // we leave the state with another event than pause => we want to start from beginning
        template <class Event,class FSM>
        typename boost::disable_if<typename boost::is_same<Event,pausing>::type,void>::type
        on_exit(Event const&,FSM& ){currentSong_ = 0;active_=false;}

        // we leave the state with pause => we want to continue where we left
        template <class Event,class FSM>
        typename boost::enable_if<typename boost::is_same<Event,pausing>::type,void>::type
        on_exit(Event const&,FSM& ){ active_=false;}

        // initial states (2 regions => 2 initial states)
        typedef boost::mpl::vector2<Running,NoVolumeChange> initial_state;
        // Playing has a transition table
        struct transition_table : mpl::vector<
            //    Start           Event            Target           Action                    Guard
            //  +----------------+----------------+----------------+-------------------------+-------------------+
            Row < Running        , next_song      , PseudoExit     , none                    , last_song         >,
            Row < Running        , next_song      , Running        , inc_song                , Not_<last_song>   >,
            Row < Running        , prev_song      , Running        , dec_song                , Not_<first_song>  >,
            Row < Running        , prev_song      , PseudoExit     , none                    , first_song        >,
            Row < Running        , update_song    , Running        , none                    , none              >,
            Row < NoVolumeChange , volume_up      , VolumeChange   , inc_volume              , none              >,
            Row < NoVolumeChange , volume_down    , VolumeChange   , dec_volume              , none              >,
            // ignore timer events
            Row < NoVolumeChange , volume_timeout , none           , none                    , none              >,
            Row < VolumeChange   , volume_up      , VolumeChange   , inc_volume              , none              >,
            Row < VolumeChange   , volume_down    , VolumeChange   , dec_volume              , none              >,
            Row < VolumeChange   , volume_timeout , NoVolumeChange , processing<update_song> , none              >
        > {};
        // Replaces the default no-transition response.
        template <class FSM,class Event>
        void no_transition(Event const& , FSM&,int state)
        {
            std::cout << "no transition from state " << state << " on event " << typeid(Event).name() << std::endl;
        }
        Playing_():currentSong_(0),volume_(0),active_(false){}
        size_t currentSong_;
        DiskInfo::Songs songs_;
        size_t volume_;
        std::shared_ptr<IDisplay> display_;
        std::shared_ptr<IHardware> hardware_;
        bool active_;
    };
    // backend => this is now usable in a transition table like any other state
    typedef boost::msm::back::state_machine<Playing_> Playing;
    // player substates
    struct Paused : public msm::front::state<>
    {
        typedef mpl::vector3<PlayPossible,OpenPossible,StopPossible>      flag_list;
        template <class Event,class FSM>
        void on_entry(Event const&,FSM& fsm) {fsm.display_->setDisplayText("Paused");}
    };
    // transition actions
    struct first_message
    {
        template <class FSM,class EVT,class SourceState,class TargetState>
        void operator()(EVT const& ,FSM& fsm,SourceState& ,TargetState& )
        {
            fsm.display_->setDisplayText("Insert Disk");
        }
    };
    struct open_drawer
    {
        template <class FSM,class EVT,class SourceState,class TargetState>
        void operator()(EVT const& ,FSM& fsm,SourceState& ,TargetState& )
        {
            // start a longer action and pass the desired callback for when action done
            fsm.hardware_->openDrawer(Callback<FSM,opened>(&fsm));
        }
    };
    struct check_disc
    {
        template <class FSM,class EVT,class SourceState,class TargetState>
        void operator()(EVT const& evt,FSM& fsm,SourceState& ,TargetState& )
        {
            // start a longer action
            fsm.template start_checking_disc(&fsm,evt.info_, evt.callback_);
        }
    };

    struct close_drawer
    {
        template <class FSM,class EVT,class SourceState,class TargetState>
        void operator()(EVT const& ,FSM& fsm,SourceState& ,TargetState& )
        {
            fsm.display_->setDisplayText("Checking Disk");
        }
    };
    struct wrong_disk
    {
        template <class FSM,class EVT,class SourceState,class TargetState>
        void operator()(EVT const& ,FSM& fsm,SourceState& ,TargetState& )
        {
            fsm.display_->setDisplayText("Wrong Disk");
        }
    };
    struct stop_playback
    {
        template <class FSM,class EVT,class SourceState,class TargetState>
        void operator()(EVT const& ,FSM& fsm,SourceState& ,TargetState& )
        {
            fsm.hardware_->stopPlaying();
        }
    };
    struct pause_playback
    {
        template <class FSM,class EVT,class SourceState,class TargetState>
        void operator()(EVT const& ,FSM& fsm,SourceState& ,TargetState& )
        {
            fsm.hardware_->pausePlaying();
        }
    };
    // transition guards
    struct good_disk
    {
        template <class FSM,class EVT,class SourceState,class TargetState>
        bool operator()(EVT const& evt,FSM&,SourceState& ,TargetState& )
        {
            return evt.info_->goodDisk;
        }
    };
    struct store_cd_info
    {
        template <class FSM,class EVT,class SourceState,class TargetState>
        void operator()(EVT const& evt, FSM& fsm,SourceState& ,TargetState& )
        {
            fsm.template get_state<Playing&>().songs_ = evt.info_->songs;
        }
    };
    // front-end: define the FSM structure
    struct StateMachine_ : public msm::front::state_machine_def<StateMachine_> , public boost::asynchronous::trackable_servant<>
    {
    public:
        // constructor, automatically forwarded from back-end
        StateMachine_(boost::asynchronous::any_weak_scheduler<> scheduler,
                      boost::asynchronous::any_shared_scheduler_proxy<> worker,
                      std::shared_ptr<IDisplay> display, std::shared_ptr<IHardware> hardware)
            : boost::asynchronous::trackable_servant<>(scheduler,worker)
            , display_(display),hardware_(hardware){}

        // the initial state of the player SM. Must be defined
        typedef Init initial_state;

        // Transition table for player
        struct transition_table : mpl::vector<
          //    Start             Event         Target     Action                     Guard
          //  +------------------+-------------+----------+--------------------------+-------------------+
          // an anonymous transition (without event trigger)
          Row < Init             , none        , Empty    , first_message            , none             >,
          //  +------------------+-------------+----------+--------------------------+-------------------+
          Row < Stopped          , play        , Playing  , none                     , none             >,
          Row < Stopped          , open_close  , Opening  , open_drawer              , none             >,
          //  +------------------+-------------+----------+--------------------------+-------------------+
          Row < Opening          , opened      , Open     , none                     , none             >,
          //  +------------------+-------------+----------+--------------------------+-------------------+
          Row < Open             , open_close  , Closing  , none                     , none             >,
          // it is possible to process events from a transition
          // note that we have an internal transition
          Row < Open             , play        , none     , ActionSequence_<
                                                            mpl::vector<
                                                              processing<open_close>,
                                                              processing<play> > >   , none             >,
          // ignore timer events
          Row < Open             , opened      , none     , none                     , none             >,
          //  +------------------+-------------+----------+--------------------------+-------------------+
          Row < Closing          , closed      , Empty    , close_drawer             , none             >,
          //  +------------------+-------------+----------+--------------------------+-------------------+
          Row < Empty            , open_close  , Opening  , open_drawer              , none             >,
          Row < Empty            , disc_in     , CheckingDisc  , check_disc                , none             >,
          Row < CheckingDisc     , cd_detected , Stopped  , ActionSequence_<
                                                            mpl::vector<
                                                              store_cd_info,
                                                              stop_playback> >       , good_disk        >,
          Row < CheckingDisc     , cd_detected , none     , wrong_disk               , Not_<good_disk>  >,
          // ignore timer events
          Row < Empty            , closed      , none     , none                     , none             >,
          //  +------------------+-------------+----------+--------------------------+-------------------+
          Row < Playing          , stop        , Stopped   , stop_playback           , none             >,
          Row < Playing          , pausing     , Paused    , pause_playback          , none             >,
          Row < Playing          , open_close  , Opening   , ActionSequence_<
                                                             mpl::vector<
                                                               stop_playback,
                                                               open_drawer> >        , none             >,
          Row < Playing::exit_pt<
                PseudoExit>      , stop        , Stopped   , stop_playback           , none             >,
          //  +------------------+-------------+----------+--------------------------+-------------------+
          Row < Paused           , play        , Playing   , none                    , none             >,
          Row < Paused           , stop        , Stopped   , stop_playback           , none             >,
          Row < Paused           , open_close  , Opening   , ActionSequence_<
                                                             mpl::vector<
                                                               stop_playback,
                                                               open_drawer> >        , none             >
          //  +------------------+-------------+----------+--------------------------+-------------------+
        >{};

        template <class Fsm>
        void start_checking_disc(Fsm* fsm,DiskInfo const& info,std::function<void(PossibleActions)> callback)
        {
            fsm->post_callback(
                   // long action, possibly parallel, executing inside our threadpool
                   []()
                   {
                        std::cout << "long action checking if disc is ok" << std::endl;
                        return 42;
                   },
                   // the lambda calls Servant, just to show that all is safe, Servant is alive if this is called
                   // callback functor.
                   [fsm,info,callback](boost::asynchronous::expected<int> res)
                   {
                        std::cout << "disc checked" << std::endl;
                        if( res.get() == 42 )
                        {
                            fsm->process_event(cd_detected(info));
                        }
                        PossibleActions actions
                        {
                            fsm->template is_flag_active<StopPossible>(),
                            fsm->template is_flag_active<PlayPossible>(),
                            fsm->template is_flag_active<PausePossible>(),
                            fsm->template is_flag_active<OpenPossible>(),
                            fsm->template is_flag_active<ClosePossible>(),
                            fsm->template is_flag_active<PrevNextPossible>(),
                            fsm->template is_flag_active<PrevNextPossible>(),
                            fsm->template is_flag_active<PrevNextPossible>(),
                            fsm->template is_flag_active<PrevNextPossible>()
                        };
                        callback(std::move(actions));
                   }
            );
        }

        // Replaces the default no-transition response.
        template <class FSM,class Event>
        void no_transition(Event const& , FSM&,int state)
        {
            std::cout << "no transition from state " << state << " on event " << typeid(Event).name() << std::endl;
        }
        std::shared_ptr<IDisplay> display_;
        std::shared_ptr<IHardware> hardware_;
    };
}
// just inherit from back-end and this structure can be forward-declared in the header file
// for shorter compile-time
struct PlayerLogic::StateMachine : public msm::back::state_machine<StateMachine_>
{
    StateMachine(boost::asynchronous::any_weak_scheduler<> scheduler,
                 boost::asynchronous::any_shared_scheduler_proxy<> worker,
                 std::shared_ptr<IDisplay> display, std::shared_ptr<IHardware> hardware)
        : msm::back::state_machine<StateMachine_>(scheduler,worker,display,hardware)
    {
        get_state<Playing&>().hardware_ = hardware;
        get_state<Playing&>().display_ = display;
        get_state<Playing&>().set_scheduler(scheduler);
    }
};
PlayerLogic::PlayerLogic(boost::asynchronous::any_weak_scheduler<> scheduler,
                         std::shared_ptr<IDisplay> display, std::shared_ptr<IHardware> hardware)
    : boost::asynchronous::trackable_servant<>(scheduler,
                                               // we decide that we need a threadpool using 4 threads
                                               // hardware_concurrency would also be a flexible option
                                               boost::asynchronous::make_shared_scheduler_proxy<
                                                  boost::asynchronous::threadpool_scheduler<
                                                    boost::asynchronous::lockfree_queue<>>>(4))
    , fsm_(new PlayerLogic::StateMachine(scheduler,get_worker(),display,hardware))
{
}
// start the state machine (first call of on_entry)
void PlayerLogic::start(std::function<void(PossibleActions)> callback)
{
    fsm_->start();
    // inform caller of new state of possible actions
    possibleActions(callback);
}
// the public interfaces simply forwards events to the state machine
void PlayerLogic::playButton(std::function<void(PossibleActions)> callback)
{
    fsm_->process_event(play());
    // inform caller of new state of possible actions
    possibleActions(callback);
}
void PlayerLogic::pauseButton(std::function<void(PossibleActions)> callback)
{
    fsm_->process_event(pausing());
    // inform caller of new state of possible actions
    possibleActions(callback);
}
void PlayerLogic::stopButton(std::function<void(PossibleActions)> callback)
{
    fsm_->process_event(stop());
    // inform caller of new state of possible actions
    possibleActions(callback);
}
void PlayerLogic::openCloseButton(std::function<void(PossibleActions)> callback)
{
    fsm_->process_event(open_close());
    // inform caller of new state of possible actions
    possibleActions(callback);
}
void PlayerLogic::nextSongButton(std::function<void(PossibleActions)> callback)
{
    fsm_->process_event(next_song());
    // inform caller of new state of possible actions
    possibleActions(callback);
}
void PlayerLogic::previousSongButton(std::function<void(PossibleActions)> callback)
{
    fsm_->process_event(prev_song());
    // inform caller of new state of possible actions
    possibleActions(callback);
}
void PlayerLogic::cdDetected(DiskInfo const& info,std::function<void(PossibleActions)> callback)
{
    // we pack the callback in the event so that the caller can be asynchronously informed after our long action
    // (waiting for hardware) completes
    fsm_->process_event(disc_in(info,std::move(callback)));
    // inform caller of new state of possible actions
    possibleActions(callback);
}
void PlayerLogic::volumeUp(std::function<void(PossibleActions)> callback)
{
    fsm_->process_event(volume_up());
    // inform caller of new state of possible actions
    possibleActions(callback);
}
void PlayerLogic::volumeDown(std::function<void(PossibleActions)> callback)
{
    fsm_->process_event(volume_down());
    // inform caller of new state of possible actions
    possibleActions(callback);
}
bool PlayerLogic::isStopPossible()const
{
    return fsm_->is_flag_active<StopPossible>();
}
bool PlayerLogic::isPlayPossible()const
{
    return fsm_->is_flag_active<PlayPossible>();
}
bool PlayerLogic::isPausePossible()const
{
    return fsm_->is_flag_active<PausePossible>();
}
bool PlayerLogic::isOpenPossible()const
{
    return fsm_->is_flag_active<OpenPossible>();
}
bool PlayerLogic::isClosePossible()const
{
    return fsm_->is_flag_active<ClosePossible>();
}
bool PlayerLogic::isNextSongPossible()const
{
    return fsm_->is_flag_active<PrevNextPossible>();
}
bool PlayerLogic::isPreviousSongPossible()const
{
    return fsm_->is_flag_active<PrevNextPossible>();
}
bool PlayerLogic::isVolumeUpPossible()const
{
    return fsm_->is_flag_active<PrevNextPossible>();
}
bool PlayerLogic::isVolumeDownPossible()const
{
    return fsm_->is_flag_active<PrevNextPossible>();
}
// asynchronous request
void PlayerLogic::possibleActions(std::function<void(PossibleActions)> callback)
{
    if (callback)
    {
        PossibleActions actions
        {
            isStopPossible(),
            isPlayPossible(),
            isPausePossible(),
            isOpenPossible(),
            isClosePossible(),
            isNextSongPossible(),
            isPreviousSongPossible(),
            isVolumeUpPossible(),
            isVolumeDownPossible()
        };
        callback(std::move(actions));
    }
}


