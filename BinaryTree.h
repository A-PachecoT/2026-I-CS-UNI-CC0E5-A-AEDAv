#ifndef __BINARYTREE_H__
#define __BINARYTREE_H__
#include <iostream>
#include <cstddef>
#include <string>
#include <fstream>
#include <thread>
#include <stack>
#include <vector>
#include <sstream>
#include <mutex>
#include <shared_mutex>
#include <utility>
#include <algorithm>
#include "containers/general_iterator.h"
#include "containers/traits.h"
#include "containers/util.h"
using namespace std;

// CRTP base — necesaria para que m_pChild tenga el tipo correcto
// cuando AVLNode hereda de BinaryTreeNodeBase.
template<typename Derived, typename T>
struct BinaryTreeNodeBase {
    using value_type = T;
    T        m_data;
    Derived* m_pChild[2];
    BinaryTreeNodeBase(T data) : m_data(data), m_pChild{nullptr, nullptr} {}
    virtual ~BinaryTreeNodeBase() = default;
    T& getDataRef()       { return m_data; }
    T  getData()    const { return m_data; }
};

template<typename T>
struct BinaryTreeNode : BinaryTreeNodeBase<BinaryTreeNode<T>, T> {
    BinaryTreeNode(T data) : BinaryTreeNodeBase<BinaryTreeNode<T>, T>(data) {}
};

// Traits específicos para BST sobre BinaryTreeNode
template<typename T> using AscendingBSTrait  = AscendingTrait<BinaryTreeNode<T>>;
template<typename T> using DescendingBSTrait = DescendingTrait<BinaryTreeNode<T>>;

// Forward declarations de iteradores
template<typename C> class BTInorderForwardIterator;
template<typename C> class BTInorderBackwardIterator;
template<typename C> class BTPreorderForwardIterator;
template<typename C> class BTPreorderBackwardIterator;
template<typename C> class BTPostorderForwardIterator;
template<typename C> class BTPostorderBackwardIterator;

template<typename Trait>
class BinaryTree {
public:
    using value_type = typename Trait::value_type;
    using Node       = typename Trait::Node;
    using Comp       = typename Trait::Comp;
    using MySelf     = BinaryTree<Trait>;

    using forward_iterator            = BTInorderForwardIterator<MySelf>;
    using backward_iterator           = BTInorderBackwardIterator<MySelf>;
    using preorder_forward_iterator   = BTPreorderForwardIterator<MySelf>;
    using preorder_backward_iterator  = BTPreorderBackwardIterator<MySelf>;
    using postorder_forward_iterator  = BTPostorderForwardIterator<MySelf>;
    using postorder_backward_iterator = BTPostorderBackwardIterator<MySelf>;

    friend forward_iterator;
    friend backward_iterator;
    friend preorder_forward_iterator;
    friend preorder_backward_iterator;
    friend postorder_forward_iterator;
    friend postorder_backward_iterator;

protected:
    Node*               m_pRoot;
    Comp                m_comp;
    size_t              m_size;
    mutable shared_mutex m_mtx;

public:
    BinaryTree() : m_pRoot(nullptr), m_size(0) {}

    // Constructor copia (deep copy)
    BinaryTree(const BinaryTree& other) : m_pRoot(nullptr), m_size(0) {
        shared_lock<shared_mutex> lock(other.m_mtx);
        m_pRoot = internal_copy(other.m_pRoot);
        m_size  = other.m_size;
    }

    // Move constructor
    BinaryTree(BinaryTree&& other) noexcept : m_pRoot(nullptr), m_size(0) {
        unique_lock<shared_mutex> lock(other.m_mtx);
        m_pRoot = exchange(other.m_pRoot, nullptr);
        m_size  = exchange(other.m_size, (size_t)0);
    }

    ~BinaryTree() { internal_destroy(m_pRoot); }

private:
    void internal_destroy(Node* node) {
        if (!node) return;
        internal_destroy(node->m_pChild[0]);
        internal_destroy(node->m_pChild[1]);
        delete node;
    }

    Node* internal_copy(Node* src) {
        if (!src) return nullptr;
        Node* n = new Node(src->m_data);
        n->m_pChild[0] = internal_copy(src->m_pChild[0]);
        n->m_pChild[1] = internal_copy(src->m_pChild[1]);
        return n;
    }

    void inorder_str(Node* node, ostringstream& oss, bool& first) const {
        if (!node) return;
        inorder_str(node->m_pChild[0], oss, first);
        if (!first) oss << ",";
        oss << node->m_data;
        first = false;
        inorder_str(node->m_pChild[1], oss, first);
    }

    size_t internal_height(Node* node) const {
        if (!node) return 0;
        return 1 + max(internal_height(node->m_pChild[0]),
                       internal_height(node->m_pChild[1]));
    }

protected:
    // virtual: la subclase AVL lo override con rebalanceo
    // branch = 1 (derecha) si data NO es menor que pNode → pone equals a la derecha
    virtual void internal_insert(Node*& pNode, value_type data) {
        if (!pNode) { pNode = new Node(data); ++m_size; return; }
        auto branch = !m_comp(data, pNode->m_data);
        internal_insert(pNode->m_pChild[branch], data);
    }

public:
    virtual void insert(value_type data) {
        unique_lock<shared_mutex> lock(m_mtx);
        internal_insert(m_pRoot, data);
    }

