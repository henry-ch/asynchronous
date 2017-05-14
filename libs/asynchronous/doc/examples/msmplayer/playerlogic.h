// Copyright 2017 Christophe Henry
// henry UNDERSCORE christophe AT hotmail DOT com
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef PLAYERLOGIC_H
#define PLAYERLOGIC_H

#include <vector>
#include <string>
#include <memory>
#include <boost/asynchronous/trackable_servant.hpp>
#include <boost/asynchronous/scheduler/single_thread_scheduler.hpp>
#include <boost/asynchronous/scheduler/threadpool_scheduler.hpp>
#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include <boost/asynchronous/servant_proxy.hpp>
#include <idisplay.h>
#include <ihardware.h>

struct DiskInfo
{
    typedef std::vector<std::string> Songs;

    DiskInfo():goodDisk(true){}
    DiskInfo(bool good):goodDisk(good){}

    bool isGood(){return goodDisk;}
    Songs& getSongs(){return songs;}

    bool goodDisk;
    Songs songs;
};

struct PossibleActions
{
    bool isStopPossible;
    bool isPlayPossible;
    bool isPausePossible;
    bool isOpenPossible;
    bool isClosePossible;
    bool isNextSongPossible;
    bool isPreviousSongPossible;
    bool isVolumeUpPossible;
    bool isVolumeDownPossible;
};

// thin wrapper around the core logic, a MSM based state machine.
// The wrapper has the advantage of hiding the state machine in its cpp file, thus reducing compile time (through include).
// no external part gets to use this servant directly, usage is exclusively done through PlayerLogicProxy
class PlayerLogic : public boost::asynchronous::trackable_servant<>
{

public:
    PlayerLogic(boost::asynchronous::any_weak_scheduler<> scheduler,std::shared_ptr<IDisplay> display, std::shared_ptr<IHardware> hardware);
    // event methods
    void start(std::function<void(PossibleActions)> callback);
    void playButton(std::function<void(PossibleActions)> callback);
    void pauseButton(std::function<void(PossibleActions)> callback);
    void stopButton(std::function<void(PossibleActions)> callback);
    void openCloseButton(std::function<void(PossibleActions)> callback);
    void nextSongButton(std::function<void(PossibleActions)> callback);
    void previousSongButton(std::function<void(PossibleActions)> callback);
    void cdDetected(DiskInfo const&,std::function<void(PossibleActions)> callback);
    void volumeUp(std::function<void(PossibleActions)> callback);
    void volumeDown(std::function<void(PossibleActions)> callback);

    bool isStopPossible()const;
    bool isPlayPossible()const;
    bool isPausePossible()const;
    bool isOpenPossible()const;
    bool isClosePossible()const;
    bool isNextSongPossible()const;
    bool isPreviousSongPossible()const;
    bool isVolumeUpPossible()const;
    bool isVolumeDownPossible()const;

    // asynchronous request
    void possibleActions(std::function<void(PossibleActions)> callback);

private:
    struct StateMachine;
    std::shared_ptr<StateMachine> fsm_;
};

// shareable proxy hiding PlayerLogic from the external world. It serializes calls to PlayerLogic.
// It creates its own scheduler as we want one for ourselves. At this point, we created a complete autonomous thread world,
// which can be used in any part of a real application.
class PlayerLogicProxy : public boost::asynchronous::servant_proxy<PlayerLogicProxy,PlayerLogic>
{
public:
    PlayerLogicProxy(std::shared_ptr<IDisplay> display, std::shared_ptr<IHardware> hardware):
        boost::asynchronous::servant_proxy<PlayerLogicProxy,PlayerLogic>
        (boost::asynchronous::make_shared_scheduler_proxy<
            boost::asynchronous::single_thread_scheduler<
              boost::asynchronous::lockfree_queue<>>>()
        ,std::move(display)
        ,std::move(hardware))
    {}

    // access members, will forward calls to PlayerLogic, within PlayerLogic's own thread.
    // _POST_ means that nothing is returned, we will exclusively use callbacks to avoid any blocking
    BOOST_ASYNC_POST_MEMBER(start)
    BOOST_ASYNC_POST_MEMBER(playButton)
    BOOST_ASYNC_POST_MEMBER(pauseButton)
    BOOST_ASYNC_POST_MEMBER(stopButton)
    BOOST_ASYNC_POST_MEMBER(openCloseButton)
    BOOST_ASYNC_POST_MEMBER(nextSongButton)
    BOOST_ASYNC_POST_MEMBER(previousSongButton)
    BOOST_ASYNC_POST_MEMBER(cdDetected)
    BOOST_ASYNC_POST_MEMBER(volumeUp)
    BOOST_ASYNC_POST_MEMBER(volumeDown)
    BOOST_ASYNC_POST_MEMBER(possibleActions)

};
#endif // PLAYERLOGIC_H
