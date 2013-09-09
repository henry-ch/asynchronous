#include <QApplication>
#include <QPushButton>
#include <iostream>
#include <boost/enable_shared_from_this.hpp>

#include <boost/asynchronous/scheduler/threadpool_scheduler.hpp>
#include <boost/asynchronous/queue/threadsafe_list.hpp>
#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include "example_qt_servant.hpp"

using namespace std;

QtServant::QtServant()
    : boost::asynchronous::qt_servant<>(// threadpool with 3 threads and a simple threadsafe_list queue
                                        boost::asynchronous::create_shared_scheduler_proxy(
                                            new boost::asynchronous::threadpool_scheduler<
                                                    boost::asynchronous::threadsafe_list<> >(3)))
{
}
void QtServant::signaled()
{
    // start long tasks in threadpool (first lambda) and callback in our thread
    post_callback(
           [](){
                    std::cout << "Long Work" << std::endl;
                    std::cout << "in thread: " << boost::this_thread::get_id() << std::endl;
                    boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
                }// work
            ,
           // the lambda calls Servant, just to show that all is safe, Servant is alive if this is called
           [](){
                      std::cout << "in callback: " << boost::this_thread::get_id() << std::endl;
           }// callback functor.
    );
}
void QtServant::signaled2()
{
    // start long tasks in threadpool (first lambda) and callback in our thread
    // unfortunately due to Qt's limitations (no template or nested slot classes, no template slots)
    // we cannot return results so we need to cheat and do the work manually...
    boost::shared_ptr<boost::promise<int> > p = boost::make_shared<boost::promise<int> >();
    boost::shared_future<int> fu = p->get_future();
    post_callback(
           [p](){
                    try{
                        std::cout << "Long Work" << std::endl;
                        std::cout << "in thread: " << boost::this_thread::get_id() << std::endl;
                        boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
                        // set result manually
                        p->set_value(42);
                    }
                    catch(std::exception e)
                    {
                        p->set_exception(boost::copy_exception(e));
                    }
                }// work
            ,
           // the lambda calls Servant, just to show that all is safe, Servant is alive if this is called
           [fu]() mutable {
                      std::cout << "in callback: " << boost::this_thread::get_id() << std::endl;
                      std::cout << "task threw exception?: " << std::boolalpha << fu.has_exception() << std::endl;
                      std::cout << "result: " << fu.get() << std::endl;
           }// callback functor.
    );
}

void example_qt_servant(int argc, char *argv[])
{
    std::cout << "example_qt_servant" << std::endl;
    {
        QApplication a(argc, argv);
        QPushButton w("Push Me");
        QtServant s;
        w.show();
        QObject::connect(&w, SIGNAL(clicked()),&s, SLOT(signaled()) );
        std::cout << "main in thread: " << boost::this_thread::get_id() << std::endl;
        a.exec();
    }
    std::cout << "end example_qt_servant \n" << std::endl;
}
void example_qt_servant2(int argc, char *argv[])
{
    std::cout << "example_qt_servant2" << std::endl;
    {
        QApplication a(argc, argv);
        QPushButton w("Push Me");
        QtServant s;
        w.show();
        QObject::connect(&w, SIGNAL(clicked()),&s, SLOT(signaled2()) );
        std::cout << "main in thread: " << boost::this_thread::get_id() << std::endl;
        a.exec();
    }
    std::cout << "end example_qt_servant2 \n" << std::endl;
}


