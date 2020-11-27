#ifndef ITERATOR_H
#define ITERATOR_H

#include <optional>
#include <vector>

/// An abstraction over container-classes/iterators missing in C++.
template<typename T>
struct input_iterator
{
    using wrapped_value = std::optional<std::reference_wrapper<T>>;
    /// Returns the next value or std::nullopt if the end was reached.
    virtual wrapped_value next() = 0;
};

/// An implementation of input_iterator for std::vector and std::list (and maybe even other std:: containers?).
template<typename T,
         template<typename>
         class Container,
         typename ContainerIterator = typename Container<T>::iterator>
struct container_input_iterator : input_iterator<T>
{
    container_input_iterator(Container<T>& c);
    /// The current iterator.
    ContainerIterator current_position;
    /// The end iterator.
    ContainerIterator end_position;
    typename input_iterator<T>::wrapped_value next() override;
};

template<typename T,
         template<typename>
         class Container,
         typename ContainerIterator>
container_input_iterator<T, Container, ContainerIterator>::
  container_input_iterator(Container<T>& c)
  : current_position(c.begin())
  , end_position(c.end())
{}

template<typename T,
         template<typename>
         class Container,
         typename ContainerIterator>
typename input_iterator<T>::wrapped_value
container_input_iterator<T, Container, ContainerIterator>::next()
{
    if (current_position != end_position) {
        return *current_position++;
    } else {
        return {};
    }
}

#endif