    size_t size() const {
        shared_lock<shared_mutex> lock(m_mtx);
        return m_size;
    }

    size_t height() const {
        shared_lock<shared_mutex> lock(m_mtx);
        return internal_height(m_pRoot);
    }

    int balance_factor(Node* node) const {
        if (!node) node = m_pRoot;
        if (!node) return 0;
        return (int)internal_height(node->m_pChild[0])
             - (int)internal_height(node->m_pChild[1]);
    }

    bool contains(value_type val) const {
        shared_lock<shared_mutex> lock(m_mtx);
        Node* cur = m_pRoot;
        while (cur) {
            bool less_than    = m_comp(val, cur->m_data);
            bool greater_than = m_comp(cur->m_data, val);
            if (!less_than && !greater_than) return true;
            cur = cur->m_pChild[!less_than];
        }
        return false;
    }

    string ToString() const {
        shared_lock<shared_mutex> lock(m_mtx);
        ostringstream oss;
        bool first = true;
        oss << "[";
        inorder_str(m_pRoot, oss, first);
        oss << "]";
        return oss.str();
    }

    friend ostream& operator<<(ostream& os, const BinaryTree& t) {
        os << t.ToString();
        return os;
    }

    // Parsea formato "[v1,v2,v3,...]"
    friend istream& operator>>(istream& is, BinaryTree& t) {
        char ch;
        if (!(is >> ch) || ch != '[') { is.setstate(ios::failbit); return is; }
        value_type val;
        while (is >> ch) {
            if (ch == ']') break;
            if (ch != ',') is.putback(ch);
            if (is >> val) t.insert(val);
        }
        return is;
    }

    forward_iterator            begin()             { return forward_iterator(this, m_pRoot); }
    forward_iterator            end()               { return forward_iterator(this, nullptr); }
    backward_iterator           rbegin()            { return backward_iterator(this, m_pRoot); }
    backward_iterator           rend()              { return backward_iterator(this, nullptr); }
    preorder_forward_iterator   preorder_begin()    { return preorder_forward_iterator(this, m_pRoot); }
    preorder_forward_iterator   preorder_end()      { return preorder_forward_iterator(this, nullptr); }
    preorder_backward_iterator  preorder_rbegin()   { return preorder_backward_iterator(this, m_pRoot); }
    preorder_backward_iterator  preorder_rend()     { return preorder_backward_iterator(this, nullptr); }
    postorder_forward_iterator  postorder_begin()   { return postorder_forward_iterator(this, m_pRoot); }
    postorder_forward_iterator  postorder_end()     { return postorder_forward_iterator(this, nullptr); }
    postorder_backward_iterator postorder_rbegin()  { return postorder_backward_iterator(this, m_pRoot); }
    postorder_backward_iterator postorder_rend()    { return postorder_backward_iterator(this, nullptr); }

    template<typename Func, typename... Args>
    void ForEach(Func func, Args&&... args) {
        shared_lock<shared_mutex> lock(m_mtx);
        if (m_size == 0) return;
        for (auto it = begin(); it != end(); ++it)
            func(*it, std::forward<Args>(args)...);
    }
};

// ============ Iteradores ============

// Inorder forward (left-root-right)
template<typename Container>
class BTInorderForwardIterator
    : public general_iterator<Container, BTInorderForwardIterator<Container>> {
public:
    using MySelf = BTInorderForwardIterator<Container>;
    using Parent = general_iterator<Container, MySelf>;
    using Node   = typename Container::Node;
    using Parent::Parent;
private:
    stack<Node*> m_stack;
    void push_left(Node* n) { while (n) { m_stack.push(n); n = n->m_pChild[0]; } }
    void advance() {
        if (m_stack.empty()) { this->m_pNode = nullptr; return; }
        Node* n = m_stack.top(); m_stack.pop();
        this->m_pNode = n;
        push_left(n->m_pChild[1]);
    }
public:
    BTInorderForwardIterator(Container* c, Node* root)
        : Parent(c, nullptr) { push_left(root); advance(); }
    BTInorderForwardIterator(Container* c, nullptr_t)
        : Parent(c, nullptr) {}
    MySelf& operator++() { advance(); return *this; }
};

// Inorder backward (right-root-left)
template<typename Container>
class BTInorderBackwardIterator
    : public general_iterator<Container, BTInorderBackwardIterator<Container>> {
public:
    using MySelf = BTInorderBackwardIterator<Container>;
    using Parent = general_iterator<Container, MySelf>;
    using Node   = typename Container::Node;
    using Parent::Parent;
private:
    stack<Node*> m_stack;
    void push_right(Node* n) { while (n) { m_stack.push(n); n = n->m_pChild[1]; } }
    void advance() {
        if (m_stack.empty()) { this->m_pNode = nullptr; return; }
        Node* n = m_stack.top(); m_stack.pop();
        this->m_pNode = n;
        push_right(n->m_pChild[0]);
    }
public:
    BTInorderBackwardIterator(Container* c, Node* root)
        : Parent(c, nullptr) { push_right(root); advance(); }
    BTInorderBackwardIterator(Container* c, nullptr_t)
        : Parent(c, nullptr) {}
    MySelf& operator++() { advance(); return *this; }
};

