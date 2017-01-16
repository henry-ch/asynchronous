// Copyright 2017 Christophe Henry
// henry UNDERSCORE christophe AT hotmail DOT com
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef IHARDWARE_H
#define IHARDWARE_H

#include <boost/function.hpp>

// interface for interaction with real hardware of a CD player (opening/closing of drawer, playing, etc.)
// it is implemented by PlayerGui's SafeHardware helper class for the purpose of this example

class IHardware
{
public:
    virtual ~IHardware();
    // supported HW actions
    // the actions with callback require the callback to be posted to the correct thread (where the fsm lives)
    virtual void openDrawer(std::function<void()> callback)=0;
    virtual void closeDrawer(std::function<void()> callback)=0;
    virtual void startPlaying()=0;
    virtual void stopPlaying()=0;
    virtual void pausePlaying()=0;
    virtual void nextSong()=0;
    virtual void previousSong()=0;
    virtual void volumeUp()=0;
    virtual void volumeDown()=0;

    virtual void startTimer(int intervalMs,std::function<void()> callback)=0;
    virtual void stopTimer()=0;
};

#endif // IHARDWARE_H
