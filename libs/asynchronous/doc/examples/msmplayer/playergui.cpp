// Copyright 2017 Christophe Henry
// henry UNDERSCORE christophe AT hotmail DOT com
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "playergui.h"

#include <QtGui>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRadioButton>
#include <QButtonGroup>
#include <QLabel>
#include <QApplication>
#include <math.h>

#include "button.h"

#include <boost/make_shared.hpp>
#include <boost/asynchronous/scheduler/threadpool_scheduler.hpp>
#include <boost/asynchronous/queue/lockfree_queue.hpp>
#include <boost/asynchronous/scheduler_shared_proxy.hpp>

#include <playerlogic.h>

namespace {
// these custom events will be send to PlayerGui from the simulated HW thread when HW action completes
class OpenedEvent : public QEvent {
public:
    OpenedEvent() : QEvent((QEvent::Type)2000) {}
    virtual ~OpenedEvent(){}
};
class ClosedEvent : public QEvent {
public:
    ClosedEvent() : QEvent((QEvent::Type)2001) {}
    virtual ~ClosedEvent(){}
};

// simple implementation of IDisplay. Will forward calls to the passed (safe) functor
struct SafeDisplay : public IDisplay
{
    SafeDisplay(std::function<void(QString)> callback)
        : callback_(callback){}
    virtual ~SafeDisplay() {}

    virtual void setDisplayText(QString text)
    {
        callback_(std::move(text));
    }
    std::function<void(QString)> callback_;
};

// simple implementation of IHardware. Will forward calls to the passed (safe) functors
struct SafeHardware : public IHardware
{
    SafeHardware(std::function<void(std::function<void()>)> open_drawer_callback,
                 std::function<void(std::function<void()>)> close_drawer_callback,
                 std::function<void()> start_playing_callback,
                 std::function<void()> stop_playing_callback,
                 std::function<void()> pause_playing_callback,
                 std::function<void()> next_song_callback,
                 std::function<void()> previous_song_callback_callback,
                 std::function<void()> volume_up_callback,
                 std::function<void()> volume_down_callback,
                 std::function<void(int,std::function<void()>)> start_timer_callback,
                 std::function<void()> stop_timer_callback)
        : open_drawer_callback_(open_drawer_callback)
        , close_drawer_callback_(close_drawer_callback)
        , start_playing_callback_(start_playing_callback)
        , stop_playing_callback_(stop_playing_callback)
        , pause_playing_callback_(pause_playing_callback)
        , next_song_callback_(next_song_callback)
        , previous_song_callback_callback_(previous_song_callback_callback)
        , volume_up_callback_(volume_up_callback)
        , volume_down_callback_(volume_down_callback)
        , start_timer_callback_(start_timer_callback)
        , stop_timer_callback_(stop_timer_callback)
    {}
    virtual ~SafeHardware() {}

    virtual void openDrawer(std::function<void()> callback)
    {
        open_drawer_callback_(std::move(callback));
    }
    virtual void closeDrawer(std::function<void()> callback)
    {
        close_drawer_callback_(std::move(callback));
    }
    virtual void startPlaying()
    {
        start_playing_callback_();
    }
    virtual void stopPlaying()
    {
        stop_playing_callback_();
    }
    virtual void pausePlaying()
    {
        pause_playing_callback_();
    }
    virtual void nextSong()
    {
        next_song_callback_();
    }
    virtual void previousSong()
    {
        previous_song_callback_callback_();
    }
    virtual void volumeUp()
    {
        volume_up_callback_();
    }
    virtual void volumeDown()
    {
        volume_down_callback_();
    }
    virtual void startTimer(int intervalMs,std::function<void()> callback)
    {
        start_timer_callback_(intervalMs,callback);
    }
    virtual void stopTimer()
    {
        stop_timer_callback_();
    }
    std::function<void(std::function<void()>)> open_drawer_callback_;
    std::function<void(std::function<void()>)> close_drawer_callback_;
    std::function<void()> start_playing_callback_;
    std::function<void()> stop_playing_callback_;
    std::function<void()> pause_playing_callback_;
    std::function<void()> next_song_callback_;
    std::function<void()> previous_song_callback_callback_;
    std::function<void()> volume_up_callback_;
    std::function<void()> volume_down_callback_;
    std::function<void(int,std::function<void()>)> start_timer_callback_;
    std::function<void()> stop_timer_callback_;
};

}

PlayerGui::~PlayerGui()
{}

