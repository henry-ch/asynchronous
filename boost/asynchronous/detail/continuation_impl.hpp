#ifndef BOOST_ASYNCHRONOUS_CONTINUATION_IMPL_HPP
#define BOOST_ASYNCHRONOUS_CONTINUATION_IMPL_HPP

#include <vector>
#include <tuple>
#include <utility>

#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>

#include <boost/asynchronous/post.hpp>
#include <boost/asynchronous/any_scheduler.hpp>
#include <boost/asynchronous/scheduler/detail/interrupt_state.hpp>

BOOST_MPL_HAS_XXX_TRAIT_DEF(state)

namespace boost { namespace asynchronous { namespace detail {

// the continuation task implementation
template <class Return, typename Job, typename Tuple=std::tuple<boost::future<Return> > >
struct continuation
{
    typedef int is_continuation_task;
    typedef Return return_type;
    typedef Tuple tuple_type;

    template <typename... Args>
    continuation(boost::shared_ptr<boost::asynchronous::detail::interrupt_state> state,boost::shared_ptr<Tuple> t,Args&&... args)
    : m_futures(t)
    , m_state(state)
    {
        boost::asynchronous::any_weak_scheduler<Job> weak_scheduler = boost::asynchronous::get_thread_scheduler<Job>();
        boost::asynchronous::any_shared_scheduler<Job> locked_scheduler = weak_scheduler.lock();
        std::vector<boost::asynchronous::any_interruptible> interruptibles;
        if (locked_scheduler.is_valid())
        {
            continuation_ctor_helper(locked_scheduler,interruptibles,std::forward<Args>(args)...);
        }
        if (m_state)
            m_state->add_subs(interruptibles.begin(),interruptibles.end());
    }
    // version where we already get futures
    continuation(boost::shared_ptr<boost::asynchronous::detail::interrupt_state> state,boost::shared_ptr<Tuple> t)
    : m_futures(t)
    , m_state(state)
    {
        //TODO interruptible
    }
    template <typename T,typename Interruptibles,typename Last>
    void continuation_ctor_helper(T& sched, Interruptibles& interruptibles,Last&& l)
    {
        if (!m_state)
        {
            // no interruptible requested
            boost::asynchronous::post_future(sched,std::forward<Last>(l),l.get_name());
        }
        else if(!m_state->is_interrupted())
        {
            // interruptible requested
            interruptibles.push_back(boost::asynchronous::interruptible_post_future(sched,std::forward<Last>(l),l.get_name()).second);
        }
    }

    template <typename T,typename Interruptibles,typename... Tail, typename Front>
    void continuation_ctor_helper(T& sched, Interruptibles& interruptibles,Front&& front,Tail&&... tail)
    {
        if (!m_state)
        {
            // no interruptible requested
            boost::asynchronous::post_future(sched,std::forward<Front>(front),front.get_name());
        }
        else
        {
            interruptibles.push_back(boost::asynchronous::interruptible_post_future(sched,std::forward<Front>(front),front.get_name()).second);
        }
        continuation_ctor_helper(sched,interruptibles,std::forward<Tail>(tail)...);
    }

    void operator()()
    {
        m_done(std::move(*m_futures));
    }

    template <class Func>
    void on_done(Func f)
    {
        m_done = f;
    }
    bool is_ready()
    {
        if (!!m_state && m_state->is_interrupted())
        {
            // we are interrupted => we are ready
            return true;
        }
        bool ready=true;
        check_ready(ready,*m_futures);
        return ready;
    }
//TODO temporary
//private:
    boost::shared_ptr<Tuple> m_futures;
    std::function<void(Tuple&&)> m_done;
    boost::shared_ptr<boost::asynchronous::detail::interrupt_state> m_state;

    template<std::size_t I = 0, typename... Tp>
    inline typename std::enable_if<I == sizeof...(Tp), void>::type
    abort_futures(std::tuple<Tp...> const& )const
    { }
    template<std::size_t I = 0, typename... Tp>
    inline typename std::enable_if<I < sizeof...(Tp), void>::type
    abort_futures(std::tuple<Tp...> const& t)const
    {

    }

    template<std::size_t I = 0, typename... Tp>
    inline typename std::enable_if<I == sizeof...(Tp), void>::type
    check_ready(bool& ,std::tuple<Tp...> const& )const
    { }

    template<std::size_t I = 0, typename... Tp>
    inline typename std::enable_if<I < sizeof...(Tp), void>::type
    check_ready(bool& ready,std::tuple<Tp...> const& t)const
    {
        if (!(std::get<I>(t)).has_value())
        {
            ready=false;
            return;
        }
        check_ready<I + 1, Tp...>(ready,t);
    }

};

// used by variadic make_future_tuple
template <typename T>
auto call_get_future(T& t) -> decltype(t.get_future())
{
    return t.get_future();
}

//create a tuple of futures
template <typename... Args>
auto make_future_tuple(Args&... args) -> boost::shared_ptr<decltype(std::make_tuple(call_get_future(args)...))>
{
    boost::shared_ptr<decltype(std::make_tuple(call_get_future(args)...))> sp =
            boost::make_shared<decltype(std::make_tuple(call_get_future(args)...))>
                 (std::make_tuple(call_get_future(args)...));
    return sp;
}

template <typename Front,typename... Tail>
struct has_future_args_helper
{
    typedef typename has_state<Front>::type type;
};

template <typename... Args>
struct has_future_args
{
    typedef typename has_future_args_helper<Args...>::type type;
};

}}}

#endif // BOOST_ASYNCHRONOUS_CONTINUATION_IMPL_HPP
