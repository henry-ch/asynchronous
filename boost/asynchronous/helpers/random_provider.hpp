// Boost.Asynchronous library
//  Copyright (C) Tobias Holl 2018
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#include <algorithm>
#include <functional>

#ifdef BOOST_ASYNCHRONOUS_USE_STD_RANDOM
#include <memory>
#include <random>
#define BOOST_ASYNCHRONOUS_RANDOM_NAMESPACE std
#else
#include <boost/random/random_device.hpp>
#include <boost/thread/tss.hpp>
#define BOOST_ASYNCHRONOUS_RANDOM_NAMESPACE boost::random
#endif

#ifndef BOOST_ASYNCHRONOUS_RANDOM_PROVIDER_HPP
#define BOOST_ASYNCHRONOUS_RANDOM_PROVIDER_HPP

namespace boost { namespace asynchronous
{

namespace detail {

// A helper for seeding PRNGs.
// Conforms to the SeedSequence concept (both from Boost and from the C++ standard), except that only
// a default constructor is provided, because additional state would be discarded anyways.
// size() will always return zero, there are no pre-defined seed values here.
class random_device_seed_seq
{
public:
    template <typename RandomAccessIterator> void generate(RandomAccessIterator begin, RandomAccessIterator end) { std::generate(begin, end, std::ref(m_device)); }

    template <typename OutputIterator> void param(OutputIterator) const {}
    size_t size() const noexcept { return static_cast<size_t>(0); }

private:
    BOOST_ASYNCHRONOUS_RANDOM_NAMESPACE::random_device m_device;
};

} // namespace detail

// A thread-safe provider of random numbers, wrapping around any random engine
// For use with the standard library's <random>, you should define BOOST_ASYNCHORNOUS_USE_STD_RANDOM
// By default, random_provider will use Boost.Random internally.
template <typename RandomEngine>
struct random_provider
{
#ifdef BOOST_ASYNCHRONOUS_USE_STD_RANDOM
    static thread_local std::shared_ptr<RandomEngine> generator;
#else
    static boost::thread_specific_ptr<RandomEngine> generator;
#endif

    template <typename Distribution>
    static decltype(std::declval<Distribution>()(*generator)) generate(Distribution & distribution)
    {
        if (generator.get() == nullptr)
        {
            boost::asynchronous::detail::random_device_seed_seq seed_sequence;
            generator.reset(new RandomEngine(seed_sequence));
        }

        return distribution(*generator);
    }

    static decltype((*generator)()) generate()
    {
        if (generator.get() == nullptr)
        {
            boost::asynchronous::detail::random_device_seed_seq seed_sequence;
            generator.reset(new RandomEngine(seed_sequence));
        }

        return (*generator)();
    }
};

#ifdef BOOST_ASYNCHRONOUS_USE_STD_RANDOM
template <typename RandomEngine>
thread_local std::shared_ptr<RandomEngine> random_provider<RandomEngine>::generator = nullptr;
#else
template <typename RandomEngine>
boost::thread_specific_ptr<RandomEngine> random_provider<RandomEngine>::generator;
#endif

}}

#undef BOOST_ASYNCHRONOUS_RANDOM_NAMESPACE
#endif // BOOST_ASYNCHRONOUS_RANDOM_PROVIDER_HPP
