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
    using value_type  = T;
    using branch_type = size_t;
    using Node = typename std::conditional<std::is_void<NodeType>::value,
                                           BinaryTreeNode<T, void>,
                                           NodeType>::type;
protected:
    T     m_data;
    Ref   m_ref;
    Node *m_pChild[2];
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

    Node*      getChild(branch_type b) const    { return m_pChild[b]; }
    Node*&     getChildRef(branch_type b)       { return m_pChild[b]; }
    void       setChild(branch_type b, Node* c) { m_pChild[b] = c; }

    Node*      getParent() const       { return m_pParent; }
    void       setParent(Node* p)      { m_pParent = p; }
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

// Iterador inorder parametrizado por direccion.
// Dir=1: forward (sucesor inorder). Dir=0: backward (predecesor inorder).
template <typename Container, std::size_t Dir>
class bt_inorder_iterator
    : public general_iterator<Container, bt_inorder_iterator<Container, Dir>>
{
public:
    using Parent = general_iterator<Container, bt_inorder_iterator<Container, Dir>>;
    using Node   = typename Container::Node;
    using Parent::Parent;
    using typename Parent::MySelf;

    MySelf operator++() {
        static constexpr std::size_t Other = 1 - Dir;
        Node* n = this->m_pNode;
        if(!n) return *this;
        if(n->getChild(Dir)){
            n = n->getChild(Dir);
            while(n->getChild(Other)) n = n->getChild(Other);
            this->m_pNode = n;
        } else {
            Node* p = n->getParent();
            while(p && p->getChild(Dir) == n){
                n = p;
                p = p->getParent();
            }
            this->m_pNode = p;
        }
        return *this;
    }
};

template <typename Container>
using bt_inorder_forward_iterator  = bt_inorder_iterator<Container, 1>;

template <typename Container>
using bt_inorder_backward_iterator = bt_inorder_iterator<Container, 0>;

enum class Traversal { Inorder, Preorder, Postorder };

template <typename Trait>
class BinaryTree {
public:
    using value_type  = typename Trait::value_type;
    using Node        = typename Trait::Node;
    using Comp        = typename Trait::Comp;
    using MySelf      = BinaryTree<Trait>;
    using size_type   = std::size_t;
    using branch_type = typename Node::branch_type;
    using forward_iterator  = bt_inorder_forward_iterator<MySelf>;
    using backward_iterator = bt_inorder_backward_iterator<MySelf>;
    friend forward_iterator;
    friend backward_iterator;

protected:
    Node *m_pRoot = nullptr;
    size_type m_size = 0;
    Comp   m_comp;
    mutable shared_mutex m_mtx;

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

    size_type size() const {
        shared_lock<shared_mutex> lock(m_mtx);
        return m_size;
    }

    bool empty() const {
        shared_lock<shared_mutex> lock(m_mtx);
        return m_size == 0;
    }

    // toString parametrizado por modo de recorrido. Inorder por default
    // mantiene el comportamiento original; preorder/postorder son recursivos
    // bajo el mismo lock.
    string toString(Traversal mode = Traversal::Inorder) const {
        shared_lock<shared_mutex> lock(m_mtx);
        ostringstream oss;
        oss << "[";
        bool first = true;

        auto emit = [&](const Node* n){
            if(!first) oss << ",";
            oss << "(" << n->getData() << "," << n->getRef() << ")";
            first = false;
        };

        if(mode == Traversal::Inorder){
            // reutiliza el iterador forward — mismo recorrido
            for(forward_iterator it(const_cast<MySelf*>(this), leftmost_unsafe(m_pRoot));
                it.getNode() != nullptr; ++it)
                emit(it.getNode());
        } else if(mode == Traversal::Preorder){
            // raiz, izq, der  — DFS recursivo
            auto walk = [&](auto& self, const Node* n) -> void {
                if(!n) return;
                emit(n);
                self(self, n->getChild(0));
                self(self, n->getChild(1));
            };
            walk(walk, m_pRoot);
        } else { // Postorder
            // izq, der, raiz
            auto walk = [&](auto& self, const Node* n) -> void {
                if(!n) return;
                self(self, n->getChild(0));
                self(self, n->getChild(1));
                emit(n);
            };
            walk(walk, m_pRoot);
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
        Token ch;
        if(!(is >> ch) || ch != '['){
            is.clear(ios_base::failbit);
            return is;
        }
        value_type val;
        Ref ref;
        Token comma, paren;
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
    branch_type branch = m_comp(pNode->getData(), data) ? 1 : 0;
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
