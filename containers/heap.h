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

// =====================================================================
// Traits del Heap.
//
// NOTA: NO heredan de BaseTrait porque BaseTrait<Node, Comp> requiere
// un tipo de nodo. El Heap no tiene nodo propio — usa VectorNode<T> via
// el Vector<T> interno. Simplemente expone value_type y Comp.
// =====================================================================
template <typename T>
struct MinHeapTrait {
    using value_type = T;
    using Comp       = std::less<T>;   // parent < child -> raiz es minimo
};

template <typename T>
struct MaxHeapTrait {
    using value_type = T;
    using Comp       = std::greater<T>; // parent > child -> raiz es maximo
};

// =====================================================================
// Heap<Trait>
//
// Composicion sobre Vector<value_type>. NO hereda; el Vector vive como
// campo. Heap es friend de Vector (declarado en vector.h A3) -> puede
// usar Vector::_unsafe sin retomar el lock del Vector mientras tiene
// el suyo propio.
//
// Indices del heap clasico (0-indexed):
//   parent(i) = (i-1)/2     left(i) = 2i+1     right(i) = 2i+2
// =====================================================================
template <typename Trait>
class Heap {
public:
    using value_type = typename Trait::value_type;
    using Comp       = typename Trait::Comp;
    using MySelf     = Heap<Trait>;

private:
    Vector<value_type>    m_vec;
    Comp                  m_comp;
    mutable shared_mutex  m_mtx;

    // ---- helpers _unsafe (asumen m_mtx ya tomado) ----

    static size_t parentIdx(size_t i){ return (i - 1) / 2; }
    static size_t leftIdx  (size_t i){ return 2*i + 1; }
    static size_t rightIdx (size_t i){ return 2*i + 2; }

    void heapifyUp_unsafe(size_t i) {
        while(i > 0) {
            const size_t p = parentIdx(i);
            // Si el child rompe el orden (debe estar antes que parent), swap.
            if(m_comp(m_vec.node_at_unsafe(i).getData(),
                      m_vec.node_at_unsafe(p).getData())) {
                m_vec.swap_unsafe(i, p);
                i = p;
            } else break;
        }
    }

    void heapifyDown_unsafe(size_t i) {
        const size_t n = m_vec.m_size;
        while(true) {
            const size_t l = leftIdx(i);
            const size_t r = rightIdx(i);
            size_t best = i;
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
        m_vec = other.m_vec;  // Vector copy assign (toma su propio lock)
    }

    Heap(Heap&& other) noexcept {
        unique_lock<shared_mutex> lock(other.m_mtx);
        m_vec = std::move(other.m_vec);
    }

    Heap& operator=(const Heap& other) {
        if(this == &other) return *this;
        // Orden de locks por direccion (mismo truco que vector.h A3 / BinaryTree.h B2)
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

    ~Heap() = default;  // Vector se destruye solo

    // ---- API publica ----

    void insert(value_type value, Ref ref) {
        unique_lock<shared_mutex> lock(m_mtx);
        m_vec.push_back_unsafe(value, ref);
        heapifyUp_unsafe(m_vec.m_size - 1);
    }

    // extract: devuelve el top (data, ref) y lo remueve.
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

    // peek: top sin remover.
    std::tuple<value_type, Ref> peek() const {
        shared_lock<shared_mutex> lock(m_mtx);
        if(m_vec.m_size == 0)
            throw std::out_of_range("Heap::peek — heap vacio");
        return std::make_tuple(m_vec.node_at_unsafe(0).getData(),
                               m_vec.node_at_unsafe(0).getRef());
    }

    size_t size()    const { shared_lock<shared_mutex> lock(m_mtx); return m_vec.m_size; }
    bool   empty()   const { shared_lock<shared_mutex> lock(m_mtx); return m_vec.m_size == 0; }
    bool   isEmpty() const { return empty(); }

    // Iteradores del Vector subyacente — habilitan range-for nativo
    // `for (auto& v : heap)`. Recorrido lineal del array (orden interno
    // del heap, NO ordenado por prioridad — para eso usar extract).
    using forward_iterator  = typename Vector<value_type>::forward_iterator;
    using backward_iterator = typename Vector<value_type>::backward_iterator;
    forward_iterator  begin()  { return m_vec.begin(); }
    forward_iterator  end()    { return m_vec.end(); }
    backward_iterator rbegin() { return m_vec.rbegin(); }
    backward_iterator rend()   { return m_vec.rend(); }

    string toString() const {
        shared_lock<shared_mutex> lock(m_mtx);
        ostringstream oss;
        oss << "[";
        for(size_t i = 0; i < m_vec.m_size; ++i) {
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

    // operator>> reconstruye el heap insertando cada (val, ref).
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
