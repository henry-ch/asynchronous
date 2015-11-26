// Boost.Asynchronous library
//  Copyright (C) Tobias Holl 2015
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#include <boost/random/random_device.hpp>
#include <boost/thread/tss.hpp>

#ifndef BOOST_ASYNCHRONOUS_RANDOM_PROVIDER_HPP
#define BOOST_ASYNCHRONOUS_RANDOM_PROVIDER_HPP

namespace boost { namespace asynchronous
{

template <typename RandomEngine>
struct random_provider
{
    static boost::thread_specific_ptr<RandomEngine> generator;

    template <typename Distribution>
    static decltype(std::declval<Distribution>()(*generator)) generate(Distribution & distribution)
    {
        if (generator.get() == nullptr)
        {
            generator.reset(new RandomEngine(boost::random::random_device{}()));
        }

        return distribution(*generator);
    }

    static decltype((*generator)()) generate()
    {
        if (generator.get() == nullptr)
        {
            generator.reset(new RandomEngine(boost::random::random_device{}()));
        }

        return (*generator)();
    }
};

template <typename RandomEngine>
boost::thread_specific_ptr<RandomEngine> random_provider<RandomEngine>::generator;

/* The original version using the new thread_local keyword and standard library types
 * from <memory> and <random>
 *

template <typename RandomEngine>
struct random_provider
{
    static thread_local std::shared_ptr<RandomEngine> generator;

    template <typename Distribution>
    static decltype(std::declval<Distribution>()(*generator)) generate(Distribution & distribution)
    {
        if (!generator)
        {
            generator = std::make_shared<RandomEngine>(std::random_device{}());
        }

        return distribution(*generator);
    }

    static decltype((*generator)()) generate()
    {
        if (!generator)
        {
            generator = std::make_shared<RandomEngine>(std::random_device{}());
        }

        return (*generator)();
    }
};

template <typename RandomEngine>
thread_local std::shared_ptr<RandomEngine> random_provider<RandomEngine>::generator = nullptr;

 *
 * Since thread_local support is sparse amongst compilers, and older, compiler-specific workarounds
 * have limitations (such as being unable to use types with constructors), this code was rewritten
 * to use Boost.Thread and Boost.Random features as seen above.
 */

}}

#endif // BOOST_ASYNCHRONOUS_RANDOM_PROVIDER_HPP