// Preorder forward (root-left-right)
template<typename Container>
class BTPreorderForwardIterator
    : public general_iterator<Container, BTPreorderForwardIterator<Container>> {
public:
    using MySelf = BTPreorderForwardIterator<Container>;
    using Parent = general_iterator<Container, MySelf>;
    using Node   = typename Container::Node;
    using Parent::Parent;
private:
    stack<Node*> m_stack;
    void advance() {
        if (m_stack.empty()) { this->m_pNode = nullptr; return; }
        Node* n = m_stack.top(); m_stack.pop();
        this->m_pNode = n;
        if (n->m_pChild[1]) m_stack.push(n->m_pChild[1]);
        if (n->m_pChild[0]) m_stack.push(n->m_pChild[0]);
    }
public:
    BTPreorderForwardIterator(Container* c, Node* root)
        : Parent(c, nullptr) { if (root) m_stack.push(root); advance(); }
    BTPreorderForwardIterator(Container* c, nullptr_t)
        : Parent(c, nullptr) {}
    MySelf& operator++() { advance(); return *this; }
};

// Preorder backward (precompute, traverse en reversa)
template<typename Container>
class BTPreorderBackwardIterator
    : public general_iterator<Container, BTPreorderBackwardIterator<Container>> {
public:
    using MySelf = BTPreorderBackwardIterator<Container>;
    using Parent = general_iterator<Container, MySelf>;
    using Node   = typename Container::Node;
    using Parent::Parent;
private:
    vector<Node*> m_nodes;
    int           m_idx;
    void collect(Node* n) {
        if (!n) return;
        m_nodes.push_back(n);
        collect(n->m_pChild[0]);
        collect(n->m_pChild[1]);
    }
public:
    BTPreorderBackwardIterator(Container* c, Node* root)
        : Parent(c, nullptr), m_idx(-1) {
        collect(root);
        m_idx = (int)m_nodes.size() - 1;
        this->m_pNode = (m_idx >= 0) ? m_nodes[m_idx] : nullptr;
    }
    BTPreorderBackwardIterator(Container* c, nullptr_t)
        : Parent(c, nullptr), m_idx(-1) {}
    MySelf& operator++() {
        --m_idx;
        this->m_pNode = (m_idx >= 0) ? m_nodes[m_idx] : nullptr;
        return *this;
    }
};

// Postorder forward (left-right-root) — two-stack technique
template<typename Container>
class BTPostorderForwardIterator
    : public general_iterator<Container, BTPostorderForwardIterator<Container>> {
public:
    using MySelf = BTPostorderForwardIterator<Container>;
    using Parent = general_iterator<Container, MySelf>;
    using Node   = typename Container::Node;
    using Parent::Parent;
private:
    stack<Node*> m_stack;
    void build(Node* root) {
        if (!root) return;
        stack<Node*> s1;
        s1.push(root);
        while (!s1.empty()) {
            Node* n = s1.top(); s1.pop();
            m_stack.push(n);
            if (n->m_pChild[0]) s1.push(n->m_pChild[0]);
            if (n->m_pChild[1]) s1.push(n->m_pChild[1]);
        }
    }
    void advance() {
        if (m_stack.empty()) { this->m_pNode = nullptr; return; }
        this->m_pNode = m_stack.top(); m_stack.pop();
    }
public:
    BTPostorderForwardIterator(Container* c, Node* root)
        : Parent(c, nullptr) { build(root); advance(); }
    BTPostorderForwardIterator(Container* c, nullptr_t)
        : Parent(c, nullptr) {}
    MySelf& operator++() { advance(); return *this; }
};

// Postorder backward (root-right-left iterativo)
template<typename Container>
class BTPostorderBackwardIterator
    : public general_iterator<Container, BTPostorderBackwardIterator<Container>> {
public:
    using MySelf = BTPostorderBackwardIterator<Container>;
    using Parent = general_iterator<Container, MySelf>;
    using Node   = typename Container::Node;
    using Parent::Parent;
private:
    stack<Node*> m_stack;
    void advance() {
        if (m_stack.empty()) { this->m_pNode = nullptr; return; }
        Node* n = m_stack.top(); m_stack.pop();
        this->m_pNode = n;
        if (n->m_pChild[0]) m_stack.push(n->m_pChild[0]);
        if (n->m_pChild[1]) m_stack.push(n->m_pChild[1]);
    }
public:
    BTPostorderBackwardIterator(Container* c, Node* root)
        : Parent(c, nullptr) { if (root) m_stack.push(root); advance(); }
    BTPostorderBackwardIterator(Container* c, nullptr_t)
        : Parent(c, nullptr) {}
    MySelf& operator++() { advance(); return *this; }
};

#endif // __BINARYTREE_H__
