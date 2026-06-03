#ifndef __VECTOR_H__
#define __VECTOR_H__

#include <iostream>
#include <cstddef> // size_t
#include <string>
#include <sstream>
#include <shared_mutex> // shared_mutex
#include <mutex>
#include <tuple>
#include <utility>     // std::exchange, std::swap
#include <stdexcept>   // std::out_of_range
#include "general_iterator.h"
#include "util.h"
#include "../types.h"
using namespace std;

template <typename Container>
class vector_forward_iterator : public general_iterator<Container, vector_forward_iterator<Container>> {
public:
    using MySelf = vector_forward_iterator<Container>;
    using Parent = general_iterator<Container, MySelf>;
    using Parent::Parent;
    MySelf operator++() { this->m_pNode++; return *this; }
};

template <typename Container>
class vector_backward_iterator : public general_iterator<Container, vector_backward_iterator<Container>> {
public:
    using MySelf = vector_backward_iterator<Container>;
    using Parent = general_iterator<Container, MySelf>;
    using Parent::Parent;
    MySelf operator++() { this->m_pNode--; return *this; }
};

template <typename T>
class VectorNode{
    T   m_data;
    Ref m_ref;
public:
    VectorNode() : m_data(T()), m_ref(Ref()) {}
    VectorNode(T data, Ref ref) : m_data(data), m_ref(ref) {}
    VectorNode(const VectorNode &other) : m_data(other.m_data), m_ref(other.m_ref) {}
    VectorNode(VectorNode &&other) : m_data(std::move(other.m_data)), m_ref(std::move(other.m_ref)) {}
    VectorNode& operator=(const VectorNode &other) {
        m_data = other.m_data;
        m_ref = other.m_ref;
        return *this;
    }
    VectorNode& operator=(VectorNode &&other) {
        m_data = std::move(other.m_data);
        m_ref = std::move(other.m_ref);
        return *this;
    }

    T          getData() const { return m_data; }
    T&         getDataRef()       { return m_data; }
    const T&   getDataRef() const { return m_data; }
    void       setData(T data) { m_data = data; }
    Ref        getRef() const { return m_ref; }
    void       setRef(Ref ref) { m_ref = ref; }
    
};

template <typename T>
ostream& operator<<(ostream& os, VectorNode<T>& node){
    return os << "(" << node.getData() << ", " << node.getRef() << ")";
}

// Forward decl para que Vector pueda declarar Heap como friend
template <typename Trait> class Heap;

template <typename T>
class Vector{
public:
    using  value_type = T;
    using  forward_iterator   = vector_forward_iterator < Vector<T> > ;
    friend forward_iterator;
    using  backward_iterator  = vector_backward_iterator< Vector<T> > ;
    friend backward_iterator;
    using  Node               = VectorNode<T>;

    // Heap necesita usar las variantes _unsafe sin retomar el lock interno
    // (su propio mutex ya bloquea durante heapifyUp / heapifyDown).
    template <typename Trait> friend class Heap;

protected:
    size_t  m_capacity;
    size_t  m_size;
    Node   *m_data;
    mutable shared_mutex m_mtx;
    void    resize_unsafe();

    // Variantes sin lock: contrato = lock externo ya tomado por el caller.
    void                push_back_unsafe(value_type value, Ref ref);
    std::tuple<value_type, Ref> pop_back_unsafe();
    void                swap_unsafe(size_t i, size_t j);
    Node&               node_at_unsafe(size_t i) { return m_data[i]; }
    const Node&         node_at_unsafe(size_t i) const { return m_data[i]; }

public:
    Vector(size_t capacity = 10);
    Vector(const Vector& other);
    Vector(Vector&& other) noexcept;
    Vector& operator=(const Vector& other);
    Vector& operator=(Vector&& other) noexcept;
    virtual ~Vector();

    virtual void push_back(value_type value, Ref ref);
    virtual std::tuple<value_type, Ref> pop_back();
    virtual size_t size() const;
    virtual bool   empty() const;
    virtual string toString() const;

    // Acceso por índice. Tira out_of_range — NUNCA devuelve basura.
    value_type&       operator[](size_t i);
    const value_type& operator[](size_t i) const;

    forward_iterator begin() { return forward_iterator(this, m_data); }
    forward_iterator end()   { return forward_iterator(this, m_data + m_size); }

    backward_iterator rbegin() { return backward_iterator(this, m_data + m_size - 1); }
    backward_iterator rend()   { return backward_iterator(this, m_data - 1); }

    // Done: Agregar control concurrente
    template <typename Func, typename... Args>
    void ForEach(Func func, Args &&...  args){
        unique_lock<shared_mutex> lock(m_mtx);
        ::ForEach(begin(), end(), func, std::forward<Args>(args)... );
    }

    // Done: Agregar control concurrente
    template <typename Func, typename... Args>
    void ReverseForEach(Func func, Args &&...  args){
        unique_lock<shared_mutex> lock(m_mtx);
        if(m_size == 0)
            return;
        ::ForEach(rbegin(), rend(), func, std::forward<Args>(args)... );
    }
};

template <typename T>
Vector<T>::Vector(size_t capacity){
    m_capacity = capacity;
    m_size = 0;
    m_data = new Node[capacity];
}

template <typename T>
Vector<T>::Vector(const Vector& other) : m_capacity(0), m_size(0), m_data(nullptr) {
    shared_lock<shared_mutex> lock(other.m_mtx);
    m_capacity = other.m_capacity;
    m_size     = other.m_size;
    m_data     = new Node[m_capacity];
    for(size_t i = 0; i < m_size; ++i)
        m_data[i] = other.m_data[i];
}

