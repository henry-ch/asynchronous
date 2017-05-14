// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2016
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNCHRONOUS_EXPECTED_HPP
#define BOOST_ASYNCHRONOUS_EXPECTED_HPP

#include <utility>
#include <boost/exception_ptr.hpp>
#include <type_traits>
#include <boost/type_traits/is_same.hpp>
#include <boost/smart_ptr.hpp>
#include <type_traits>

namespace boost { namespace asynchronous {

/*!
 * \class expected
 * expected is the return value of tasks executed by a scheduler, usually a threadpool.
 * It contains either the value returned by the task or an exception if the task failed.
 * Calling get() will return any of those.
 * Version for types which are not default-constructible
 */
template<class T, class Enable=void>
class expected
{
public:
    typedef T value_type;

    /*!
     * \brief default constructor
     */
    expected()
        : m_value()
        , m_exception() {}

    /*!
     * \brief constructor taking a value as argument.
     */
    expected(value_type value)
        : m_value(boost::make_shared<value_type>(std::move(value)))
        , m_exception() {}

    /*!
     * \brief constructor taking an exception_ptr as argument
     */
    expected(boost::exception_ptr ex)
    : m_value()
    , m_exception(std::move(ex)) {}

    /*!
     * \brief move constructor noexcept
     */
    expected(expected&& rhs) noexcept
        : m_value(std::move(rhs.m_value))
        , m_exception(std::move(rhs.m_exception))
    {
    }

    /*!
     * \brief copy constructor noexcept
     */
    expected(expected const& rhs) noexcept
        : m_value(boost::make_shared<value_type>(*rhs.m_value))
        , m_exception(rhs.m_exception)
    {
    }

    /*!
     * \brief move assignment operator noexcept
     */
    expected& operator=(expected&& rhs) noexcept
    {
        std::swap(m_value,rhs.m_value);
        std::swap(m_exception,rhs.m_exception);
        return *this;
    }

    /*!
     * \brief copy assignment operator noexcept
     */
    expected& operator=(expected const& rhs) noexcept
    {
        if(!!m_value)
            *m_value=*rhs.m_value;
        else
            m_value = boost::make_shared<value_type>(*rhs.m_value);
        m_exception = rhs.m_exception;
        return *this;
    }

    /*!
     * \brief sets the exception, calling get() later will throw
     * \param boost::exception_ptr the exception which will be thrown
     */
    void set_exception(const boost::exception_ptr & ex)
    {
      m_exception = ex;
    }

    /*!
     * \brief sets the value, calling get() later will return this value
     * \param value the value which will be returned
     */
    void set_value(value_type value)
    {
        if(!!m_value)
            *m_value=std::move(value);
        else
            m_value = boost::make_shared<value_type>(std::move(value));
    }

    /*!
     * \brief returns by reference the contained value
     * \return contained value or exception thrown if set. If both are set, the exception wins
     */
    value_type& get()
    {
        if (m_exception)
        {
            boost::rethrow_exception(m_exception);
        }
        return *m_value;
    }

    /*!
     * \brief const version. returns by reference the contained value
     * \return contained value or exception thrown if set. If both are set, the exception wins
     */
    value_type const& get() const
    {
        if (m_exception)
        {
            boost::rethrow_exception(m_exception);
        }
        return *m_value;
    }

    /*!
     * \brief returns whether an exception has been set without throwing it
     * \return bool
     */
    bool has_exception() const
    {
        return !!m_exception;
    }

    /*!
     * \brief returns the stored exception pointer if any
     * \return boost::exception_ptr
     */
    boost::exception_ptr get_exception_ptr()
    {
        return m_exception;
    }

    /*!
     * \brief returns whether a value has been set
     * \return bool
     */
    bool has_value() const
    {
        return !has_exception();
    }

private:
    boost::shared_ptr<value_type>   m_value;
    boost::exception_ptr            m_exception;
};

/*!
 * \class expected
 * expected is the return value of tasks executed by a scheduler, usually a threadpool.
 * It contains either the value returned by the task or an exception if the task failed.
 * Calling get() will return any of those.
 * Version for types which are default-constructible
 */
template<class T>
class expected<T, typename std::enable_if<std::is_default_constructible<T>::value>::type>
{
public:
    typedef T value_type;

    /*!
     * \brief default constructor
     */
    expected()
        : m_value()
        , m_exception() {}

    /*!
     * \brief constructor taking a value as argument.
     */
    expected(value_type value)
        : m_value(std::move(value))
        , m_exception() {}

    /*!
     * \brief constructor taking an exception_ptr as argument
     */
    expected(boost::exception_ptr ex)
    : m_value()
    , m_exception(std::move(ex)) {}

    /*!
     * \brief move constructor noexcept
     */
    expected(expected&& rhs) noexcept
        : m_value(std::move(rhs.m_value))
        , m_exception(std::move(rhs.m_exception))
    {
    }