PlayerGui::PlayerGui(QWidget *parent)
    : QWidget(parent),
      // create logic component, which we will see as an own world with its own thread
      logic_(std::shared_ptr<IDisplay>(
                 // we are a qt_servant, so we have safe callbacks. Make one to our setDisplayText
                 // this callback will be posted to our UI thread, and executed only if the PlayerGui is alive
                 new SafeDisplay(
                     make_safe_callback([this](QString text){setDisplayText(std::move(text));}))),
             std::shared_ptr<IHardware>(
                 // we are a qt_servant, so we have safe callbacks. Make one to our openDrawer/closeDrawer/...
                 // these callbacks will be posted to our UI thread, and executed only if the PlayerGui is alive
                  new SafeHardware(
                      make_safe_callback([this](std::function<void()> cb){openDrawer(std::move(cb));}),
                      make_safe_callback([this](std::function<void()> cb){closeDrawer(std::move(cb));}),
                      make_safe_callback([this](){startPlaying();}),
                      make_safe_callback([this](){stopPlaying();}),
                      make_safe_callback([this](){pausePlaying();}),
                      make_safe_callback([this](){nextSong();}),
                      make_safe_callback([this](){previousSong();}),
                      make_safe_callback([this](){volumeUp();}),
                      make_safe_callback([this](){volumeDown();}),
                      make_safe_callback([this](int intervalMs,std::function<void()> callback){startTimer(intervalMs,callback);}),
                      make_safe_callback([this](){stopTimer();})
                 ))),
      // for simplicity we simulate real hardware with a timer. Asio will provide one
      ioservice_(boost::make_shared<boost::asio::io_service>()),
      work_(new boost::asio::io_service::work(*ioservice_)),
      thread_()
{
    // create Asio thread
    auto service = ioservice_;
    thread_ = boost::make_shared<boost::thread>([service](){service->run();});

    // create UI
    display = new QLineEdit("");

    display->setReadOnly(true);
    display->setAlignment(Qt::AlignRight);
    display->setMaxLength(20);

    QFont font = display->font();
    font.setPointSize(font.pointSize() + 8);
    display->setFont(font);

    playButton = createButton(tr("Play"), SLOT(playClicked()));
    stopButton = createButton(tr("Stop"), SLOT(stopClicked()));
    pauseButton = createButton(tr("Pause"), SLOT(pauseClicked()));
    openCloseButton = createButton(tr("Open/Close"), SLOT(openCloseClicked()));
    previousSongButton = createButton(tr("Prev"), SLOT(previousSongClicked()));
    nextSongButton = createButton(tr("Next"), SLOT(nextSongClicked()));
    volumeUpButton = createButton(tr("Vol +"), SLOT(volumeUpButtonClicked()));
    volumeDownButton = createButton(tr("Vol -"), SLOT(volumeDownButtonClicked()));

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->setSizeConstraint(QLayout::SetFixedSize);
    mainLayout->addWidget(display);
    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->addWidget(openCloseButton);
    buttonLayout->addWidget(stopButton);
    buttonLayout->addWidget(pauseButton);
    buttonLayout->addWidget(playButton);
    buttonLayout->addWidget(previousSongButton);
    buttonLayout->addWidget(nextSongButton);
    buttonLayout->addWidget(volumeDownButton);
    buttonLayout->addWidget(volumeUpButton);
    mainLayout->addItem(buttonLayout);

    QHBoxLayout *hwSimuLayout = new QHBoxLayout;
    cdDetected = createButton(tr("CD Detected"), SLOT(cdDetectedClicked()));
    hwSimuLayout->addWidget(cdDetected);
    QRadioButton* goodDiskButton = new QRadioButton("Good Disk");
    goodDiskButton->setChecked(true);
    QRadioButton* badDiskButton = new QRadioButton("Bad Disk");
    diskInfoButtonGroup = new QButtonGroup;
    diskInfoButtonGroup->addButton(goodDiskButton);
    diskInfoButtonGroup->setId(goodDiskButton,0);
    diskInfoButtonGroup->addButton(badDiskButton);
    QVBoxLayout *diskButtonLayout = new QVBoxLayout;
    diskButtonLayout->addWidget(goodDiskButton);
    diskButtonLayout->addWidget(badDiskButton);

    hwSimuLayout->addItem(diskButtonLayout);

    hwActions = new QLineEdit(this);
    hwActions->setReadOnly(true);
    hwActions->setFixedWidth(200);
    hwActionsLabel = new QLabel("HW Actions:",this);
    hwActionsLabel->setBuddy(hwActions);
    hwSimuLayout->addWidget(hwActionsLabel);
    hwSimuLayout->addWidget(hwActions);

    mainLayout->addItem(hwSimuLayout);

    setLayout(mainLayout);

    timer = new QTimer(this);
    timer->setSingleShot(true);
    setWindowTitle(tr("Player"));
    logic_.start(actionsHandler());
}

void PlayerGui::playClicked()
{
    // forward action to logic and let it take the correct action. When done, let it call our callback
    logic_.playButton(actionsHandler());
}

void PlayerGui::stopClicked()
{
    // forward action to logic and let it take the correct action. When done, let it call our callback
    logic_.stopButton(actionsHandler());
}

void PlayerGui::pauseClicked()
{
    // forward action to logic and let it take the correct action. When done, let it call our callback
    logic_.pauseButton(actionsHandler());
}

void PlayerGui::openCloseClicked()
{
    // forward action to logic and let it take the correct action. When done, let it call our callback
    logic_.openCloseButton(actionsHandler());
}

