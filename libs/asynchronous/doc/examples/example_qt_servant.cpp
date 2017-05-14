#include <QApplication>
#include <QPushButton>
#include <iostream>


#include <boost/asynchronous/scheduler/threadpool_scheduler.hpp>
#include <boost/asynchronous/queue/lockfree_queue.hpp>
#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include "example_qt_servant.hpp"

using namespace std;

QtServant::QtServant()
    : boost::asynchronous::qt_servant<>
      // threadpool with 3 threads
      (boost::asynchronous::make_shared_scheduler_proxy<
            boost::asynchronous::threadpool_scheduler<
                    boost::asynchronous::lockfree_queue<>>>(3))
{
    m_safe_cb =
            make_safe_callback([]()
            {
              std::cout << "Thread: " << boost::this_thread::get_id() << ", make_safe_callback" << std::endl;
            },"",0);
}
void QtServant::signaled()
{
    // start long tasks in threadpool (first lambda) and callback in our thread
    post_callback(
           [](){
                    std::cout << "Long Work" << std::endl;
                    std::cout << "in thread: " << boost::this_thread::get_id() << std::endl;
                    boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
                },// work
           // the lambda calls Servant, just to show that all is safe, Servant is alive if this is called
           [](boost::asynchronous::expected<void>){
                      std::cout << "work done. Thread in callback: " << boost::this_thread::get_id() << std::endl;
           }// callback functor.
           ,"",0,0);
}
void QtServant::signaled2()
{
    // start long tasks in threadpool (first lambda) and callback in our thread
    post_callback(
           [](){
                    std::cout << "Long Work" << std::endl;
                    std::cout << "in thread: " << boost::this_thread::get_id() << std::endl;
                    boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
                    return 42;
                },// work
           // the lambda calls Servant, just to show that all is safe, Servant is alive if this is called
           [](boost::asynchronous::expected<int> fu) {
                      std::cout << "in callback: " << boost::this_thread::get_id() << std::endl;
                      std::cout << "task threw exception?: " << std::boolalpha << fu.has_exception() << std::endl;
                      std::cout << "result: " << fu.get() << std::endl;
           }// callback functor.
           ,"",0,0);
}
void QtServant::signaled3()
{
    // start long tasks in threadpool (first lambda) and callback in our thread
    auto safe = this->make_safe_callback(
                [](int i, std::string s)
                {
                    std::cout << "Thread: " << boost::this_thread::get_id() << ",safe callback with int:" << i
                              << " and string: " << s << std::endl;
                }
                ,"",0);
    post_callback(
           [safe](){
                    std::cout << "In threadpool" << std::endl;
                    std::cout << "in thread: " << boost::this_thread::get_id() << std::endl;
                    safe(42,std::string("hello"));
                },// work
           // ignore
           [](boost::asynchronous::expected<void> ) {
           }// callback functor.
           ,"",0,0);
}

void QtServant::signaled4()
{
    // post to self should be same thread
    post_self([]()
              {
                std::cout << "Thread: " << boost::this_thread::get_id() << ", post self" << std::endl;
              },"",0);
}
void QtServant::signaled5()
{
    // make_safe_callback should be same thread
    m_safe_cb();
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

void example_qt_servant3(int argc, char *argv[])
{
    std::cout << "example_qt_servant3" << std::endl;
    {
        QApplication a(argc, argv);
        QPushButton w("Push Me");
        QtServant s;
        w.show();
        QObject::connect(&w, SIGNAL(clicked()),&s, SLOT(signaled3()) );
        std::cout << "main in thread: " << boost::this_thread::get_id() << std::endl;
        a.exec();
    }
    std::cout << "end example_qt_servant3 \n" << std::endl;
}

void example_qt_servant4(int argc, char *argv[])
{
    std::cout << "example_qt_servant4" << std::endl;
    {
        QApplication a(argc, argv);
        QPushButton w("Push Me");
        QtServant s;
        w.show();
        QObject::connect(&w, SIGNAL(clicked()),&s, SLOT(signaled4()) );
        std::cout << "main in thread: " << boost::this_thread::get_id() << std::endl;
        a.exec();
    }
    std::cout << "end example_qt_servant4 \n" << std::endl;
}

void example_qt_servant5(int argc, char *argv[])
{
    std::cout << "example_qt_servant5" << std::endl;
    {
        QApplication a(argc, argv);
        QPushButton w("Push Me");
        QtServant s;
        w.show();
        QObject::connect(&w, SIGNAL(clicked()),&s, SLOT(signaled5()) );
        std::cout << "main in thread: " << boost::this_thread::get_id() << std::endl;
        a.exec();
    }
    std::cout << "end example_qt_servant5 \n" << std::endl;
}
void example_qt_servant6(int argc, char *argv[])
{
    std::cout << "example_qt_servant6 (safe callback and thread)" << std::endl;
    {
        QApplication a(argc, argv);
        QPushButton w("Push Me");
        QtServant s;
        w.show();
        std::cout << "main in thread: " << boost::this_thread::get_id() << std::endl;
        auto cb = s.m_safe_cb;
        boost::thread t([cb]()
        {
            std::cout << "extra with id: " << boost::this_thread::get_id() << std::endl;
            cb();
            cb();
            cb();
        });
        a.exec();
        t.join();
    }
    std::cout << "end example_qt_servant6 \n" << std::endl;
}
