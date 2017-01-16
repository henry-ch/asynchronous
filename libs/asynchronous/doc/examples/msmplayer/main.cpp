// Copyright 2017 Christophe Henry
// henry UNDERSCORE christophe AT hotmail DOT com
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <QApplication>

#include "playergui.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    PlayerGui p;
    p.show();
    return app.exec();
}
