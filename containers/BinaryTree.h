#ifndef __BINARYTREE_H__
#define __BINARYTREE_H__
#include <iostream>
#include <cstddef>
#include <string>
#include <sstream>
#include <fstream>
#include <thread>
#include <mutex>
#include <shared_mutex>
#include <utility>
#include <type_traits>
#include "general_iterator.h"
#include "util.h"
#include "traits.h"
#include "../types.h"
using namespace std;

// BinaryTreeNode<T, NodeType=void>
//   - NodeType=void : nodo simple (default). m_pChild apunta a BinaryTreeNode<T,void>.
//   - NodeType=AVLNode<T> u otra subclase : m_pChild apunta al tipo derivado
//     sin necesidad de casteo.
template <typename T, typename NodeType = void>
class BinaryTreeNode {
public:
    using value_type = T;
    using Node = typename std::conditional<std::is_void<NodeType>::value,
                                           BinaryTreeNode<T, void>,
                                           NodeType>::type;
protected:
    T     m_data;
    Ref   m_ref;
    Node *m_pChild[2];   // 0 = left, 1 = right
    Node *m_pParent;

public:
    BinaryTreeNode(T data = T(), Ref ref = Ref(), Node *parent = nullptr)
        : m_data(data), m_ref(ref), m_pChild{nullptr, nullptr}, m_pParent(parent) {}
    virtual ~BinaryTreeNode() {}

    T          getData() const     { return m_data; }
    T&         getDataRef()        { return m_data; }
    const T&   getDataRef() const  { return m_data; }
    void       setData(T data)     { m_data = data; }

    Ref        getRef() const      { return m_ref; }
    void       setRef(Ref ref)     { m_ref = ref; }

    Node*      getChild(int b) const   { return m_pChild[b]; }
    Node*&     getChildRef(int b)      { return m_pChild[b]; }
    void       setChild(int b, Node* c){ m_pChild[b] = c; }

    Node*      getParent() const       { return m_pParent; }
    void       setParent(Node* p)      { m_pParent = p; }

    // Para que general_iterator (que llama getNext) pueda usarse — no aplica
    // a iteradores de árbol, los iteradores propios manejan sucesor/predecesor.
};

// Traits para BinaryTree. Reciben el TIPO DEL NODO concreto.
template <typename T>
struct AscendingBTTrait : public BaseTrait<BinaryTreeNode<T>, std::less<T>> {};

template <typename T>
struct DescendingBTTrait : public BaseTrait<BinaryTreeNode<T>, std::greater<T>> {};

// Forward decl del contenedor para los iteradores
template <typename Trait> class BinaryTree;

// =============================================================
// Iteradores inorder (forward / backward) usando m_pParent
// =============================================================
//
// Estrategia: el iterador guarda el nodo actual. operator++ calcula
// el sucesor inorder usando m_pParent (no necesitamos pila ni vector).
//
// Sucesor inorder de N:
//   - Si N tiene hijo derecho: leftmost(right(N))
//   - Si no: subir por padres hasta que veamos un padre desde su
//     hijo izquierdo. Ese padre es el sucesor. Si no hay -> end().

template <typename Container>
class bt_inorder_forward_iterator
    : public general_iterator<Container, bt_inorder_forward_iterator<Container>>
{
public:
    using Parent = general_iterator<Container, bt_inorder_forward_iterator<Container>>;
    using Node   = typename Container::Node;
    using Parent::Parent;
    using typename Parent::MySelf;

    MySelf operator++() {
        Node* n = this->m_pNode;
        if(!n) return *this;
        if(n->getChild(1)){
            // Tiene hijo derecho -> leftmost del subarbol derecho
            n = n->getChild(1);
            while(n->getChild(0)) n = n->getChild(0);
            this->m_pNode = n;
        } else {
            // Subir hasta encontrar un padre del cual venimos por izquierda
            Node* p = n->getParent();
            while(p && p->getChild(1) == n){
                n = p;
                p = p->getParent();
            }
            this->m_pNode = p;  // puede ser nullptr = end()
        }
        return *this;
    }
};

