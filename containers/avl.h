#ifndef __AVL_H__
#define __AVL_H__

#include "BinaryTree.h"
#include <algorithm>

using namespace std;

// AVLNode hereda BinaryTreeNode usando el patron CRTP-en-Node:
// BinaryTreeNode<T, AVLNode<T>> hace que m_pChild[] sea AVLNode*
// sin necesidad de casteo.
template <typename T>
class AVLNode : public BinaryTreeNode<T, AVLNode<T>> {
public:
    using Base        = BinaryTreeNode<T, AVLNode<T>>;
    using Node        = AVLNode<T>;
    // Alias publico: cualquier capa que use heights del AVL toma el tipo
    // de aca en vez de un nativo. signed permite restas de balance sin wrap.
    using height_type = int;

private:
    height_type m_height = 1;

public:
    AVLNode(T data = T(), Ref ref = Ref(), Node *parent = nullptr)
        : Base(data, ref, parent), m_height(1) {}

    height_type getHeight() const          { return m_height; }
    void        setHeight(height_type h)   { m_height = h; }
};

// Traits del AVL
template <typename T>
struct AscendingAVLTrait : public BaseTrait<AVLNode<T>, std::less<T>> {};

template <typename T>
struct DescendingAVLTrait : public BaseTrait<AVLNode<T>, std::greater<T>> {};

template <typename Trait>
class AVL : public BinaryTree<Trait> {
public:
    using Base        = BinaryTree<Trait>;
    using value_type  = typename Trait::value_type;
    using Node        = typename Trait::Node;
    using Comp        = typename Trait::Comp;
    using MySelf      = AVL<Trait>;
    // Reutiliza el alias del Node — el tipo de altura es del dominio del nodo,
    // no del container. Cambiar la base en AVLNode reverbera aca sin tocar nada.
    using height_type = typename Node::height_type;

protected:
    // height_type es signed, las restas de balance no wrappean.
    static height_type height_unsafe(const Node* n) {
        return n ? n->getHeight() : 0;
    }

    static height_type balance_factor_unsafe(const Node* n) {
        if(!n) return 0;
        return height_unsafe(n->getChild(0)) - height_unsafe(n->getChild(1));
    }

    static void update_height_unsafe(Node* n) {
        if(!n) return;
        height_type hl = height_unsafe(n->getChild(0));
        height_type hr = height_unsafe(n->getChild(1));
        n->setHeight(1 + std::max(hl, hr));
    }

    // rotate_unsafe(pNode, dir):
    //   dir = 0  ->  rotate-left
    //   dir = 1  ->  rotate-right
    //
    // pNode es Node*& (referencia al puntero que el padre tiene hacia este
    // subarbol). Reasignar pNode hace que el padre apunte al nuevo root.
    //
    // Rotacion generica unica para LL/RR — dir parametriza la direccion.
    void rotate_unsafe(Node*& pNode, int dir) {
        const int other = 1 - dir;
        Node* p = pNode;
        Node* q = p->getChild(other);     // el que sube
        if(!q) return;                    // sanity

        // p->other  <-  q->dir
        p->setChild(other, q->getChild(dir));
        if(q->getChild(dir)) q->getChild(dir)->setParent(p);

        // q->dir  <-  p
        q->setChild(dir, p);

        // q toma el parent que tenia p
        q->setParent(p->getParent());
        p->setParent(q);

        // El padre (a traves de pNode) ahora apunta a q
        pNode = q;

        // Actualizar alturas: primero p (hijo), despues q (padre)
        update_height_unsafe(p);
        update_height_unsafe(q);
    }

    void rebalance_unsafe(Node*& pNode) {
        if(!pNode) return;
        const auto bf = balance_factor_unsafe(pNode);

        // Pesado a la izquierda
        if(bf > 1) {
            if(balance_factor_unsafe(pNode->getChild(0)) < 0) {
                // Caso LR: primero rotate-left en el hijo izquierdo
                Node*& leftRef = pNode->getChildRef(0);
                rotate_unsafe(leftRef, 0);
            }
            // Caso LL (o LR despues del paso anterior): rotate-right en pNode
            rotate_unsafe(pNode, 1);
        }
        // Pesado a la derecha
        else if(bf < -1) {
            if(balance_factor_unsafe(pNode->getChild(1)) > 0) {
                // Caso RL: primero rotate-right en el hijo derecho
                Node*& rightRef = pNode->getChildRef(1);
                rotate_unsafe(rightRef, 1);
            }
            // Caso RR: rotate-left en pNode
            rotate_unsafe(pNode, 0);
        }
    }

    // Override del internal_insert del BinaryTree.
    // Despues de la insercion recursiva, actualiza altura y rebalancea.
    // Las rotaciones NO mueven el nodo recien creado en memoria, solo
    // reordenan punteros — el Node* retornado sigue siendo valido aunque
    // su posicion en el arbol haya cambiado.
    virtual Node* internal_insert_unsafe(Node*& pNode,
                                         const value_type& data,
                                         Ref ref,
                                         Node* parent) override {
        if(pNode == nullptr) {
            pNode = new Node(data, ref, parent);
            ++this->m_size;
            return pNode;
        }
        int branch = this->m_comp(pNode->getData(), data) ? 1 : 0;
        Node* inserted = internal_insert_unsafe(pNode->getChildRef(branch),
                                                data, ref, pNode);

        update_height_unsafe(pNode);
        rebalance_unsafe(pNode);
        return inserted;
    }

    // Post-order: hijos primero, luego update_height(n). Tras copiar un
    // AVL via Base, todos los AVLNode quedan con m_height=1 (el default
    // del ctor de AVLNode). Hay que recalcular bottom-up para restaurar
    // el invariante de altura.
    static void recompute_heights_unsafe(Node* n) {
        if(!n) return;
        recompute_heights_unsafe(n->getChild(0));
        recompute_heights_unsafe(n->getChild(1));
        update_height_unsafe(n);
    }

public:
    AVL() = default;

    // Override del Big Five copy/move: delega al Base para clonar la
    // estructura, despues recalcula heights. Sin esto, una copia de un
    // AVL no-trivial queda con alturas inconsistentes hasta el primer
    // insert que las repare.
    AVL(const AVL& other) : Base(static_cast<const Base&>(other)) {
        unique_lock<shared_mutex> lock(this->m_mtx);
        recompute_heights_unsafe(this->m_pRoot);
    }

    AVL(AVL&& other) noexcept : Base(static_cast<Base&&>(other)) {
        // Move conserva los AVLNode originales con sus heights intactas.
        // No hace falta recompute.
    }

    AVL& operator=(const AVL& other) {
        if(this == &other) return *this;
        Base::operator=(static_cast<const Base&>(other));
        unique_lock<shared_mutex> lock(this->m_mtx);
        recompute_heights_unsafe(this->m_pRoot);
        return *this;
    }

    AVL& operator=(AVL&& other) noexcept {
        if(this == &other) return *this;
        Base::operator=(static_cast<Base&&>(other));
        return *this;
    }

    virtual ~AVL() = default;
};

#endif // __AVL_H__
