#ifndef __CIRCULARLINKEDLIST_H__
#define __CIRCULARLINKEDLIST_H__

#include "linkedlist.h"

template <typename T>
struct AscendingCLLTrait : BaseTrait<T, less<T>, LLNode<T>> {};

template <typename T>
struct DescendingCLLTrait : BaseTrait<T, greater<T>, LLNode<T>> {};

// Iterador Forward Circular (usa marcador m_start para no loopear infinito)
template <typename Container>
class CLLForwardIterator : public general_iterator<Container, CLLForwardIterator<Container>>{
public:
    using MySelf = CLLForwardIterator<Container>;
    using Parent = general_iterator<Container, MySelf>;
    using Node   = typename Container::Node;
private:
    Node *m_start;
public:
    CLLForwardIterator(Container *pContainer, Node *pNode)
        : Parent(pContainer, pNode), m_start(pNode) {}
    CLLForwardIterator(Container *pContainer, Node *pNode, Node *start)
        : Parent(pContainer, pNode), m_start(start) {}

    MySelf operator++() {
        if (this->m_pNode) {
            this->m_pNode = this->m_pNode->getNext();
            if (this->m_pNode == m_start) this->m_pNode = nullptr;
        }
        return *this;
    }
};

template <typename Trait>
class CircularLinkedList : public LinkedList<Trait>{
public:
    using value_type       = typename Trait::value_type;
    using Node             = typename Trait::Node;
    using Comp             = typename Trait::Comp;
    using MySelf           = CircularLinkedList<Trait>;
    using forward_iterator = CLLForwardIterator<MySelf>;
    friend forward_iterator;

    using Parent = LinkedList<Trait>;
    using Parent::m_pRoot;
    using Parent::m_tail;
    using Parent::m_size;
    using Parent::m_comp;
    using Parent::m_mtx;

private:
    void internal_insert(const value_type &value, Ref ref){
        Node *newNode = new Node(value, ref);
        m_size++;
        if (!m_pRoot) {
            newNode->setNext(newNode);
            m_pRoot = newNode;
            m_tail  = newNode;
            return;
        }
        if (m_comp(value, m_pRoot->getDataRef())) {
            newNode->setNext(m_pRoot);
            m_tail->setNext(newNode);
            m_pRoot = newNode;
            return;
        }
        Node *act = m_pRoot;
        while (act->getNext() != m_pRoot && !m_comp(value, act->getNext()->getDataRef()))
            act = act->getNext();
        newNode->setNext(act->getNext());
        act->setNext(newNode);
        if (act == m_tail) m_tail = newNode;
    }

public:
    CircularLinkedList() : Parent() {}

    CircularLinkedList(const CircularLinkedList &other) : Parent() {
        shared_lock<shared_mutex> lock(other.m_mtx);
        if (!other.m_pRoot) return;
        Node *curr = other.m_pRoot;
        do {
            push_back(curr->getData(), curr->getRef());
            curr = curr->getNext();
        } while (curr != other.m_pRoot);
    }

    CircularLinkedList(CircularLinkedList &&other) : Parent() {
        unique_lock<shared_mutex> lock(other.m_mtx);
        m_pRoot = std::exchange(other.m_pRoot, nullptr);
        m_tail  = std::exchange(other.m_tail,  nullptr);
        m_size  = std::exchange(other.m_size,  0);
    }

    CircularLinkedList& operator=(const CircularLinkedList &other) {
        if (this != &other) {
            clear();
            shared_lock<shared_mutex> lock(other.m_mtx);
            if (!other.m_pRoot) return *this;
            Node *curr = other.m_pRoot;
            do {
                push_back(curr->getData(), curr->getRef());
                curr = curr->getNext();
            } while (curr != other.m_pRoot);
        }
        return *this;
    }

    CircularLinkedList& operator=(CircularLinkedList &&other) {
        if (this != &other) {
            clear();
            unique_lock<shared_mutex> lock(other.m_mtx);
            m_pRoot = std::exchange(other.m_pRoot, nullptr);
            m_tail  = std::exchange(other.m_tail,  nullptr);
            m_size  = std::exchange(other.m_size,  0);
        }
        return *this;
    }

    virtual ~CircularLinkedList() { clear(); }

