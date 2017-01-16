// Copyright 2017 Christophe Henry
// henry UNDERSCORE christophe AT hotmail DOT com
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef IDISPLAY_H
#define IDISPLAY_H

#include <qstring.h>

// interface for setting a text on the display
// it is implemented by PlayerGui's SafeDisplay helper class for the purpose of this example

struct IDisplay
{
    virtual ~IDisplay();
    virtual void setDisplayText(QString text)=0;
};


#endif // IDISPLAY_H
