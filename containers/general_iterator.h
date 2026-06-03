#ifndef __ITERATOR_H__
#define __ITERATOR_H__
#include <algorithm>
#include <utility>

template <typename Container, class IteratorBase>
class general_iterator
{public:
    using Node       = typename Container::Node;
    using value_type = typename Container::value_type;
    using myself     = general_iterator<Container, IteratorBase>;
    // CRTP: el tipo derivado YA llega como IteratorBase. Exponerlo como
    // MySelf permite a los iteradores derivados omitir `using MySelf = X<C>;`
    // y heredarlo directamente (`using typename Parent::MySelf;`).
    using MySelf     = IteratorBase;

protected:
    Container *m_pContainer;
    Node      *m_pNode;
public:
    general_iterator(Container *pContainer, Node *pNode)
        : m_pContainer(pContainer), m_pNode(pNode) {}

    // Copy ctor: const&. Sin esto no se puede pasar temporales
    // (ej. return it_factory()) ni iteradores const.
    general_iterator(const myself &other)
        : m_pContainer(other.m_pContainer), m_pNode(other.m_pNode) {}

    // Move ctor
    general_iterator(myself &&other) noexcept
        : m_pContainer(other.m_pContainer), m_pNode(other.m_pNode) {
        other.m_pNode = nullptr;
    }

    // operator=: IteratorBase& (no por valor). Permite chaining
    //   it1 = it2 = it3
    // y evita una copia espuria.
    IteratorBase& operator=(const IteratorBase &iter){
        m_pContainer = iter.m_pContainer;
        m_pNode      = iter.m_pNode;
        return *static_cast<IteratorBase*>(this);
    }

    Node *getNode() const { return m_pNode; }

    // Comparación como método const. Antes era friend — funcional
    // pero requería que el operador completo viviera en la clase
    // base, lo cual es menos flexible.
    bool operator==(const IteratorBase &other) const {
        return m_pNode == other.m_pNode;
    }
    bool operator!=(const IteratorBase &other) const {
        return m_pNode != other.m_pNode;
    }

    typename Container::value_type &operator*(){
        return m_pNode->getDataRef();
    }
};

#endif
