
#pragma once

#include <iterator>

template <typename T>
struct rangeBase : std::iterator<std::input_iterator_tag, T> {
	
    rangeBase(T value) : value(value) { }

    T operator *() const { return value; }

    T const* operator ->() const { return &value; }

    rangeBase& operator ++() {
        ++value;
        return *this;
    }

    rangeBase operator ++(int) {
        auto copy = *this;
        ++*this;
        return copy;
    }

    bool operator == (rangeBase const& other) const {
        return value == other.value;
    }

    bool operator != (rangeBase const& other) const {
        return not (*this == other);
    }

protected:
    T value;
};


template <typename T>
struct rangeProxy {
    struct iter : rangeBase<T> {
        iter(T value) : rangeBase<T>(value) { }
    };
	
    rangeProxy(T begin, T end) : _begin(begin), _end(end) { }

    iter begin() const { return this->_begin; }

    iter end() const { return this->_end; }

private:
    iter _begin;
    iter _end;
};

template <typename T>
rangeProxy<T> range(T begin, T end) {
    return {begin, end};
}

template <typename T>
rangeProxy<T> range(T end) {
    return {0, end};
}
