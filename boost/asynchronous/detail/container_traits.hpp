#ifndef BOOST_ASYNCHRONOUS_CONTAINER_TRAITS_HPP
#define BOOST_ASYNCHRONOUS_CONTAINER_TRAITS_HPP

#include <iterator>

namespace boost
{
namespace asynchronous
{

// Helper namespace to avoid polluting boost::asynchronous::detail
namespace detail
{
namespace container_traits_helper
{
    struct invalid {};

    // Iterators

    template <typename Container, typename Iterator = decltype(std::begin(std::declval<Container>()))>
    Iterator begin_detector(const Container&);
    invalid  begin_detector(...);

    template <typename Container> struct iterator_type_detector { using type = decltype(begin_detector(std::declval<Container>())); };

    // Size

#if __cpp_lib_nonmember_container_access
    template <typename Container, typename Size = decltype(std::size(std::declval<Container>()))>
    Size    size_detector(const Container&);
    invalid size_detector(...);
#else
    template <typename Container, typename Size = decltype(std::distance(std::declval<typename iterator_type_detector<Container>::type>(), std::declval<typename iterator_type_detector<Container>::type>()))>
    Size    size_detector(const Container&);
    invalid size_detector(...);
#endif

    template <typename Container> struct size_type_detector { using type = decltype(size_detector(std::declval<Container>())); };

    // Values

    template <typename Container, typename Value = typename std::iterator_traits<typename iterator_type_detector<Container>::type>::value_type>
    Value   value_detector(const Container&);
    invalid value_detector(...);

    template <typename Container> struct value_type_detector { using type = decltype(value_detector(std::declval<Container>())); };
} // namespace container_traits_helper
} // namespace detail

// Container traits
template <class Container>
struct container_traits
{
    using iterator_type = typename boost::asynchronous::detail::container_traits_helper::iterator_type_detector<Container>::type;
    using size_type     = typename boost::asynchronous::detail::container_traits_helper::size_type_detector<Container>::type;
    using value_type    = typename boost::asynchronous::detail::container_traits_helper::value_type_detector<Container>::type;
};

// Size of the container
template <class Container>
typename boost::asynchronous::container_traits<Container>::size_type container_size(const Container& c)
{
#if __cpp_lib_nonmember_container_access
    return std::size(c);
#else
    return std::distance(std::begin(c), std::end(c));
#endif
}

} // namespace asynchronous
} // namespace boost

#endif // BOOST_ASYNCHRONOUS_CONTAINER_TRAITS_HPP
