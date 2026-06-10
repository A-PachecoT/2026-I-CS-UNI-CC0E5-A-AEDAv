#ifndef __HEAP_H__
#define __HEAP_H__

#include <iostream>
#include <cstddef>
#include <string>
#include <sstream>
#include <stdexcept>
#include <mutex>
#include <shared_mutex>
#include <utility>
#include <tuple>
#include "vector.h"
#include "util.h"
#include "../types.h"
using namespace std;

template <typename T>
struct MinHeapTrait {
    using value_type = T;
    using Comp       = std::less<T>;
};

template <typename T>
struct MaxHeapTrait {
    using value_type = T;
    using Comp       = std::greater<T>;
};

template <typename Trait>
class Heap {
public:
    using value_type = typename Trait::value_type;
    using Comp       = typename Trait::Comp;
    using MySelf     = Heap<Trait>;
    using size_type  = std::size_t;

private:
    Vector<Trait>         m_vec;
    Comp                  m_comp;
    mutable shared_mutex  m_mtx;

    static size_type parentIdx(size_type i){ return (i - 1) / 2; }
    static size_type leftIdx  (size_type i){ return 2*i + 1; }
    static size_type rightIdx (size_type i){ return 2*i + 2; }

    void heapifyUp_unsafe(size_type i) {
        while(i > 0) {
            const size_type p = parentIdx(i);
            if(m_comp(m_vec.node_at_unsafe(i).getData(),
                      m_vec.node_at_unsafe(p).getData())) {
                m_vec.swap_unsafe(i, p);
                i = p;
            } else break;
        }
    }

    void heapifyDown_unsafe(size_type i) {
        const size_type n = m_vec.m_size;
        while(true) {
            const size_type l = leftIdx(i);
            const size_type r = rightIdx(i);
            size_type best = i;
            if(l < n && m_comp(m_vec.node_at_unsafe(l).getData(),
                               m_vec.node_at_unsafe(best).getData())) best = l;
            if(r < n && m_comp(m_vec.node_at_unsafe(r).getData(),
                               m_vec.node_at_unsafe(best).getData())) best = r;
            if(best == i) break;
            m_vec.swap_unsafe(i, best);
            i = best;
        }
    }

public:
    Heap() = default;

    Heap(const Heap& other) {
        shared_lock<shared_mutex> lock(other.m_mtx);
        m_vec = other.m_vec;
    }

    Heap(Heap&& other) noexcept {
        unique_lock<shared_mutex> lock(other.m_mtx);
        m_vec = std::move(other.m_vec);
    }

    Heap& operator=(const Heap& other) {
        if(this == &other) return *this;
        if(this < &other) {
            unique_lock<shared_mutex> lockThis(m_mtx);
            shared_lock<shared_mutex> lockOther(other.m_mtx);
            m_vec = other.m_vec;
        } else {
            shared_lock<shared_mutex> lockOther(other.m_mtx);
            unique_lock<shared_mutex> lockThis(m_mtx);
            m_vec = other.m_vec;
        }
        return *this;
    }

    Heap& operator=(Heap&& other) noexcept {
        if(this == &other) return *this;
        unique_lock<shared_mutex> lockThis(m_mtx);
        unique_lock<shared_mutex> lockOther(other.m_mtx);
        m_vec = std::move(other.m_vec);
        return *this;
    }

    ~Heap() = default;

    void insert(value_type value, Ref ref) {
        unique_lock<shared_mutex> lock(m_mtx);
        m_vec.push_back_unsafe(value, ref);
        heapifyUp_unsafe(m_vec.m_size - 1);
    }

    std::tuple<value_type, Ref> extract() {
        unique_lock<shared_mutex> lock(m_mtx);
        if(m_vec.m_size == 0)
            throw std::out_of_range("Heap::extract — heap vacio");
        auto top = std::make_tuple(m_vec.node_at_unsafe(0).getData(),
                                   m_vec.node_at_unsafe(0).getRef());
        m_vec.swap_unsafe(0, m_vec.m_size - 1);
        m_vec.pop_back_unsafe();
        if(m_vec.m_size > 0) heapifyDown_unsafe(0);
        return top;
    }

    std::tuple<value_type, Ref> peek() const {
        shared_lock<shared_mutex> lock(m_mtx);
        if(m_vec.m_size == 0)
            throw std::out_of_range("Heap::peek — heap vacio");
        return std::make_tuple(m_vec.node_at_unsafe(0).getData(),
                               m_vec.node_at_unsafe(0).getRef());
    }

    size_type size()    const { shared_lock<shared_mutex> lock(m_mtx); return m_vec.m_size; }
    bool   empty()   const { shared_lock<shared_mutex> lock(m_mtx); return m_vec.m_size == 0; }
    bool   isEmpty() const { return empty(); }

    using forward_iterator  = typename Vector<Trait>::forward_iterator;
    using backward_iterator = typename Vector<Trait>::backward_iterator;
    forward_iterator  begin()  { return m_vec.begin(); }
    forward_iterator  end()    { return m_vec.end(); }
    backward_iterator rbegin() { return m_vec.rbegin(); }
    backward_iterator rend()   { return m_vec.rend(); }

    string toString() const {
        shared_lock<shared_mutex> lock(m_mtx);
        ostringstream oss;
        oss << "[";
        for(size_type i = 0; i < m_vec.m_size; ++i) {
            if(i > 0) oss << ",";
            const auto& n = m_vec.node_at_unsafe(i);
            oss << "(" << n.getData() << "," << n.getRef() << ")";
        }
        oss << "]";
        return oss.str();
    }

    friend ostream& operator<<(ostream& os, const Heap& h) {
        return os << h.toString();
    }

    friend istream& operator>>(istream& is, Heap& h) {
        char ch;
        if(!(is >> ch) || ch != '[') {
            is.clear(ios_base::failbit);
            return is;
        }
        value_type val;
        Ref ref;
        char comma, paren;
        while(is >> ch && ch != ']') {
            if(ch == '(') {
                if(is >> val >> comma >> ref >> paren) {
                    if(comma == ',' && paren == ')') {
                        h.insert(val, ref);   // toma su propio lock
                    }
                }
            }
        }
        return is;
    }
};

#endif // __HEAP_H__
