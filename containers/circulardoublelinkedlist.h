#ifndef __CIRCULARDOUBLELINKEDLIST_H__
#define __CIRCULARDOUBLELINKEDLIST_H__

#include "doublelinkedlist.h"

// Reuso DLLNode (ya tiene prev/next). Solo cambio: tail->next=root, root->prev=tail.
template <typename T>
struct AscendingCDLLTrait : BaseTrait<T, less<T>> {
    using Node = DLLNode<T>;
};

template <typename T>
struct DescendingCDLLTrait : BaseTrait<T, greater<T>> {
    using Node = DLLNode<T>;
};

// Iteradores con centinela: corta cuando el avance vuelve al m_start.
template <typename Container>
class CDLLForwardIterator
    : public general_iterator<Container, CDLLForwardIterator<Container>> {
public:
    using MySelf = CDLLForwardIterator<Container>;
    using Parent = general_iterator<Container, MySelf>;
    using Node   = typename Container::Node;
private:
    Node *m_start;
public:
    CDLLForwardIterator(Container *pCont, Node *pNode)
        : Parent(pCont, pNode), m_start(pNode) {}
    CDLLForwardIterator(Container *pCont, Node *pNode, bool /*sentinel*/)
        : Parent(pCont, pNode), m_start(nullptr) {}

    MySelf operator++() {
        if (this->m_pNode) {
            this->m_pNode = this->m_pNode->getNext();
            if (this->m_pNode == m_start) this->m_pNode = nullptr;
        }
        return *this;
    }
};

template <typename Container>
class CDLLBackwardIterator
    : public general_iterator<Container, CDLLBackwardIterator<Container>> {
public:
    using MySelf = CDLLBackwardIterator<Container>;
    using Parent = general_iterator<Container, MySelf>;
    using Node   = typename Container::Node;
private:
    Node *m_start;
public:
    CDLLBackwardIterator(Container *pCont, Node *pNode)
        : Parent(pCont, pNode), m_start(pNode) {}
    CDLLBackwardIterator(Container *pCont, Node *pNode, bool /*sentinel*/)
        : Parent(pCont, pNode), m_start(nullptr) {}

    MySelf operator++() {
        if (this->m_pNode) {
            this->m_pNode = this->m_pNode->getPrev();
            if (this->m_pNode == m_start) this->m_pNode = nullptr;
        }
        return *this;
    }
};

template <typename Trait>
class CircularDoubleLinkedList : public LinkedList<Trait> {
public:
    using value_type        = typename Trait::value_type;
    using Node              = typename Trait::Node;
    using Comp              = typename Trait::Comp;
    using MySelf            = CircularDoubleLinkedList<Trait>;
    using forward_iterator  = CDLLForwardIterator<MySelf>;
    using backward_iterator = CDLLBackwardIterator<MySelf>;
    friend forward_iterator;
    friend backward_iterator;

private:
    Node  *m_pRoot = nullptr;
    Node  *m_tail  = nullptr;
    size_t m_size  = 0;
    Comp   m_comp;
    mutable shared_mutex m_mtx;

    void internal_insert(const value_type &value, Ref ref) {
        Node *newNode = new Node(value, ref);
        m_size++;
        if (!m_pRoot) {
            // Círculo de uno: se apunta a sí mismo en ambas direcciones.
            newNode->setNext(newNode);
            newNode->setPrev(newNode);
            m_pRoot = newNode;
            m_tail  = newNode;
            return;
        }
        if (m_comp(value, m_pRoot->getDataRef())) {
            // Nuevo head: cierre con tail en ambos sentidos.
            newNode->setNext(m_pRoot);
            newNode->setPrev(m_tail);
            m_pRoot->setPrev(newNode);
            m_tail->setNext(newNode);
            m_pRoot = newNode;
            return;
        }
        // Slot interno.
        Node *act = m_pRoot;
        while (act->getNext() != m_pRoot && !m_comp(value, act->getNext()->getDataRef()))
            act = act->getNext();
        Node *following = act->getNext();
        newNode->setNext(following);
        newNode->setPrev(act);
        act->setNext(newNode);
        following->setPrev(newNode);
        if (act == m_tail) m_tail = newNode;
    }

public:
    CircularDoubleLinkedList() : LinkedList<Trait>() {}

    CircularDoubleLinkedList(const CircularDoubleLinkedList &other)
        : LinkedList<Trait>(), m_pRoot(nullptr), m_tail(nullptr), m_size(0) {
        shared_lock<shared_mutex> lock(other.m_mtx);
        if (!other.m_pRoot) return;
        Node *curr = other.m_pRoot;
        do {
            push_back(curr->getData(), curr->getRef());
            curr = curr->getNext();
        } while (curr != other.m_pRoot);
    }

