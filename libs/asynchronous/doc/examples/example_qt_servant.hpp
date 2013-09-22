#ifndef EXAMPLE_QT_SERVANT_HPP
#define EXAMPLE_QT_SERVANT_HPP

#include <boost/asynchronous/extensions/qt/qt_servant.hpp>

struct QtServant : public QObject
                 , public boost::asynchronous::qt_servant<void>
                 , public boost::asynchronous::qt_servant<int>
{
    Q_OBJECT

public:
    QtServant();
public slots:
    void signaled();
    void signaled2();
};

#endif // EXAMPLE_QT_SERVANT_HPP
