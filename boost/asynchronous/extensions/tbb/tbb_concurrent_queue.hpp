#ifndef BOOST_ASYNCHRONOUS_EXTENSIONS_CONCURRENT_QUEUE_HPP
#define BOOST_ASYNCHRONOUS_EXTENSIONS_CONCURRENT_QUEUE_HPP

#include <boost/asynchronous/callable_any.hpp>
#include <boost/asynchronous/queue/queue_base.hpp>
#include <boost/asynchronous/queue/any_queue.hpp>

#include <tbb/concurrent_queue.h>

namespace boost { namespace asynchronous
{

template <class JOB = boost::asynchronous::any_callable >
class tbb_concurrent_queue:
#ifdef BOOST_ASYNCHRONOUS_NO_TYPE_ERASURE
        public boost::asynchronous::any_queue_concept<JOB>,
#endif
        public boost::asynchronous::queue_base<JOB>, private boost::noncopyable
{
public:
    typedef boost::asynchronous::tbb_concurrent_queue<JOB> this_type;
    typedef JOB job_type;

    tbb_concurrent_queue(): m_queue()
    {
    }

    std::size_t get_queue_size() const
    {
        return m_queue.unsafe_size();
    }

    void push(JOB && j, std::size_t)
    {
        m_queue.push(std::forward<JOB>(j));
    }
    void push(JOB && j)
    {
        m_queue.push(std::forward<JOB>(j));
    }
    void push(JOB const& j, std::size_t=0)
    {
        m_queue.push(j);
    }

    //todo move?
    JOB pop()
    {
        JOB resp;
        while (!m_queue.try_pop(resp))
        {
            boost::this_thread::yield();
        }
        return resp;
    }
    bool try_pop(JOB& job)
    {
       return m_queue.try_pop(job);
    }
    //TODO at other end
    bool try_steal(JOB& job)
    {
        return try_pop(job);
    }
private:
    tbb::concurrent_queue<JOB> m_queue;
};

}} // boost::asynchronous


#endif // BOOST_ASYNCHRONOUS_EXTENSIONS_CONCURRENT_QUEUE_HPP
