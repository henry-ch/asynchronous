// Copyright 2017 Christophe Henry
// henry UNDERSCORE christophe AT hotmail DOT com
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef CALCULATOR_H
#define CALCULATOR_H
#include <boost/asio.hpp>
#include <memory>

#include <boost/asynchronous/extensions/qt/qt_servant.hpp>

#include <QWidget>
#include <idisplay.h>
#include <ihardware.h>
#include <playerlogic.h>



QT_BEGIN_NAMESPACE
class QLineEdit;
class QButtonGroup;
class QTimer;
class QLabel;
class QLineEdit;
QT_END_NAMESPACE
class Button;

// A simple UI for interaction with our CD Player logic
// note that we made it a qt_servant so that it can use safe callbacks (make_safe_callback)
// provided by asynchronous. It also could use a threadpool, though we currently do not make use of this feature
class PlayerGui : public QWidget, public boost::asynchronous::qt_servant<>
{
    Q_OBJECT

public:
    PlayerGui(QWidget *parent = 0);

    virtual ~PlayerGui();
private slots:
    void playClicked();
    void stopClicked();
    void pauseClicked();
    void previousSongClicked();
    void nextSongClicked();
    void volumeUpButtonClicked();
    void volumeDownButtonClicked();
    void openCloseClicked();
    void timerDone();
    void cdDetectedClicked();
    //void startOpenCloseTimer(int intervalMs);

private:
    Button *createButton(const QString &text, const char *member);
    // IDisplay implementation
    void setDisplayText(QString text);
    // IHardware implementation
    void openDrawer(std::function<void()> callback);
    void closeDrawer(std::function<void()> callback);
    void startPlaying();
    void stopPlaying();
    void pausePlaying();
    void nextSong();
    void previousSong();
    void volumeUp();
    void volumeDown();
    void startTimer(int intervalMs,std::function<void()> callback);
    void stopTimer();

    std::function<void(PossibleActions)> actionsHandler();
    void checkEnabledButtons();
    virtual void customEvent(QEvent* event);

    QLineEdit *display;
    Button *playButton;
    Button *stopButton;
    Button *pauseButton;
    Button *openCloseButton;
    Button *cdDetected;
    Button *nextSongButton;
    Button *previousSongButton;
    Button *volumeUpButton;
    Button *volumeDownButton;
    QButtonGroup* diskInfoButtonGroup;
    QTimer* timer;
    QLabel* hwActionsLabel;
    QLineEdit* hwActions;

    PlayerLogicProxy logic_;
    // our asio worker thread
    // is used to simulate hardware
    std::shared_ptr<boost::asio::io_service > ioservice_;
    std::shared_ptr<boost::asio::io_service::work> work_;
    std::shared_ptr<boost::thread> thread_;


    std::function<void()> currentCallback;

};


#endif
