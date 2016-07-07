#ifndef EXAMPLE_QT_SERVANT_HPP
#define EXAMPLE_QT_SERVANT_HPP

#include <boost/asynchronous/extensions/qt/qt_servant.hpp>

struct QtServant : public QObject
                 , public boost::asynchronous::qt_servant<>
{
    Q_OBJECT

public:
    QtServant();
public slots:
    void signaled();
    void signaled2();
    void signaled3();
    void signaled4();
};

#endif // EXAMPLE_QT_SERVANT_HPP
