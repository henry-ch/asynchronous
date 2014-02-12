#ifndef LAZY_IRANGE_HPP
#define LAZY_IRANGE_HPP

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <memory>
#include <stdexcept>


namespace boost { namespace asynchronous {

namespace detail {

// An iterator for lazy_irange fulfilling the requirements of
// RandomAccessIterator
// The Thing template is necessary to avoid dependency mess
// (lazy_irange would need the iterator, which in turn would need
// lazy_irange, which leads to compile errors)
template <class Thing, class ValueType, class NumberType>
class thing_iterator {
private:
    Thing* lr;
    NumberType pos;

public:
	typedef ValueType value_type;
	typedef long difference_type;
	typedef value_type* pointer;
	typedef value_type& reference;
	typedef std::random_access_iterator_tag iterator_category;

	thing_iterator(Thing* r, NumberType position) : lr(r), pos(position) {}
	thing_iterator(thing_iterator<Thing, ValueType, NumberType> const& other) : lr(other.lr), pos(other.pos) {}
	~thing_iterator() {}
	
	thing_iterator<Thing, ValueType, NumberType>& operator+=(NumberType o) { // +=
		pos += o;
		return *this;
	}
	thing_iterator<Thing, ValueType, NumberType>& operator++() { // ++ (prefix)
		++pos;
		return *this;
	}
	thing_iterator<Thing, ValueType, NumberType> operator++(int) { // ++ (postfix)
		thing_iterator<Thing, ValueType, NumberType> copy(*this);
		++pos;
		return copy;
	}
	thing_iterator<Thing, ValueType, NumberType> operator+(NumberType o) const { // + (binary)
		return thing_iterator<Thing, ValueType, NumberType>(lr, pos + o);
	}
	
	
	thing_iterator<Thing, ValueType, NumberType>& operator-=(long o) { // -=
		pos -= o;
		return *this;
	}
	thing_iterator<Thing, ValueType, NumberType>& operator--() { // -- (prefix)
		--pos;
		return *this;
	}
	thing_iterator<Thing, ValueType, NumberType> operator--(int) { // -- (postfix)
		thing_iterator<Thing, ValueType, NumberType> copy(*this);
		--pos;
		return copy;
	}
	thing_iterator<Thing, ValueType, NumberType> operator-(NumberType o) const { // - (binary, with number)
		return thing_iterator<Thing, ValueType, NumberType>(lr, pos - o);
	}
	
	template <class Q, class R, class S>
	long operator-(thing_iterator<Q, R, S> const& o) const { // - (binary, with lri)
		return pos - o.pos;
	}
	
	template <class Q, class R, class S>
	bool operator==(thing_iterator<Q, R, S> const& it) const { // ==
		return (pos == it.pos);
	}
	
	template <class Q, class R, class S>
	bool operator!=(thing_iterator<Q, R, S> const& it) const { // !=
		return (pos != it.pos);
	}
	
	ValueType operator*() const { // * (unary / dereference)
		return lr->at(pos);
	}
	
	std::shared_ptr<ValueType> operator->() const { // ->
		return lr->_ptr_at(pos);
	}
	
	ValueType operator[](NumberType p) const { // []
		return lr->at(pos + p);
	}
	
	template <class Q, class R, class S>
	bool operator<(thing_iterator<Q, R, S> const& o) const { // <
		return (pos < o.pos);
	}
	template <class Q, class R, class S>
	bool operator<=(thing_iterator<Q, R, S> const& o) const { // <=
		return (pos <= o.pos);
	}
	template <class Q, class R, class S>
	bool operator>(thing_iterator<Q, R, S> const& o) const { // >
		return (pos > o.pos);
	}
	template <class Q, class R, class S>
	bool operator>=(thing_iterator<Q, R, S> const& o) const { // >=
		return (pos >= o.pos);
	}
	
	inline NumberType position() {
		return pos;
	}
};


// A "lazy" range, evaluating the value at position i when it is asked for,
// by applying the given transformation to i.
template <typename Func, typename NumberType>
class lazy_irange {
private:
	Func transform;
	NumberType start_;
	NumberType end_;
public:
	typedef decltype(std::declval<Func>()(std::declval<NumberType>())) value_type;
    typedef detail::thing_iterator<lazy_irange<Func, NumberType>, value_type, NumberType> iterator_type;
    typedef iterator_type const_iterator;
    typedef iterator_type iterator;

    lazy_irange(NumberType start, NumberType end, Func const& tf) : transform(tf), start_(start), end_(end) {}
	
	value_type at(NumberType index) {
		if (index < start_ || index >= end_)
            throw std::logic_error("lazy_irange out of bounds");
		return transform(index);
	}
	
	void setstart(NumberType start) {
		start_ = start;
	}
	void setend(NumberType end) {
		end_ = end;
	}
	
	std::shared_ptr<value_type> _ptr_at(NumberType index) {
		return std::make_shared<value_type>(transform(index));
	}
	
	NumberType size() {
		return end_ - start_;
	}
	
	iterator_type begin() {
		return iterator_type(this, start_);
	}
	
	iterator_type end() {
		return iterator_type(this, end_);
	}
	
	inline value_type operator[](NumberType index) {
		return at(index);
	}
};

}

template <typename F, typename N>
detail::lazy_irange<F, N> lazy_irange(N start, N end, F func) {
    return detail::lazy_irange<F, N>(start, end, std::move(func));
}

}}

#endif
