//  Copyright (C) Tobias Holl 2014
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef LAZY_RANGE_HPP
#define LAZY_RANGE_HPP

#include <cstdint>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <type_traits>

namespace detail {

// An identity transform, default for lazy_range
class identity {
public:
	int operator()(long i) { return ((int) i); }
};

// An iterator for lazy_range fulfilling the requirements of
// RandomAccessIterator
// The Thing template is necessary to avoid dependency mess
// (lazy_range would need the iterator, which in turn would need
// lazy_range, which leads to compile errors)
template <class Thing, class ValueType=int>
class thing_iterator {
private:
    Thing* lr;
    long pos;

public:
	typedef ValueType value_type;
	typedef long difference_type;
	typedef value_type* pointer;
	typedef value_type& reference;
	typedef std::random_access_iterator_tag iterator_category;

	thing_iterator(Thing* r, long position) : lr(r), pos(position) {}
	thing_iterator(thing_iterator<Thing, ValueType> const& other) : lr(other.lr), pos(other.pos) {}
	~thing_iterator() {}
	
	thing_iterator<Thing, ValueType>& operator+=(long o) { // +=
		pos += o;
		return *this;
	}
	thing_iterator<Thing, ValueType>& operator++() { // ++ (prefix)
		++pos;
		return *this;
	}
	thing_iterator<Thing, ValueType> operator++(int) { // ++ (postfix)
		thing_iterator<Thing, ValueType> copy(*this);
		++pos;
		return copy;
	}
	thing_iterator<Thing, ValueType> operator+(long o) const { // + (binary)
		return thing_iterator<Thing, ValueType>(lr, pos + o);
	}
	
	
	thing_iterator<Thing, ValueType>& operator-=(long o) { // -=
		pos -= o;
		return *this;
	}
	thing_iterator<Thing, ValueType>& operator--() { // -- (prefix)
		--pos;
		return *this;
	}
	thing_iterator<Thing, ValueType> operator--(int) { // -- (postfix)
		thing_iterator<Thing, ValueType> copy(*this);
		--pos;
		return copy;
	}
	thing_iterator<Thing, ValueType> operator-(long o) const { // - (binary, with number)
		return thing_iterator<Thing, ValueType>(lr, pos - o);
	}
	
	template <class Q, class R>
	long operator-(thing_iterator<Q, R> const& o) const { // - (binary, with lri)
		return pos - o.pos;
	}
	
	template <class Q, class R>
	bool operator==(thing_iterator<Q, R> const& it) const { // ==
		return (pos == it.pos);
	}
	
	template <class Q, class R>
	bool operator!=(thing_iterator<Q, R> const& it) const { // !=
		return (pos != it.pos);
	}
	
	ValueType operator*() const { // * (unary / dereference)
		return lr->at(pos);
	}
	
	std::shared_ptr<ValueType> operator->() const { // ->
		return lr->_ptr_at(pos);
	}
	
	ValueType operator[](long p) const { // []
		return lr->at(pos + p);
	}
	
	template <class Q, class R>
	bool operator<(thing_iterator<Q, R> const& o) const { // <
		return (pos < o.pos);
	}
	template <class Q, class R>
	bool operator<=(thing_iterator<Q, R> const& o) const { // <=
		return (pos <= o.pos);
	}
	template <class Q, class R>
	bool operator>(thing_iterator<Q, R> const& o) const { // >
		return (pos > o.pos);
	}
	template <class Q, class R>
	bool operator>=(thing_iterator<Q, R> const& o) const { // >=
		return (pos >= o.pos);
	}
	
	inline long position() {
		return pos;
	}
};

}


// A "lazy" range, evaluating the value at position i when it is asked for,
// by applying the given transformation to i. Default transformation is
// identity, simply returning the value of i.
template <typename Func>
class lazy_range {
private:
	Func transform;
	long size_;
public:
	typedef decltype(std::declval<Func>()(0)) value_type;
    typedef detail::thing_iterator<lazy_range<Func>, value_type> const_iterator_type;
	typedef detail::thing_iterator<lazy_range<Func>, value_type> iterator_type;
    typedef const_iterator_type const_iterator;
    typedef iterator_type iterator;

	lazy_range(long size, Func const& tf) : transform(tf), size_(size) {}
	
	value_type at(long index) {
		if (index < 0 || index >= size_)
			throw std::logic_error("lazy_range out of bounds");
		return transform(index);
	}
	
	void setsize(long newsize) {
		size_ = newsize;
	}
	
	std::shared_ptr<value_type> _ptr_at(long index) {
		return std::make_shared<value_type>(transform(index));
	}
	
	long size() {
		return size_;
	}
	
	iterator_type begin() {
		return iterator_type(this, 0);
	}
	
	iterator_type end() {
		return iterator_type(this, size_);
	}
	
	inline value_type operator[](long index) {
		return at(index);
	}
};


#endif
