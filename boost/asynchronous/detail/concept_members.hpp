// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2013
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNC_CONCEPT_MEMBERS_HPP
#define BOOST_ASYNC_CONCEPT_MEMBERS_HPP

#include <boost/type_erasure/any.hpp>
#include <boost/type_erasure/member.hpp>
#include <boost/asynchronous/detail/any_interruptible.hpp>
                  
BOOST_TYPE_ERASURE_MEMBER((boost)(asynchronous)(has_scheduler_lock), scheduler_lock, 0);
BOOST_TYPE_ERASURE_MEMBER((boost)(asynchronous)(has_lock), lock, 0);
BOOST_TYPE_ERASURE_MEMBER((boost)(asynchronous)(has_thread_ids), thread_ids, 0);
BOOST_TYPE_ERASURE_MEMBER((boost)(asynchronous)(has_get_weak_scheduler), get_weak_scheduler, 0);
BOOST_TYPE_ERASURE_MEMBER((boost)(asynchronous)(has_reset), reset, 0);
BOOST_TYPE_ERASURE_MEMBER((boost)(asynchronous)(has_is_valid), is_valid, 0);
BOOST_TYPE_ERASURE_MEMBER((boost)(asynchronous)(has_join), join, 0);
BOOST_TYPE_ERASURE_MEMBER((boost)(asynchronous)(has_set_name), set_name, 1);
BOOST_TYPE_ERASURE_MEMBER((boost)(asynchronous)(has_get_name), get_name, 0);
BOOST_TYPE_ERASURE_MEMBER((boost)(asynchronous)(has_get_diagnostic_item), get_diagnostic_item, 0);
BOOST_TYPE_ERASURE_MEMBER((boost)(asynchronous)(has_get_queue_size), get_queue_size, 0);

BOOST_TYPE_ERASURE_MEMBER((boost)(asynchronous)(has_push), push,2);
BOOST_TYPE_ERASURE_MEMBER((boost)(asynchronous)(has_pop), pop,0);
BOOST_TYPE_ERASURE_MEMBER((boost)(asynchronous)(has_try_pop), try_pop,1);
BOOST_TYPE_ERASURE_MEMBER((boost)(asynchronous)(has_try_steal), try_steal,1);
BOOST_TYPE_ERASURE_MEMBER((boost)(asynchronous)(has_get_worker), get_worker,0);
BOOST_TYPE_ERASURE_MEMBER((boost)(asynchronous)(has_get_queues), get_queues,0);
BOOST_TYPE_ERASURE_MEMBER((boost)(asynchronous)(has_set_steal_from_queues), set_steal_from_queues,1);
BOOST_TYPE_ERASURE_MEMBER((boost)(asynchronous)(has_get_internal_scheduler_aspect), get_internal_scheduler_aspect,0);
BOOST_TYPE_ERASURE_MEMBER((boost)(asynchronous)(has_clear_diagnostics), clear_diagnostics,0);

BOOST_TYPE_ERASURE_MEMBER((boost)(asynchronous)(has_set_done_func), set_done_func, 1);

BOOST_TYPE_ERASURE_MEMBER((boost)(asynchronous)(has_processor_bind), processor_bind, 1);

BOOST_TYPE_ERASURE_MEMBER((boost)(asynchronous)(has_execute_in_all_threads), execute_in_all_threads, 1);