template <typename T>
Vector<T>::Vector(Vector&& other) noexcept : m_capacity(0), m_size(0), m_data(nullptr) {
    unique_lock<shared_mutex> lock(other.m_mtx);
    m_capacity = std::exchange(other.m_capacity, 0);
    m_size     = std::exchange(other.m_size, 0);
    m_data     = std::exchange(other.m_data, nullptr);
}

template <typename T>
Vector<T>& Vector<T>::operator=(const Vector& other){
    if(this == &other) return *this;
    // Adquirir ambos locks. Orden por dirección de memoria para evitar
    // deadlock con asignaciones cruzadas simultáneas.
    if(this < &other){
        unique_lock<shared_mutex> lockThis(m_mtx);
        shared_lock<shared_mutex> lockOther(other.m_mtx);
        delete [] m_data;
        m_capacity = other.m_capacity;
        m_size     = other.m_size;
        m_data     = new Node[m_capacity];
        for(size_t i = 0; i < m_size; ++i) m_data[i] = other.m_data[i];
    } else {
        shared_lock<shared_mutex> lockOther(other.m_mtx);
        unique_lock<shared_mutex> lockThis(m_mtx);
        delete [] m_data;
        m_capacity = other.m_capacity;
        m_size     = other.m_size;
        m_data     = new Node[m_capacity];
        for(size_t i = 0; i < m_size; ++i) m_data[i] = other.m_data[i];
    }
    return *this;
}

template <typename T>
Vector<T>& Vector<T>::operator=(Vector&& other) noexcept {
    if(this == &other) return *this;
    unique_lock<shared_mutex> lockThis(m_mtx);
    unique_lock<shared_mutex> lockOther(other.m_mtx);
    delete [] m_data;
    m_capacity = std::exchange(other.m_capacity, 0);
    m_size     = std::exchange(other.m_size, 0);
    m_data     = std::exchange(other.m_data, nullptr);
    return *this;
}

template <typename T>
Vector<T>::~Vector(){
    delete [] m_data;
}

// resize_unsafe: asume lock externo. NO retomar.
template <typename T>
void Vector<T>::resize_unsafe(){
    m_capacity = (m_capacity < 10) ? m_capacity+10 : m_capacity * 2;
    Node * new_data = new Node[m_capacity];
    for(size_t i = 0; i < m_size; ++i)
        new_data[i] = std::move(m_data[i]);
    delete [] m_data;
    m_data = new_data;
}

template <typename T>
void Vector<T>::push_back_unsafe(value_type value, Ref ref){
    if(m_size == m_capacity)
        resize_unsafe();
    m_data[m_size++] = Node(value, ref);
}

template <typename T>
void Vector<T>::push_back(value_type value, Ref ref){
    unique_lock<shared_mutex> lock(m_mtx);
    push_back_unsafe(value, ref);
}

template <typename T>
std::tuple<typename Vector<T>::value_type, Ref> Vector<T>::pop_back_unsafe(){
    if(m_size == 0)
        throw std::out_of_range("Vector::pop_back en Vector vacio");
    --m_size;
    return std::make_tuple(m_data[m_size].getData(), m_data[m_size].getRef());
}

template <typename T>
std::tuple<typename Vector<T>::value_type, Ref> Vector<T>::pop_back(){
    unique_lock<shared_mutex> lock(m_mtx);
    return pop_back_unsafe();
}

template <typename T>
void Vector<T>::swap_unsafe(size_t i, size_t j){
    if(i == j) return;
    std::swap(m_data[i], m_data[j]);
}

template <typename T>
typename Vector<T>::value_type& Vector<T>::operator[](size_t i){
    shared_lock<shared_mutex> lock(m_mtx);
    if(i >= m_size)
        throw std::out_of_range("Vector::operator[]: indice fuera de rango");
    return m_data[i].getDataRef();
}

template <typename T>
const typename Vector<T>::value_type& Vector<T>::operator[](size_t i) const {
    shared_lock<shared_mutex> lock(m_mtx);
    if(i >= m_size)
        throw std::out_of_range("Vector::operator[] const: indice fuera de rango");
    return m_data[i].getDataRef();
}

template <typename T>
size_t Vector<T>::size() const{
    shared_lock<shared_mutex> lock(m_mtx);
    return m_size;
}

template <typename T>
bool Vector<T>::empty() const {
    shared_lock<shared_mutex> lock(m_mtx);
    return m_size == 0;
}

template <typename T>
string Vector<T>::toString() const{
    shared_lock<shared_mutex> lock(m_mtx);
    ostringstream oss;
    oss << "[";
    for(size_t i = 0; i < m_size; ++i){
        if(i > 0)
            oss << ",";
        oss << m_data[i];
    }
    oss << "]";
    return oss.str();
}

template <typename T>
ostream& operator<<(ostream& os, const Vector<T>& v){
    return os << v.toString();
}

// TODO: Implementar
template <typename T>
istream& operator>>(istream& is, Vector<T>& v){
    return is;
}

// template <typename T>
// template <typename Func, typename... Args>
// void Vector<T>::ForEach(Func func, Args &&...  args){
//     ::ForEach(begin(), end(), func, std::forward<Args>(args)... );
// }

void DemoVector();
void DemoConcurrentVector();

#endif // __VECTOR_H__