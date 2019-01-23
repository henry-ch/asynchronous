// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2018
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNCHRON_EXCEPTIONS_HPP
#define BOOST_ASYNCHRON_EXCEPTIONS_HPP

#include <exception>
#include <typeindex>
#include <vector>
#include <boost/current_function.hpp>

namespace boost { namespace asynchronous
{
template<
    class SomethingDerivedFromAsynchronousException>
[[ noreturn ]]
void asynchronous_throw(SomethingDerivedFromAsynchronousException ex, const char* currentFunction, const char* file, int line)
{
  ex.type = std::type_index(typeid(ex));
  ex.what_ = ex.what();
  ex.currentFunction_ = currentFunction;
  ex.file_ = file;
  ex.line_ = line;
  throw ex;
}

template<
    class SomethingDerivedFromAsynchronousException>
std::exception_ptr asynchronous_create_exception(SomethingDerivedFromAsynchronousException ex, const char* currentFunction, const char* file, int line)
{
    std::exception_ptr eptr;
    try
    {
      ex.type = std::type_index(typeid(ex));
      ex.what_ = ex.what();
      ex.currentFunction_ = currentFunction;
      ex.file_ = file;
      ex.line_ = line;
      throw ex;
    }
    catch(...)
    {
        eptr = std::current_exception();
    }
    return eptr;
}

}}
#define ASYNCHRONOUS_THROW(SomethingDerivedFromAsynchronousException_) boost::asynchronous::asynchronous_throw(SomethingDerivedFromAsynchronousException_, BOOST_CURRENT_FUNCTION, __FILE__, __LINE__)

#define ASYNCHRONOUS_CREATE_EXCEPTION(SomethingDerivedFromAsynchronousException_) boost::asynchronous::asynchronous_create_exception(SomethingDerivedFromAsynchronousException_, BOOST_CURRENT_FUNCTION, __FILE__, __LINE__)


namespace boost { namespace asynchronous
{

struct asynchronous_exception: public std::exception
{
  virtual ~asynchronous_exception() throw () {}

  virtual const char* what() const throw ()
  {
    return "asynchronous_exception";
  }

  std::type_index type = std::type_index(typeid(asynchronous_exception));
  const char* what_ = "asynchronous_exception";
  const char* currentFunction_ = "";
  const char* file_ = "";
  int line_ = -1;
};

// exception thrown by interruptible_post_callback.
struct task_aborted_exception : public boost::asynchronous::asynchronous_exception//std::exception
{
    virtual const char* what() const throw ()
    {
      return "task_aborted_exception";
    }
};

// Exception consisting of multiple underlying exceptions.
struct combined_exception : public boost::asynchronous::asynchronous_exception
{
    combined_exception(size_t count)
        : m_underlying(count)
    {}

    const char* what() const noexcept override { return "any_of_exception"; }

    std::vector<std::exception_ptr>& underlying_exceptions() { return m_underlying; }
    const std::vector<std::exception_ptr>& underlying_exceptions() const { return m_underlying; }

private:
    std::vector<std::exception_ptr> m_underlying;
};

}}
#endif // BOOST_ASYNCHRON_EXCEPTIONS_HPP
