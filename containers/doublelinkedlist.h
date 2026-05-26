#ifndef __DOUBLELINKEDLIST_H__
#define __DOUBLELINKEDLIST_H__

#include "linkedlist.h"

// DLLNode: hereda de LLNode via CRTP y agrega el puntero previo
template <typename T>
class DLLNode : public LLNode<T, DLLNode<T>>{
    using Node = DLLNode<T>;
private:
    Node *m_pPrev;
public:
    DLLNode() : LLNode<T, DLLNode<T>>(), m_pPrev(nullptr) {}
    DLLNode(T data, Ref ref, Node *next = nullptr, Node *prev = nullptr)
        : LLNode<T, DLLNode<T>>(data, ref, next), m_pPrev(prev) {}
    virtual ~DLLNode() {}

    Node*  getPrev() const     { return m_pPrev; }
    void   setPrev(Node *prev) { m_pPrev = prev; }
    Node*& getPrevRef()        { return m_pPrev; }
};

// Traits adaptadas a BaseTrait paramétrica
template <typename T>
struct AscendingDLLTrait : BaseTrait<T, less<T>, DLLNode<T>> {};

template <typename T>
struct DescendingDLLTrait : BaseTrait<T, greater<T>, DLLNode<T>> {};

// Forward iterator: avanza con getNext()
template <typename Container>
class DoubleLinkedListForwardIterator : public general_iterator<Container, DoubleLinkedListForwardIterator<Container>>{
public:
    using MySelf = DoubleLinkedListForwardIterator<Container>;
    using Parent = general_iterator<Container, MySelf>;
    using Parent::Parent;

    MySelf operator++() {
        if (this->m_pNode) this->m_pNode = this->m_pNode->getNext();
        return *this;
    }
};

// Backward iterator: avanza (en su sentido) con getPrev()
template <typename Container>
class DoubleLinkedListBackwardIterator : public general_iterator<Container, DoubleLinkedListBackwardIterator<Container>>{
public:
    using MySelf = DoubleLinkedListBackwardIterator<Container>;
    using Parent = general_iterator<Container, MySelf>;
    using Parent::Parent;

    MySelf operator++() {
        if (this->m_pNode) this->m_pNode = this->m_pNode->getPrev();
        return *this;
    }
};

template <typename Trait>
class DoubleLinkedList : public LinkedList<Trait>{
public:
    using value_type        = typename Trait::value_type;
    using Node              = typename Trait::Node;
    using Comp              = typename Trait::Comp;
    using MySelf            = DoubleLinkedList<Trait>;
    using forward_iterator  = DoubleLinkedListForwardIterator<MySelf>;
    using backward_iterator = DoubleLinkedListBackwardIterator<MySelf>;
    friend forward_iterator;
    friend backward_iterator;

    using Parent = LinkedList<Trait>;
    using Parent::m_pRoot;
    using Parent::m_tail;
    using Parent::m_size;
    using Parent::m_comp;
    using Parent::m_mtx;

private:
    void internal_insert(Node *&actual, Node *pPrevNode, const value_type &value, Ref ref){
        if (!actual || m_comp(value, actual->getDataRef())) {
            Node *newNode = new Node(value, ref, actual, pPrevNode);
            if (actual) actual->setPrev(newNode);
            actual = newNode;
            m_size++;
            if (newNode->getNext() == nullptr) m_tail = newNode;
            return;
        }
        internal_insert(actual->getNextRef(), actual, value, ref);
    }

public:
    DoubleLinkedList() : Parent() {}

    DoubleLinkedList(const DoubleLinkedList &other) : Parent() {
        shared_lock<shared_mutex> lock(other.m_mtx);
        for (Node *c = other.m_pRoot; c != nullptr; c = c->getNext())
            push_back(c->getData(), c->getRef());
    }

    DoubleLinkedList(DoubleLinkedList &&other) : Parent() {
        unique_lock<shared_mutex> lock(other.m_mtx);
        m_pRoot = std::exchange(other.m_pRoot, nullptr);
        m_tail  = std::exchange(other.m_tail,  nullptr);
        m_size  = std::exchange(other.m_size,  0);
    }

    DoubleLinkedList& operator=(const DoubleLinkedList &other) {
        if (this != &other) {
            clear();
            shared_lock<shared_mutex> lock(other.m_mtx);
            for (Node *c = other.m_pRoot; c != nullptr; c = c->getNext())
                push_back(c->getData(), c->getRef());
        }
        return *this;
    }