template <typename Container>
class bt_inorder_backward_iterator
    : public general_iterator<Container, bt_inorder_backward_iterator<Container>>
{
public:
    using Parent = general_iterator<Container, bt_inorder_backward_iterator<Container>>;
    using Node   = typename Container::Node;
    using Parent::Parent;
    using typename Parent::MySelf;

    // ++ = predecesor inorder (espejado del forward)
    MySelf operator++() {
        Node* n = this->m_pNode;
        if(!n) return *this;
        if(n->getChild(0)){
            n = n->getChild(0);
            while(n->getChild(1)) n = n->getChild(1);
            this->m_pNode = n;
        } else {
            Node* p = n->getParent();
            while(p && p->getChild(0) == n){
                n = p;
                p = p->getParent();
            }
            this->m_pNode = p;
        }
        return *this;
    }
};

// =============================================================
// BinaryTree<Trait>
// =============================================================
template <typename Trait>
class BinaryTree {
public:
    using value_type = typename Trait::value_type;
    using Node       = typename Trait::Node;
    using Comp       = typename Trait::Comp;
    using MySelf     = BinaryTree<Trait>;
    using forward_iterator  = bt_inorder_forward_iterator<MySelf>;
    using backward_iterator = bt_inorder_backward_iterator<MySelf>;
    friend forward_iterator;
    friend backward_iterator;

protected:
    Node *m_pRoot = nullptr;
    size_t m_size = 0;
    Comp   m_comp;
    mutable shared_mutex m_mtx;

    // ---- helpers _unsafe (asumen lock externo) ----

    // Retorna puntero al nodo recien insertado (o existente si duplicado).
    // Permite a HashTable::operator[] hacer find_or_insert en una sola
    // pasada en vez de dos busquedas O(log n).
    virtual Node* internal_insert_unsafe(Node* &pNode, const value_type &data, Ref ref, Node* parent);

    // copy recursivo manteniendo m_pParent. Si no se propaga el parent,
    // los nodos copiados quedan con m_pParent = nullptr y los iteradores
    // inorder (que suben por padres para calcular sucesor) se rompen.
    Node* internal_copy_unsafe(const Node* src, Node* parent) const;

    void  internal_clear_unsafe(Node* n);

    Node* internal_search_unsafe(Node* n, const value_type& val) const;

    // leftmost: usado por begin() inorder
    Node* leftmost_unsafe(Node* n) const {
        if(!n) return nullptr;
        while(n->getChild(0)) n = n->getChild(0);
        return n;
    }
    Node* rightmost_unsafe(Node* n) const {
        if(!n) return nullptr;
        while(n->getChild(1)) n = n->getChild(1);
        return n;
    }

public:
    BinaryTree() = default;

    BinaryTree(const BinaryTree& other) {
        shared_lock<shared_mutex> lock(other.m_mtx);
        m_pRoot = internal_copy_unsafe(other.m_pRoot, nullptr);
        m_size  = other.m_size;
    }

    BinaryTree(BinaryTree&& other) noexcept {
        unique_lock<shared_mutex> lock(other.m_mtx);
        m_pRoot = std::exchange(other.m_pRoot, nullptr);
        m_size  = std::exchange(other.m_size, 0);
    }

    BinaryTree& operator=(const BinaryTree& other) {
        if(this == &other) return *this;
        if(this < &other){
            unique_lock<shared_mutex> lockThis(m_mtx);
            shared_lock<shared_mutex> lockOther(other.m_mtx);
            internal_clear_unsafe(m_pRoot);
            m_pRoot = internal_copy_unsafe(other.m_pRoot, nullptr);
            m_size  = other.m_size;
        } else {
            shared_lock<shared_mutex> lockOther(other.m_mtx);
            unique_lock<shared_mutex> lockThis(m_mtx);
            internal_clear_unsafe(m_pRoot);
            m_pRoot = internal_copy_unsafe(other.m_pRoot, nullptr);
            m_size  = other.m_size;
        }
        return *this;
    }

    BinaryTree& operator=(BinaryTree&& other) noexcept {
        if(this == &other) return *this;
        unique_lock<shared_mutex> lockThis(m_mtx);
        unique_lock<shared_mutex> lockOther(other.m_mtx);
        internal_clear_unsafe(m_pRoot);
        m_pRoot = std::exchange(other.m_pRoot, nullptr);
        m_size  = std::exchange(other.m_size, 0);
        return *this;
    }

    virtual ~BinaryTree() {
        unique_lock<shared_mutex> lock(m_mtx);
        internal_clear_unsafe(m_pRoot);
        m_pRoot = nullptr;
    }