    CircularDoubleLinkedList(CircularDoubleLinkedList &&other)
        : LinkedList<Trait>(), m_pRoot(nullptr), m_tail(nullptr), m_size(0) {
        unique_lock<shared_mutex> lock(other.m_mtx);
        m_pRoot = std::exchange(other.m_pRoot, nullptr);
        m_tail  = std::exchange(other.m_tail,  nullptr);
        m_size  = std::exchange(other.m_size,  0);
    }

    CircularDoubleLinkedList& operator=(const CircularDoubleLinkedList &other) {
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

    CircularDoubleLinkedList& operator=(CircularDoubleLinkedList &&other) {
        if (this != &other) {
            clear();
            unique_lock<shared_mutex> lock(other.m_mtx);
            m_pRoot = std::exchange(other.m_pRoot, nullptr);
            m_tail  = std::exchange(other.m_tail,  nullptr);
            m_size  = std::exchange(other.m_size,  0);
        }
        return *this;
    }

    virtual ~CircularDoubleLinkedList() { clear(); }

    void clear() {
        unique_lock<shared_mutex> lock(m_mtx);
        if (!m_pRoot) return;
        m_tail->setNext(nullptr);  // rompo el círculo para borrar lineal
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
            newNode->setPrev(newNode);
            m_pRoot = newNode;
            m_tail  = newNode;
        } else {
            newNode->setNext(m_pRoot);
            newNode->setPrev(m_tail);
            m_pRoot->setPrev(newNode);
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
            newNode->setPrev(newNode);
            m_pRoot = newNode;
            m_tail  = newNode;
        } else {
            newNode->setPrev(m_tail);
            newNode->setNext(m_pRoot);
            m_tail->setNext(newNode);
            m_pRoot->setPrev(newNode);
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
        Node *temp  = m_pRoot;
        auto result = std::make_tuple(temp->getData(), temp->getRef());
        if (m_size == 1) {
            m_pRoot = nullptr;
            m_tail  = nullptr;
        } else {
            m_pRoot = temp->getNext();
            m_pRoot->setPrev(m_tail);
            m_tail->setNext(m_pRoot);
        }
        delete temp;
        m_size--;
        return result;
    }

    std::tuple<value_type, Ref> pop_back() override {
        unique_lock<shared_mutex> lock(m_mtx);
        if (!m_pRoot) throw runtime_error("La lista esta vacia");
        Node *temp  = m_tail;
        auto result = std::make_tuple(temp->getData(), temp->getRef());
        if (m_size == 1) {
            m_pRoot = nullptr;
            m_tail  = nullptr;
        } else {
            m_tail = temp->getPrev();
            m_tail->setNext(m_pRoot);
            m_pRoot->setPrev(m_tail);
        }
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
    forward_iterator  end()    { return forward_iterator (this, nullptr, true); }
    backward_iterator rbegin() { return backward_iterator(this, m_tail);  }
    backward_iterator rend()   { return backward_iterator(this, nullptr, true); }

    template <typename Func, typename... Args>
    void ForEach(Func func, Args&&... args) {
        shared_lock<shared_mutex> lock(m_mtx);
        if (!m_pRoot) return;
        Node *act = m_pRoot;
        do {
            func(act->getDataRef(), std::forward<Args>(args)...);
            act = act->getNext();
        } while (act != m_pRoot);
    }

    template <typename Func, typename... Args>
    void circularForEach(size_t vueltas, Func func, Args&&... args) {
        shared_lock<shared_mutex> lock(m_mtx);
        if (!m_pRoot || vueltas == 0) return;
        Node  *act   = m_pRoot;
        size_t pasos = m_size * vueltas;
        for (size_t i = 0; i < pasos; ++i) {
            func(act->getDataRef(), std::forward<Args>(args)...);
            act = act->getNext();
        }
    }

    // Idem en sentido inverso — prueba que prev también cierra el círculo.
    template <typename Func, typename... Args>
    void circularReverseForEach(size_t vueltas, Func func, Args&&... args) {
        shared_lock<shared_mutex> lock(m_mtx);
        if (!m_pRoot || vueltas == 0) return;
        Node  *act   = m_tail;
        size_t pasos = m_size * vueltas;
        for (size_t i = 0; i < pasos; ++i) {
            func(act->getDataRef(), std::forward<Args>(args)...);
            act = act->getPrev();
        }
    }

    friend ostream& operator<<(ostream &os, const CircularDoubleLinkedList &list) {
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
        if (list.m_pRoot)
            os << " ->root(" << list.m_pRoot->getData()
               << ") tail(" << list.m_tail->getData() << ")";
        return os;
    }

    friend istream& operator>>(istream &is, CircularDoubleLinkedList &list) {
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

#endif // __CIRCULARDOUBLELINKEDLIST_H__
