#ifndef __BINARYTREE_AVL_H__
#define __BINARYTREE_AVL_H__

#include <algorithm>
#include <shared_mutex>
#include <utility>
#include "BinaryTree.h"
using namespace std;

// AVLNode extiende BinaryTreeNodeBase agregando altura.
// Hereda con CRTP para que m_pChild[] sea AVLNode* (no BinaryTreeNode*).
template<typename T>
struct AVLNode : BinaryTreeNodeBase<AVLNode<T>, T> {
    int m_height;
    AVLNode(T data) : BinaryTreeNodeBase<AVLNode<T>, T>(data), m_height(1) {}
};

template<typename T> using AscendingAVLTrait  = AscendingTrait<AVLNode<T>>;
template<typename T> using DescendingAVLTrait = DescendingTrait<AVLNode<T>>;

template<typename Trait>
class BinaryTreeAVL : public BinaryTree<Trait> {
public:
    using Base       = BinaryTree<Trait>;
    using Node       = typename Base::Node;
    using value_type = typename Base::value_type;

    using Base::Base;

    void insert(value_type data) override {
        unique_lock lock(this->m_mtx);
        this->m_pRoot = avl_insert(this->m_pRoot, data);
    }

private:
    int node_height(Node* n) const { return n ? n->m_height : 0; }

    int balance(Node* n) const {
        return n ? node_height(n->m_pChild[0]) - node_height(n->m_pChild[1]) : 0;
    }

    void update_height(Node* n) {
        if (n) n->m_height = 1 + max(node_height(n->m_pChild[0]),
                                     node_height(n->m_pChild[1]));
    }

    // Rotación unificada: n=0 sube el hijo izquierdo (rotate_right),
    //                    n=1 sube el hijo derecho  (rotate_left).
    // Los dos casos son simétricos — uso 1-n para el subárbol que migra.
    Node* rotate(Node* pNode, int n) {
        Node* pChild = pNode->m_pChild[n];
        Node* T2     = pChild->m_pChild[1-n];
        pChild->m_pChild[1-n] = pNode;
        pNode->m_pChild[n]    = T2;
        update_height(pNode);
        update_height(pChild);
        return pChild;
    }

    // Adapta el insert del BinaryTree agregando altura + rotaciones.
    Node* avl_insert(Node* node, value_type data) {
        if (!node) { ++this->m_size; return new Node(data); }

        auto branch = !this->m_comp(data, node->m_data);
        node->m_pChild[branch] = avl_insert(node->m_pChild[branch], data);
        update_height(node);

        auto bf = balance(node);

        // Left-Left  → data < m_pChild[0]  → rotación derecha simple (n=0)
        if (bf > 1  &&  this->m_comp(data, node->m_pChild[0]->m_data))
            return rotate(node, 0);
        // Right-Right → data >= m_pChild[1] → rotación izquierda simple (n=1)
        if (bf < -1 && !this->m_comp(data, node->m_pChild[1]->m_data))
            return rotate(node, 1);
        // Left-Right → izq sobre hijo izq + der sobre nodo
        if (bf > 1  && !this->m_comp(data, node->m_pChild[0]->m_data)) {
            node->m_pChild[0] = rotate(node->m_pChild[0], 1);
            return rotate(node, 0);
        }
        // Right-Left → der sobre hijo der + izq sobre nodo
        if (bf < -1 &&  this->m_comp(data, node->m_pChild[1]->m_data)) {
            node->m_pChild[1] = rotate(node->m_pChild[1], 0);
            return rotate(node, 1);
        }
        return node;
    }
};

#endif // __BINARYTREE_AVL_H__