    /*!
     * \brief copy constructor noexcept
     */
    expected(expected const& rhs) noexcept
        : m_value(rhs.m_value)
        , m_exception(rhs.m_exception)
    {
    }

    /*!
     * \brief move assignment operator noexcept
     */
    expected& operator=(expected&& rhs) noexcept
    {
        std::swap(m_value,rhs.m_value);
        std::swap(m_exception,rhs.m_exception);
        return *this;
    }

    /*!
     * \brief copy assignment operator noexcept
     */
    expected& operator=(expected const& rhs) noexcept
    {
        m_value = rhs.m_value;
        m_exception = rhs.m_exception;
        return *this;
    }

    /*!
     * \brief sets the exception, calling get() later will throw
     * \param boost::exception_ptr the exception which will be thrown
     */
    void set_exception(const boost::exception_ptr & ex)
    {
      m_exception = ex;
    }

    /*!
     * \brief sets the value, calling get() later will return this value
     * \param value the value which will be returned
     */
    void set_value(value_type value)
    {
        m_value=std::move(value);
    }

    /*!
     * \brief returns by reference the contained value
     * \return contained value or exception thrown if set. If both are set, the exception wins
     */
    value_type& get()
    {
        if (m_exception)
        {
            boost::rethrow_exception(m_exception);
        }
        return m_value;
    }

    /*!
     * \brief const version. returns by reference the contained value
     * \return contained value or exception thrown if set. If both are set, the exception wins
     */
    value_type const& get() const
    {
        if (m_exception)
        {
            boost::rethrow_exception(m_exception);
        }
        return m_value;
    }

    /*!
     * \brief returns whether an exception has been set without throwing it
     * \return bool
     */
    bool has_exception() const
    {
        return !!m_exception;
    }

    /*!
     * \brief returns the stored exception pointer if any
     * \return boost::exception_ptr
     */
    boost::exception_ptr get_exception_ptr()
    {
        return m_exception;
    }

    /*!
     * \brief returns whether a value has been set
     * \return bool
     */
    bool has_value() const
    {
        return !has_exception();
    }

private:
    value_type                m_value;
    boost::exception_ptr      m_exception;
};

/*!
 * \class expected
 * expected is the return value of tasks executed by a scheduler, usually a threadpool.
 * It contains either the value returned by the task or an exception if the task failed.
 * Calling get() will return any of those.
 * Version for void as template type.
 */
template<class T>
class expected<T, typename std::enable_if<boost::is_same<T,void>::value>::type>
{
public:
    typedef void value_type;

    /*!
     * \brief default constructor
     */
    expected()
    : m_exception() {}

    /*!
     * \brief constructor taking an exception_ptr as argument
     */
    explicit expected(boost::exception_ptr ex)
    : m_exception(std::move(ex)) {}

    /*!
     * \brief copy constructor noexcept
     */
    expected(expected const& rhs) noexcept
        : m_exception(rhs.m_exception)
    {
    }

    /*!
     * \brief move constructor noexcept
     */
    expected(expected&& rhs) noexcept
    : m_exception(std::move(rhs.m_exception))
    {
    }

    /*!
     * \brief move assignment operator noexcept
     */
    expected& operator=(expected&& rhs) noexcept
    {
        std::swap(m_exception,rhs.m_exception);
        return *this;
    }

    /*!
     * \brief copy assignment operator noexcept
     */
    expected& operator=(expected const& rhs) noexcept
    {
        m_exception = rhs.m_exception;
        return *this;
    }

    /*!
     * \brief sets the exception, calling get() later will throw
     * \param boost::exception_ptr the exception which will be thrown
     */
    void set_exception(const boost::exception_ptr & ex)
    {
      m_exception = ex;
    }

    /*!
     * \brief sets the value, calling get() later will return this value
     * \param value the value which will be returned
     */
    void set_value()
    {
    }

    /*!
     * \brief Check if exception to rethrow. If not, empty implementation.
     * \return nothing. Rethrows if exception contained
     */
    void get()
    {
        if (m_exception)
        {
            boost::rethrow_exception(m_exception);
        }
    }

    /*!
     * \brief const version. Check if exception to rethrow. If not, empty implementation.
     * \return nothing. Rethrows if exception contained
     */
    void get() const
    {
        if (m_exception)
        {
            boost::rethrow_exception(m_exception);
        }
    }

    /*!
     * \brief returns whether an exception has been set without throwing it
     * \return bool
     */
    bool has_exception() const
    {
        return !!m_exception;
    }

    /*!
     * \brief returns the stored exception pointer if any
     * \return boost::exception_ptr
     */
    boost::exception_ptr get_exception_ptr()
    {
        return m_exception;
    }

    /*!
     * \brief returns whether a value has been set (no exception contained)
     * \return bool
     */
    bool has_value() const
    {
        return !has_exception();
    }

private:
    boost::exception_ptr      m_exception;
};

}}
#endif // BOOST_ASYNCHRONOUS_EXPECTED_HPP