    DoubleLinkedList& operator=(DoubleLinkedList &&other) {
        if (this != &other) {
            clear();
            unique_lock<shared_mutex> lock(other.m_mtx);
            m_pRoot = std::exchange(other.m_pRoot, nullptr);
            m_tail  = std::exchange(other.m_tail,  nullptr);
            m_size  = std::exchange(other.m_size,  0);
        }
        return *this;
    }

    virtual ~DoubleLinkedList() { clear(); }

    void clear() {
        unique_lock<shared_mutex> lock(m_mtx);
        Node *act = m_pRoot;
        while (act) {
            Node *next = act->getNext();
            delete act;
            act = next;
        }
        m_pRoot = nullptr;
        m_tail  = nullptr;
        m_size  = 0;
    }

    void push_front(value_type value, Ref ref) override {
        unique_lock<shared_mutex> lock(m_mtx);
        Node *newNode = new Node(value, ref, m_pRoot, nullptr);
        if (m_pRoot) m_pRoot->setPrev(newNode);
        else         m_tail = newNode;
        m_pRoot = newNode;
        m_size++;
    }

    void push_back(value_type value, Ref ref) override {
        unique_lock<shared_mutex> lock(m_mtx);
        Node *newNode = new Node(value, ref, nullptr, m_tail);
        if (m_tail) m_tail->setNext(newNode);
        else        m_pRoot = newNode;
        m_tail = newNode;
        m_size++;
    }

    void insert(const value_type &value, Ref ref) override {
        unique_lock<shared_mutex> lock(m_mtx);
        internal_insert(m_pRoot, nullptr, value, ref);
        if (m_size == 1) m_tail = m_pRoot;
    }

    std::tuple<value_type, Ref> pop_front() override {
        unique_lock<shared_mutex> lock(m_mtx);
        if (!m_pRoot) throw runtime_error("La lista esta vacia");
        Node *temp = m_pRoot;
        auto result = std::make_tuple(temp->getData(), temp->getRef());
        m_pRoot = temp->getNext();
        if (m_pRoot) m_pRoot->setPrev(nullptr);
        else         m_tail = nullptr;
        delete temp;
        m_size--;
        return result;
    }

    std::tuple<value_type, Ref> pop_back() override {
        unique_lock<shared_mutex> lock(m_mtx);
        if (!m_pRoot) throw runtime_error("La lista esta vacia");
        Node *temp = m_tail;
        auto result = std::make_tuple(temp->getData(), temp->getRef());
        m_tail = temp->getPrev();
        if (m_tail) m_tail->setNext(nullptr);
        else        m_pRoot = nullptr;
        delete temp;
        m_size--;
        return result;
    }

    forward_iterator  begin()  { return forward_iterator (this, m_pRoot); }
    forward_iterator  end()    { return forward_iterator (this, nullptr); }
    backward_iterator rbegin() { return backward_iterator(this, m_tail);  }
    backward_iterator rend()   { return backward_iterator(this, nullptr); }

    template <typename Func, typename... Args>
    void ForEach(Func func, Args &&... args) {
        unique_lock<shared_mutex> lock(m_mtx);
        if (m_size == 0) return;
        for (auto& item : *this) func(item, std::forward<Args>(args)...);
    }

    template <typename Func, typename... Args>
    void ReverseForEach(Func func, Args &&... args) {
        unique_lock<shared_mutex> lock(m_mtx);
        if (m_size == 0) return;
        for (auto it = rbegin(); it != rend(); ++it)
            func(*it, std::forward<Args>(args)...);
    }

    friend ostream& operator<<(ostream& os, const DoubleLinkedList& list) {
        shared_lock<shared_mutex> lock(list.m_mtx);
        os << "[";
        Node *act = list.m_pRoot;
        while (act) {
            os << "(" << act->getData() << "," << act->getRef() << ")";
            if (act->getNext()) os << ",";
            act = act->getNext();
        }
        os << "]";
        return os;
    }

    friend ostream& operator<<(ostream& os, DoubleLinkedList& list) {
        return os << static_cast<const DoubleLinkedList&>(list);
    }

    friend istream& operator>>(istream& is, DoubleLinkedList& list) {
        char ch;
        if (!(is >> ch) || ch != '[') {
            is.clear(ios_base::failbit);
            return is;
        }
        value_type val;
        Ref ref;
        char comma, parenClose;
        while (is >> ch && ch != ']') {
            if (ch == '(') {
                if (is >> val >> comma >> ref >> parenClose) {
                    if (comma == ',' && parenClose == ')')
                        list.insert(val, ref);
                }
            }
        }
        return is;
    }
};

#endif // __DOUBLELINKEDLIST_H__