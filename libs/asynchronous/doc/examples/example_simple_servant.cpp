#include <iostream>

#include <boost/asynchronous/scheduler/single_thread_scheduler.hpp>
#include <boost/asynchronous/queue/threadsafe_list.hpp>
#include <boost/asynchronous/scheduler_shared_proxy.hpp>
#include <boost/asynchronous/servant_proxy.hpp>
#include <boost/asynchronous/post.hpp>

using namespace std;

namespace
{
struct Servant
{
    // optional, ctor is simple enough not to be posted
    typedef int simple_ctor;
    Servant(boost::asynchronous::any_weak_scheduler<>,int data): m_data(data){}
    int doIt()const
    {
        std::cout << "Servant::doIt with m_data:" << m_data << std::endl;
        return 5;
    }
    void foo(int i)const
    {
        std::cout << "Servant::foo with int:" << i << std::endl;
    }
    void foobar(int i, char c)const
    {
        std::cout << "Servant::foobar with int:" << i << " and char:" << c <<std::endl;
    }
    int m_data;
};

class ServantProxy : public boost::asynchronous::servant_proxy<ServantProxy,Servant>
{
public:
    // forwarding constructor. Scheduler to servant_proxy, followed by arguments to Servant.
    template <class Scheduler>
    ServantProxy(Scheduler s, int data):
        boost::asynchronous::servant_proxy<ServantProxy,Servant>(s, data)
    {}
    // the following members must be available "outside"
    // foo and foobar, just as a post (no interesting return value)
    BOOST_ASYNC_POST_MEMBER(foo)
    BOOST_ASYNC_POST_MEMBER(foobar)
    // for doIt, we'd like a future
    BOOST_ASYNC_FUTURE_MEMBER(doIt)
};

}

void exercise_simple_servant()
{
    int something = 3;
    {
        // with c++11
        auto scheduler = boost::asynchronous::create_shared_scheduler_proxy(
                    new boost::asynchronous::single_thread_scheduler<
                          boost::asynchronous::threadsafe_list<> >);

        {
            // arguments (here 42) are forwarded to Servant's constructor
            ServantProxy proxy(scheduler,42);
            // post a call to foobar, arguments are forwarded.
            proxy.foobar(1,'a');
            // post a call to foo. To avoid races, "something" is moved.
            proxy.foo(std::move(something));
            // post and get a future because we're interested in the result.
            boost::shared_future<int> fu = proxy.doIt();
            std::cout<< "future:" << fu.get() << std::endl;
        }// here, Servant's destructor is posted
    }// scheduler is gone, its thread has been joined
}