    void clear() {
        unique_lock<shared_mutex> lock(m_mtx);
        if (!m_pRoot) return;
        m_tail->setNext(nullptr); // romper el ciclo para poder borrar linealmente
        Node *curr = m_pRoot;
        while (curr) {
            Node *next = curr->getNext();
            delete curr;
            curr = next;
        }
        m_pRoot = nullptr;
        m_tail  = nullptr;
        m_size  = 0;
    }

    void push_front(value_type value, Ref ref) override {
        unique_lock<shared_mutex> lock(m_mtx);
        Node *newNode = new Node(value, ref);
        if (!m_pRoot) {
            newNode->setNext(newNode);
            m_pRoot = newNode;
            m_tail  = newNode;
        } else {
            newNode->setNext(m_pRoot);
            m_tail->setNext(newNode);
            m_pRoot = newNode;
        }
        m_size++;
    }

    void push_back(value_type value, Ref ref) override {
        unique_lock<shared_mutex> lock(m_mtx);
        Node *newNode = new Node(value, ref);
        if (!m_pRoot) {
            newNode->setNext(newNode);
            m_pRoot = newNode;
            m_tail  = newNode;
        } else {
            newNode->setNext(m_pRoot);
            m_tail->setNext(newNode);
            m_tail = newNode;
        }
        m_size++;
    }

    void insert(const value_type &value, Ref ref) override {
        unique_lock<shared_mutex> lock(m_mtx);
        internal_insert(value, ref);
    }

    std::tuple<value_type, Ref> pop_front() override {
        unique_lock<shared_mutex> lock(m_mtx);
        if (!m_pRoot) throw runtime_error("La lista esta vacia");
        Node *temp = m_pRoot;
        auto result = std::make_tuple(temp->getData(), temp->getRef());
        if (m_size == 1) {
            m_pRoot = nullptr;
            m_tail  = nullptr;
        } else {
            m_pRoot = temp->getNext();
            m_tail->setNext(m_pRoot);
        }
        delete temp;
        m_size--;
        return result;
    }

    std::tuple<value_type, Ref> pop_back() override {
        unique_lock<shared_mutex> lock(m_mtx);
        if (!m_pRoot) throw runtime_error("La lista esta vacia");
        auto result = std::make_tuple(m_tail->getData(), m_tail->getRef());
        if (m_size == 1) {
            delete m_tail;
            m_pRoot = nullptr;
            m_tail  = nullptr;
        } else {
            Node *act = m_pRoot;
            while (act->getNext() != m_tail) act = act->getNext();
            delete m_tail;
            m_tail = act;
            m_tail->setNext(m_pRoot);
        }
        m_size--;
        return result;
    }

    forward_iterator begin() { return forward_iterator(this, m_pRoot, m_pRoot); }
    forward_iterator end()   { return forward_iterator(this, nullptr, m_pRoot); }

    template <typename Func, typename... Args>
    void ForEach(Func func, Args &&... args) {
        unique_lock<shared_mutex> lock(m_mtx);
        if (m_size == 0) return;
        for (auto& item : *this) func(item, std::forward<Args>(args)...);
    }

    // Recorre N vueltas (para demostrar la naturaleza circular)
    template <typename Func, typename... Args>
    void circularForEach(size_t vueltas, Func func, Args &&... args) {
        unique_lock<shared_mutex> lock(m_mtx);
        if (!m_pRoot || vueltas == 0) return;
        Node *act = m_pRoot;
        size_t pasos = m_size * vueltas;
        for (size_t i = 0; i < pasos; ++i) {
            func(act->getDataRef(), std::forward<Args>(args)...);
            act = act->getNext();
        }
    }

    friend ostream& operator<<(ostream& os, const CircularLinkedList& list) {
        shared_lock<shared_mutex> lock(list.m_mtx);
        os << "[";
        if (list.m_pRoot) {
            Node *act = list.m_pRoot;
            do {
                os << "(" << act->getData() << "," << act->getRef() << ")";
                act = act->getNext();
                if (act != list.m_pRoot) os << ",";
            } while (act != list.m_pRoot);
        }
        os << "]";
        return os;
    }

    friend ostream& operator<<(ostream& os, CircularLinkedList& list) {
        return os << static_cast<const CircularLinkedList&>(list);
    }

    friend istream& operator>>(istream& is, CircularLinkedList& list) {
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

#endif // __CIRCULARLINKEDLIST_H__
