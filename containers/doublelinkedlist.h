#ifndef __DOUBLELINKEDLIST_H__
#define __DOUBLELINKEDLIST_H__

#include "linkedlist.h"

// DLLNode: nodo doblemente enlazado. Reusa LLNode con CRTP, agrega m_pPrev.
template <typename T>
class DLLNode : public LLNode<T, DLLNode<T>> {
public:
    using Node = DLLNode<T>;
    using Base = LLNode<T, Node>;
private:
    Node *m_pPrev;
public:
    DLLNode() : Base(), m_pPrev(nullptr) {}
    DLLNode(T data, Ref ref, Node *next = nullptr, Node *prev = nullptr)
        : Base(data, ref, next), m_pPrev(prev) {}
    virtual ~DLLNode() {}

    Node*  getPrev() const     { return m_pPrev; }
    void   setPrev(Node *prev) { m_pPrev = prev; }
    Node*& getPrevRef()        { return m_pPrev; }
};

// Traits para DLL
template <typename T>
struct AscendingDLLTrait : BaseTrait<T, less<T>> {
    using Node = DLLNode<T>;
};

template <typename T>
struct DescendingDLLTrait : BaseTrait<T, greater<T>> {
    using Node = DLLNode<T>;
};

// Iterador forward: avanza con getNext()
template <typename Container>
class DoubleLinkedListForwardIterator
    : public general_iterator<Container, DoubleLinkedListForwardIterator<Container>> {
public:
    using MySelf = DoubleLinkedListForwardIterator<Container>;
    using Parent = general_iterator<Container, MySelf>;
    using Parent::Parent;

    MySelf operator++() {
        if (this->m_pNode) this->m_pNode = this->m_pNode->getNext();
        return *this;
    }
};

// Iterador backward: avanza con getPrev() (op++ recorre la lista en su sentido).
template <typename Container>
class DoubleLinkedListBackwardIterator
    : public general_iterator<Container, DoubleLinkedListBackwardIterator<Container>> {
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
class DoubleLinkedList : public LinkedList<Trait> {
public:
    using value_type        = typename Trait::value_type;
    using Node              = typename Trait::Node;
    using Comp              = typename Trait::Comp;
    using MySelf            = DoubleLinkedList<Trait>;
    using forward_iterator  = DoubleLinkedListForwardIterator<MySelf>;
    using backward_iterator = DoubleLinkedListBackwardIterator<MySelf>;
    friend forward_iterator;
    friend backward_iterator;

private:
    Node  *m_pRoot = nullptr;
    Node  *m_tail  = nullptr;
    size_t m_size  = 0;
    Comp   m_comp;
    mutable shared_mutex m_mtx;

    // Inserción ordenada manteniendo prev correctamente.
    void internal_insert(Node *&actual, Node *pPrevNode, const value_type &value, Ref ref) {
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
    DoubleLinkedList() : LinkedList<Trait>() {}

    DoubleLinkedList(const DoubleLinkedList &other)
        : LinkedList<Trait>(), m_pRoot(nullptr), m_tail(nullptr), m_size(0) {
        shared_lock<shared_mutex> lock(other.m_mtx);
        for (Node *c = other.m_pRoot; c != nullptr; c = c->getNext())
            push_back(c->getData(), c->getRef());
    }

    DoubleLinkedList(DoubleLinkedList &&other)
        : LinkedList<Trait>(), m_pRoot(nullptr), m_tail(nullptr), m_size(0) {
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
    }

    std::tuple<value_type, Ref> pop_front() override {
        unique_lock<shared_mutex> lock(m_mtx);
        if (!m_pRoot) throw runtime_error("La lista esta vacia");
        Node *temp  = m_pRoot;
        auto result = std::make_tuple(temp->getData(), temp->getRef());
        m_pRoot     = temp->getNext();
        if (m_pRoot) m_pRoot->setPrev(nullptr);
        else         m_tail = nullptr;
        delete temp;
        m_size--;
        return result;
    }

    std::tuple<value_type, Ref> pop_back() override {
        unique_lock<shared_mutex> lock(m_mtx);
        if (!m_tail) throw runtime_error("La lista esta vacia");
        Node *temp  = m_tail;
        auto result = std::make_tuple(temp->getData(), temp->getRef());
        m_tail      = temp->getPrev();
        if (m_tail) m_tail->setNext(nullptr);
        else        m_pRoot = nullptr;
        delete temp;
        m_size--;
        return result;
    }

    value_type& operator[](size_t index) override {
        shared_lock<shared_mutex> lock(m_mtx);
        if (index >= m_size) throw out_of_range("Indice fuera de rango");
        Node *act = m_pRoot;
        for (size_t i = 0; i < index; ++i) act = act->getNext();
        return act->getDataRef();
    }

    size_t size() const override {
        shared_lock<shared_mutex> lock(m_mtx);
        return m_size;
    }

    forward_iterator  begin()  { return forward_iterator (this, m_pRoot); }
    forward_iterator  end()    { return forward_iterator (this, nullptr); }
    backward_iterator rbegin() { return backward_iterator(this, m_tail);  }
    backward_iterator rend()   { return backward_iterator(this, nullptr); }

    template <typename Func, typename... Args>
    void ForEach(Func func, Args&&... args) {
        shared_lock<shared_mutex> lock(m_mtx);
        for (Node *act = m_pRoot; act; act = act->getNext())
            func(act->getDataRef(), std::forward<Args>(args)...);
    }

    template <typename Func, typename... Args>
    void ReverseForEach(Func func, Args&&... args) {
        shared_lock<shared_mutex> lock(m_mtx);
        for (Node *act = m_tail; act; act = act->getPrev())
            func(act->getDataRef(), std::forward<Args>(args)...);
    }

    // op<<: formato `[(val,ref),...]` para roundtrip con op>>.
    // La bidireccionalidad se evidencia recorriendo con rbegin/rend en el demo.
    friend ostream& operator<<(ostream &os, const DoubleLinkedList &list) {
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

    // Helper de defensa: imprime fwd y bwd para demostrar la bidireccionalidad
    // sin romper el contrato simétrico de op<</op>>.
    void dumpBidirectional(ostream &os) const {
        shared_lock<shared_mutex> lock(m_mtx);
        os << "fwd[";
        Node *act = m_pRoot;
        while (act) {
            os << "(" << act->getData() << "," << act->getRef() << ")";
            if (act->getNext()) os << ",";
            act = act->getNext();
        }
        os << "] | bwd[";
        act = m_tail;
        while (act) {
            os << "(" << act->getData() << "," << act->getRef() << ")";
            if (act->getPrev()) os << ",";
            act = act->getPrev();
        }
        os << "]";
    }

    friend istream& operator>>(istream &is, DoubleLinkedList &list) {
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
