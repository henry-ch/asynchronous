#ifndef BOOST_ASYNCHRONOUS_SCHEDULER_INTERRUPT_STATE_HPP
#define BOOST_ASYNCHRONOUS_SCHEDULER_INTERRUPT_STATE_HPP

#include <future>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>
#include <boost/asynchronous/detail/any_interruptible.hpp>

namespace boost { namespace asynchronous { namespace detail
{
struct interrupt_state
{
    interrupt_state():m_state(0){}
    bool is_interrupted()const
    {
        boost::mutex::scoped_lock lock(m_mutex);
        return (m_state == 2);
    }
    bool is_completed()const
    {
        boost::mutex::scoped_lock lock(m_mutex);
        return (m_state == 1);
    }
    bool is_running()const
    {
        boost::mutex::scoped_lock lock(m_mutex);
        return (m_state == 3);
    }
    void interrupt(std::shared_future<boost::thread*> fworker)
    {
        boost::mutex::scoped_lock lock(m_mutex);
        // if task is already interrupted, no nothing
        // if task is completed, ditto
        if (is_completed_int())
        {
            // mark that we are interrupted
            m_state = 2;
            // no need to protect m_state any more
            lock.unlock();
            // copy to minimize locking
            boost::mutex::scoped_lock ilock(m_interruptibles_mutex);
            std::vector<boost::asynchronous::any_interruptible> interruptibles = m_interruptibles;
            ilock.unlock();
            // interrupt subs if any
            for (std::vector<boost::asynchronous::any_interruptible>::iterator it = interruptibles.begin(); it != interruptibles.end();++it)
            {
                (*it).interrupt();
            }
            return;
        }
        if (is_interrupted_int())
        {
            return;
        }
        else if (is_running_int())
        {
            // copy to minimize locking
            boost::mutex::scoped_lock ilock(m_interruptibles_mutex);
            std::vector<boost::asynchronous::any_interruptible> interruptibles = m_interruptibles;
            ilock.unlock();
            // interrupt subs if any
            for (std::vector<boost::asynchronous::any_interruptible>::iterator it = interruptibles.begin(); it != interruptibles.end();++it)
            {
                (*it).interrupt();
            }
            // if true then we have a worker
            boost::thread* w = fworker.get();
            w->interrupt();
        }
        m_state = 2;
    }
    void complete()
    {
        boost::mutex::scoped_lock lock(m_mutex);
        m_state = 1;
    }
    void run()
    {
        boost::mutex::scoped_lock lock(m_mutex);
        if (!is_interrupted_int())
        {
            m_state = 3;
        }
    }
    template <class Iterator>
    void add_subs(Iterator beg, Iterator end)
    {
        boost::mutex::scoped_lock lock(m_interruptibles_mutex);
        m_interruptibles.insert(m_interruptibles.end(),beg,end);
    }
    void add_sub(boost::asynchronous::any_interruptible const& i)
    {
        boost::mutex::scoped_lock lock(m_interruptibles_mutex);
        m_interruptibles.push_back(i);
    }
private:
    bool is_interrupted_int()const
    {
        return (m_state == 2);
    }
    bool is_completed_int()const
    {
        return (m_state == 1);
    }
    bool is_running_int()const
    {
        return (m_state == 3);
    }
    // 0: not interrupted
    // 1: completed
    // 2: interrupted
    // 3: running
    unsigned short m_state;
    // protects m_state
    mutable boost::mutex m_mutex;
    // subtasks which need to be interrupted too. Used for continuations
    std::vector<boost::asynchronous::any_interruptible> m_interruptibles;
    // protects m_interruptibles
    mutable boost::mutex m_interruptibles_mutex;
};

}}}
#endif // BOOST_ASYNCHRONOUS_SCHEDULER_INTERRUPT_STATE_HPP