#ifdef BOOST_ASYNCHRONOUS_USE_TYPE_ERASURE
#ifndef BOOST_NO_RVALUE_REFERENCES
BOOST_TYPE_ERASURE_MEMBER((boost)(asynchronous)(has_post), post);
BOOST_TYPE_ERASURE_MEMBER((boost)(asynchronous)(has_interruptible_post), interruptible_post);
BOOST_TYPE_ERASURE_MEMBER((boost)(asynchronous)(has_add), add);
BOOST_TYPE_ERASURE_MEMBER((boost)(asynchronous)(has_interruptible_add), interruptible_add);
BOOST_TYPE_ERASURE_MEMBER((boost)(asynchronous)(has_get_diagnostics), get_diagnostics);
#else
BOOST_TYPE_ERASURE_MEMBER((boost)(asynchronous)(has_add), add,2);
BOOST_TYPE_ERASURE_MEMBER((boost)(asynchronous)(has_interruptible_add), interruptible_add,2);
BOOST_TYPE_ERASURE_MEMBER((boost)(asynchronous)(has_get_diagnostics), get_diagnostics, 0);

// no variadic macro, we use concept_interface directly
// post
// first we need a functor with 1,2 or 3 arguments
namespace boost{ namespace asynchronous
{
template<class T, class U, class V=void, class W=void>
struct post
{
    static void apply(const T& t, const U& u, const V& v, const W& w) { t.post(u,v,w); }
};
template<class T, class U>
struct post<T,U,void,void>
{
    static void apply(const T& t, const U& u) { t.post(u); }
};
template<class T, class U, class V>
struct post<T,U,V,void>
{
    static void apply(const T& t, const U& u, const V& v) { t.post(u,v); }
};

}}

// then we need 3*2 concept_interface specializations
namespace boost {
namespace type_erasure {

template<class T, class U, class Base, class Enable>
struct concept_interface< ::boost::asynchronous::post<T, U,void>, Base, T, Enable> : Base
{
    typedef void _fun_defined;
    void post(typename rebind_any<Base, const U&>::type arg)const
    {
        call(boost::asynchronous::post<T, U, void>(), *this, arg);
    }
};

template<class T, class U, class Base>
struct concept_interface< ::boost::asynchronous::post<T, U,void>, Base, T, typename Base::_fun_defined> : Base
{
    using Base::post;
    void post(typename rebind_any<Base, const U&>::type arg)const
    {
        call(boost::asynchronous::post<T, U,void>(), *this, arg);
    }
};

template<class T, class U, class V, class Base, class Enable>
struct concept_interface< ::boost::asynchronous::post<T, U,V>, Base, T, Enable> : Base
{
    typedef void _fun_defined;
    void post(typename rebind_any<Base, const U&>::type arg, typename rebind_any<Base, const V&>::type arg2)const
    {
        call(boost::asynchronous::post<T, U,V>(), *this, arg,arg2);
    }
};

template<class T, class U, class V, class Base>
struct concept_interface< ::boost::asynchronous::post<T, U,V>, Base, T, typename Base::_fun_defined> : Base
{
    using Base::post;
    void post(typename rebind_any<Base, const U&>::type arg, typename rebind_any<Base, const V&>::type arg2)const
    {
        call(boost::asynchronous::post<T, U,V>(), *this, arg,arg2);
    }
};

template<class T, class U, class V, class W, class Base, class Enable>
struct concept_interface< ::boost::asynchronous::post<T, U, V, W>, Base, T, Enable> : Base
{
    typedef void _fun_defined;
    void post(typename rebind_any<Base, const U&>::type arg, typename rebind_any<Base, const V&>::type arg2, typename rebind_any<Base, const W&>::type arg3)const
    {
        call(boost::asynchronous::post<T, U, V, W>(), *this, arg,arg2,arg3);
    }
};

template<class T, class U, class V, class W, class Base>
struct concept_interface< ::boost::asynchronous::post<T, U, V, W>, Base, T, typename Base::_fun_defined> : Base
{
    using Base::post;
    void post(typename rebind_any<Base, const U&>::type arg, typename rebind_any<Base, const V&>::type arg2, typename rebind_any<Base, const W&>::type arg3)const
    {
        call(boost::asynchronous::post<T, U,V,W>(), *this, arg,arg2,arg3);
    }
};
}
}