void PlayerGui::cdDetectedClicked()
{
    // forward action to logic and let it take the correct action. When done, let it call our callback
    DiskInfo newDisk((diskInfoButtonGroup->checkedId()==0));
    newDisk.songs.push_back("Let it be");
    newDisk.songs.push_back("Yellow Submarine");
    newDisk.songs.push_back("Yesterday");
    logic_.cdDetected(newDisk,actionsHandler());
}
void PlayerGui::previousSongClicked()
{
    // forward action to logic and let it take the correct action. When done, let it call our callback
    logic_.previousSongButton(actionsHandler());
}
void PlayerGui::nextSongClicked()
{
    // forward action to logic and let it take the correct action. When done, let it call our callback
    logic_.nextSongButton(actionsHandler());
}
void PlayerGui::volumeUpButtonClicked()
{
    // forward action to logic and let it take the correct action. When done, let it call our callback
    logic_.volumeUp(actionsHandler());
}
void PlayerGui::volumeDownButtonClicked()
{
    // forward action to logic and let it take the correct action. When done, let it call our callback
    logic_.volumeDown(actionsHandler());
}
void PlayerGui::customEvent(QEvent *event)
{
    // if one of our custom vents, call the callback
    if ((event->type() == (QEvent::Type)2000) || (event->type() == (QEvent::Type)2001))
    {
        currentCallback();
        checkEnabledButtons();
    }
    QObject::customEvent(event);
}

void PlayerGui::startTimer(int intervalMs,std::function<void()> callback)
{
    currentCallback = callback;
    connect(timer,SIGNAL(timeout()),this,SLOT(timerDone()));
    timer->start(intervalMs);
}
void PlayerGui::stopTimer()
{
    timer->stop();
}
void PlayerGui::timerDone()
{
    currentCallback();
    checkEnabledButtons();
}

Button *PlayerGui::createButton(const QString &text, const char *member)
{
    Button *button = new Button(text);
    connect(button, SIGNAL(clicked()), this, member);
    return button;
}

void PlayerGui::setDisplayText(QString text)
{
    display->setText(text);
}

void PlayerGui::openDrawer(std::function<void()> callback)
{
    // post job to the HW thread and wait for cutom event when done
    currentCallback = callback;
    ioservice_->post([this]()
                     {
                        boost::this_thread::sleep(boost::posix_time::milliseconds(2000));
                        QApplication::postEvent(this,new OpenedEvent);
                     });
    hwActions->setText("open");
}
void PlayerGui::closeDrawer(std::function<void()> callback)
{
    // post job to the HW thread and wait for cutom event when done
    currentCallback = callback;
    ioservice_->post([this]()
                     {
                        boost::this_thread::sleep(boost::posix_time::milliseconds(2000));
                        QApplication::postEvent(this,new ClosedEvent);
                     });
    hwActions->setText("close");
}
void PlayerGui::startPlaying()
{
    // simulate hardware and show the text indicating what real hardware would do
    hwActions->setText("play");
}
void PlayerGui::stopPlaying()
{
    // simulate hardware and show the text indicating what real hardware would do
    hwActions->setText("stop");
}
void PlayerGui::pausePlaying()
{
    // simulate hardware and show the text indicating what real hardware would do
    hwActions->setText("pause");
}
void PlayerGui::nextSong()
{
    // simulate hardware and show the text indicating what real hardware would do
    hwActions->setText("next song");
}
void PlayerGui::previousSong()
{
    // simulate hardware and show the text indicating what real hardware would do
    hwActions->setText("previous song");
}
void PlayerGui::volumeUp()
{
    // simulate hardware and show the text indicating what real hardware would do
    hwActions->setText("vol. up");
}
void PlayerGui::volumeDown()
{
    // simulate hardware and show the text indicating what real hardware would do
    hwActions->setText("vol. down");
}
std::function<void(PossibleActions)> PlayerGui::actionsHandler()
{
    // for all our actions, pass a callback which will execute in the UI thread and set the buttons to correct state
    return make_safe_callback(
                [this](PossibleActions actions)
                {
                    playButton->setEnabled(actions.isPlayPossible);
                    stopButton->setEnabled(actions.isStopPossible);
                    pauseButton->setEnabled(actions.isPausePossible);
                    nextSongButton->setEnabled(actions.isNextSongPossible);
                    previousSongButton->setEnabled(actions.isPreviousSongPossible);
                    volumeUpButton->setEnabled(actions.isVolumeUpPossible);
                    volumeDownButton->setEnabled(actions.isVolumeDownPossible);
                    if (actions.isOpenPossible)
                    {
                        openCloseButton->setText("Open");
                        openCloseButton->setEnabled(true);
                    }
                    else if (actions.isClosePossible)
                    {
                        openCloseButton->setText("Close");
                        openCloseButton->setEnabled(true);
                    }
                    else
                    {
                        openCloseButton->setEnabled(false);
                    }
                });
}

void PlayerGui::checkEnabledButtons()
{
    logic_.possibleActions(actionsHandler());
}
