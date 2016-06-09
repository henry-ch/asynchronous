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
#include <boost/utility/enable_if.hpp>
#include <boost/type_traits/is_same.hpp>

// expected is the "future" type of asynchronous, used in post_callback.
// It will contain a value if a task completed successfully, an exception if one was thrown.
// CAUTION: a contained type must be default-constructible.
// unlike futures, calling get() will never block.

namespace boost { namespace asynchronous {

template<class T, class Enable=void>
class expected
{
public:
    typedef T value_type;

    expected()
        : m_value()
        , m_exception() {}

    expected(value_type value)
        : m_value(std::move(value))
        , m_exception() {}

    expected(boost::exception_ptr ex)
    : m_value()
    , m_exception(std::move(ex)) {}

    expected(expected&& rhs) noexcept
        : m_value(std::move(rhs.m_value))
        , m_exception(std::move(rhs.m_exception))
    {
    }
    expected(expected const& rhs) noexcept
        : m_value(rhs.m_value)
        , m_exception(rhs.m_exception)
    {
    }
    expected& operator=(expected&& rhs) noexcept
    {
        std::swap(m_value,rhs.m_value);
        std::swap(m_exception,rhs.m_exception);
        return *this;
    }
    expected& operator=(expected const& rhs) noexcept
    {
        m_value = rhs.m_value;
        m_exception = rhs.m_exception;
        return *this;
    }
    void set_exception(const boost::exception_ptr & ex)
    {
      m_exception = ex;
    }
    void set_value(value_type value)
    {
        m_value=std::move(value);
    }
    value_type& get()
    {
        if (m_exception)
        {
            boost::rethrow_exception(m_exception);
        }
        return m_value;
    }
    bool has_exception() const
    {
        return !!m_exception;
    }
    boost::exception_ptr get_exception_ptr()
    {
        return m_exception;
    }

    bool has_value() const
    {
        return !has_exception();
    }

private:
    value_type                m_value;
    boost::exception_ptr      m_exception;
};

template<class T>
class expected<T, typename ::boost::enable_if<boost::is_same<T,void>>::type>
{
public:
    typedef void value_type;

    expected()
    : m_exception() {}

    explicit expected(boost::exception_ptr ex)
    : m_exception(std::move(ex)) {}

    expected(expected const& rhs) noexcept
        : m_exception(rhs.m_exception)
    {
    }

    expected(expected&& rhs) noexcept
    : m_exception(std::move(rhs.m_exception))
    {
    }

    expected& operator=(expected&& rhs) noexcept
    {
        std::swap(m_exception,rhs.m_exception);
        return *this;
    }
    expected& operator=(expected const& rhs) noexcept
    {
        m_exception = rhs.m_exception;
        return *this;
    }
    void set_exception(const boost::exception_ptr & ex)
    {
      m_exception = ex;
    }
    void set_value()
    {
    }
    void get()
    {
        if (m_exception)
        {
            boost::rethrow_exception(m_exception);
        }
    }
    bool has_exception() const
    {
        return !!m_exception;
    }
    boost::exception_ptr get_exception_ptr()
    {
        return m_exception;
    }
    bool has_value() const
    {
        return !has_exception();
    }

private:
    boost::exception_ptr      m_exception;
};

}}
#endif // BOOST_ASYNCHRONOUS_EXPECTED_HPP