// interruptible_post
// first we need a functor with 1,2 or 3 arguments
namespace boost{ namespace asynchronous
{
template<class T, class U, class V=void, class W=void>
struct interruptible_post
{
    static boost::asynchronous::any_interruptible apply(const T& t, const U& u, const V& v, const W& w) { return t.interruptible_post(u,v,w); }
};
template<class T, class U>
struct interruptible_post<T,U,void,void>
{
    static boost::asynchronous::any_interruptible apply(const T& t, const U& u) { return t.interruptible_post(u); }
};
template<class T, class U, class V>
struct interruptible_post<T,U,V,void>
{
    static boost::asynchronous::any_interruptible apply(const T& t, const U& u, const V& v) { return t.interruptible_post(u,v); }
};

}}

// then we need 3*2 concept_interface specializations
namespace boost {
namespace type_erasure {

template<class T, class U, class Base, class Enable>
struct concept_interface< ::boost::asynchronous::interruptible_post<T, U,void>, Base, T, Enable> : Base
{
    typedef void _fun_defined;
    boost::asynchronous::any_interruptible interruptible_post(typename rebind_any<Base, const U&>::type arg)const
    {
        return call(boost::asynchronous::interruptible_post<T, U, void>(), *this, arg);
    }
};

template<class T, class U, class Base>
struct concept_interface< ::boost::asynchronous::interruptible_post<T, U,void>, Base, T, typename Base::_fun_defined> : Base
{
    using Base::interruptible_post;
    boost::asynchronous::any_interruptible interruptible_post(typename rebind_any<Base, const U&>::type arg)const
    {
        return call(boost::asynchronous::interruptible_post<T, U,void>(), *this, arg);
    }
};

template<class T, class U, class V, class Base, class Enable>
struct concept_interface< ::boost::asynchronous::interruptible_post<T, U,V>, Base, T, Enable> : Base
{
    typedef void _fun_defined;
    boost::asynchronous::any_interruptible interruptible_post(typename rebind_any<Base, const U&>::type arg, typename rebind_any<Base, const V&>::type arg2)const
    {
        return call(boost::asynchronous::interruptible_post<T, U,V>(), *this, arg,arg2);
    }
};

template<class T, class U, class V, class Base>
struct concept_interface< ::boost::asynchronous::interruptible_post<T, U,V>, Base, T, typename Base::_fun_defined> : Base
{
    using Base::interruptible_post;
    boost::asynchronous::any_interruptible interruptible_post(typename rebind_any<Base, const U&>::type arg, typename rebind_any<Base, const V&>::type arg2)const
    {
        return call(boost::asynchronous::interruptible_post<T, U,V>(), *this, arg,arg2);
    }
};

template<class T, class U, class V, class W, class Base, class Enable>
struct concept_interface< ::boost::asynchronous::interruptible_post<T, U, V, W>, Base, T, Enable> : Base
{
    typedef void _fun_defined;
    boost::asynchronous::any_interruptible interruptible_post(typename rebind_any<Base, const U&>::type arg, typename rebind_any<Base, const V&>::type arg2, typename rebind_any<Base, const W&>::type arg3)const
    {
        return call(boost::asynchronous::interruptible_post<T, U, V, W>(), *this, arg,arg2,arg3);
    }
};

template<class T, class U, class V, class W, class Base>
struct concept_interface< ::boost::asynchronous::interruptible_post<T, U, V, W>, Base, T, typename Base::_fun_defined> : Base
{
    using Base::interruptible_post;
    boost::asynchronous::any_interruptible interruptible_post(typename rebind_any<Base, const U&>::type arg, typename rebind_any<Base, const V&>::type arg2, typename rebind_any<Base, const W&>::type arg3)const
    {
        return call(boost::asynchronous::interruptible_post<T, U,V,W>(), *this, arg,arg2,arg3);
    }
};
}
}
#endif
#endif
#endif // BOOST_ASYNC_CONCEPT_MEMBERS_HPP