    // Inserción pública
    virtual void insert(const value_type &data, Ref ref) {
        unique_lock<shared_mutex> lock(m_mtx);
        internal_insert_unsafe(m_pRoot, data, ref, nullptr);
    }

    // Búsqueda pública (devuelve true si existe)
    bool contains(const value_type& val) const {
        shared_lock<shared_mutex> lock(m_mtx);
        return internal_search_unsafe(m_pRoot, val) != nullptr;
    }

    size_t size() const {
        shared_lock<shared_mutex> lock(m_mtx);
        return m_size;
    }

    bool empty() const {
        shared_lock<shared_mutex> lock(m_mtx);
        return m_size == 0;
    }

    string toString() const {
        shared_lock<shared_mutex> lock(m_mtx);
        ostringstream oss;
        oss << "[";
        bool first = true;
        // recorrido inorder iterativo via leftmost+sucesor
        Node* cur = leftmost_unsafe(m_pRoot);
        while(cur){
            if(!first) oss << ",";
            oss << "(" << cur->getData() << "," << cur->getRef() << ")";
            first = false;
            // sucesor inorder inline (no podemos usar el iterador con lock ya tomado)
            if(cur->getChild(1)){
                cur = cur->getChild(1);
                while(cur->getChild(0)) cur = cur->getChild(0);
            } else {
                Node* p = cur->getParent();
                while(p && p->getChild(1) == cur){
                    cur = p;
                    p = p->getParent();
                }
                cur = p;
            }
        }
        oss << "]";
        return oss.str();
    }

    // Iteradores (no toman lock — el usuario es responsable durante iteración)
    forward_iterator  begin()  { return forward_iterator(this,  leftmost_unsafe(m_pRoot)); }
    forward_iterator  end()    { return forward_iterator(this,  nullptr); }
    backward_iterator rbegin() { return backward_iterator(this, rightmost_unsafe(m_pRoot)); }
    backward_iterator rend()   { return backward_iterator(this, nullptr); }

    friend ostream& operator<<(ostream& os, const BinaryTree& t) {
        return os << t.toString();
    }

    friend istream& operator>>(istream& is, BinaryTree& t) {
        char ch;
        if(!(is >> ch) || ch != '['){
            is.clear(ios_base::failbit);
            return is;
        }
        value_type val;
        Ref ref;
        char comma, paren;
        while(is >> ch && ch != ']'){
            if(ch == '('){
                if(is >> val >> comma >> ref >> paren){
                    if(comma == ',' && paren == ')'){
                        t.insert(val, ref);
                    }
                }
            }
        }
        return is;
    }
};

// ----- impl _unsafe helpers -----

template <typename Trait>
typename BinaryTree<Trait>::Node*
BinaryTree<Trait>::internal_insert_unsafe(Node* &pNode, const value_type &data, Ref ref, Node* parent){
    if(pNode == nullptr){
        pNode = new Node(data, ref, parent);
        ++m_size;
        return pNode;
    }
    int branch = m_comp(pNode->getData(), data) ? 1 : 0;
    return internal_insert_unsafe(pNode->getChildRef(branch), data, ref, pNode);
}

template <typename Trait>
typename BinaryTree<Trait>::Node*
BinaryTree<Trait>::internal_copy_unsafe(const Node* src, Node* parent) const {
    if(!src) return nullptr;
    Node* nuevo = new Node(src->getData(), src->getRef(), parent);
    nuevo->setChild(0, internal_copy_unsafe(src->getChild(0), nuevo));
    nuevo->setChild(1, internal_copy_unsafe(src->getChild(1), nuevo));
    return nuevo;
}

template <typename Trait>
void BinaryTree<Trait>::internal_clear_unsafe(Node* n){
    if(!n) return;
    internal_clear_unsafe(n->getChild(0));
    internal_clear_unsafe(n->getChild(1));
    delete n;
}

template <typename Trait>
typename BinaryTree<Trait>::Node*
BinaryTree<Trait>::internal_search_unsafe(Node* n, const value_type& val) const {
    while(n){
        if(!m_comp(n->getData(), val) && !m_comp(val, n->getData()))
            return n;
        n = m_comp(n->getData(), val) ? n->getChild(1) : n->getChild(0);
    }
    return nullptr;
}

#endif // __BINARYTREE_H__
