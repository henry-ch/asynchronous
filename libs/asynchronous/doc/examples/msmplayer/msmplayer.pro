QT       += core
QT       += gui
QT       += widgets
TEMPLATE = app
CONFIG += console
CONFIG += app_bundle

Release: QMAKE_CXXFLAGS += -O2
QMAKE_CXXFLAGS += -DBOOST_ASYNCHRONOUS_SAVE_CPU_LOAD


QMAKE_CXXFLAGS += -std=c++14 -Wno-strict-aliasing
INCLUDEPATH += /home/xtoff/projects/asynchronous/
INCLUDEPATH += /home/xtoff/boost_1_57_0/
INCLUDEPATH += /home/xtoff/tbb43_20141204oss_1/include/
INCLUDEPATH += /home/xtoff/boost_1_57_0//libs/serialization/example/

LIBS += /home/xtoff/boost_1_57_0/stage/lib/libboost_system.a
LIBS += /home/xtoff/boost_1_57_0/stage/lib/libboost_thread.a
LIBS += /home/xtoff/boost_1_57_0/stage/lib/libboost_chrono.a
LIBS += /home/xtoff/boost_1_57_0/stage/lib/libboost_date_time.a
LIBS += /home/xtoff/boost_1_57_0/stage/lib/libboost_unit_test_framework.a
LIBS += /home/xtoff/boost_1_57_0/stage/lib/libboost_serialization.a

QMAKE_CXXFLAGS += -DBOOST_THREAD_USE_LIB
unix:!macx: LIBS += -lrt
LIBS += -lpthread


# install
target.path = .\player
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS player.pro
sources.path = .\player
INSTALLS += target sources

HEADERS += \
    playergui.h \
    playerlogic.h \
    ihardware.h \
    idisplay.h \
    button.h \
    ../../../../../boost/asynchronous/extensions/qt/connect_functor_helper.hpp \
    ../../../../../boost/asynchronous/extensions/qt/qt_post_helper.hpp \
    ../../../../../boost/asynchronous/extensions/qt/qt_safe_callback_helper.hpp \
    ../../../../../boost/asynchronous/extensions/qt/qt_servant.hpp

SOURCES += \
    playergui.cpp \
    playerlogic.cpp \
    main.cpp \
    ihardware.cpp \
    idisplay.cpp \
    button.cpp \
    ../../../../../boost/asynchronous/extensions/qt/connect_functor_helper.cpp \
    ../../../../../boost/asynchronous/extensions/qt/qt_post_helper.cpp \
    ../../../../../boost/asynchronous/extensions/qt/qt_safe_callback_helper.cpp \
    ../../../../../boost/asynchronous/extensions/qt/qt_servant.cpp


