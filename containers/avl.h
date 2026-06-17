#ifndef __AVL_H__
#define __AVL_H__

#include "BinaryTree.h"
#include <algorithm>

using namespace std;

template <typename T>
class AVLNode : public BinaryTreeNode<T, AVLNode<T>> {
public:
    using Base        = BinaryTreeNode<T, AVLNode<T>>;
    using Node        = AVLNode<T>;
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
    using height_type = typename Node::height_type;
    using branch_type = typename Node::branch_type;

protected:
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

    void rotate_unsafe(Node*& pNode, branch_type dir) {
        const branch_type other = 1 - dir;
        Node* p = pNode;
        Node* q = p->getChild(other);
        if(!q) return;

        p->setChild(other, q->getChild(dir));
        if(q->getChild(dir)) q->getChild(dir)->setParent(p);

        q->setChild(dir, p);

        q->setParent(p->getParent());
        p->setParent(q);

        pNode = q;

        update_height_unsafe(p);
        update_height_unsafe(q);
    }

    void rebalance_unsafe(Node*& pNode) {
        if(!pNode) return;
        const auto bf = balance_factor_unsafe(pNode);

        if(bf > 1) {
            if(balance_factor_unsafe(pNode->getChild(0)) < 0) {
                Node*& leftRef = pNode->getChildRef(0);
                rotate_unsafe(leftRef, 0);
            }
            rotate_unsafe(pNode, 1);
        }
        else if(bf < -1) {
            if(balance_factor_unsafe(pNode->getChild(1)) > 0) {
                Node*& rightRef = pNode->getChildRef(1);
                rotate_unsafe(rightRef, 1);
            }
            rotate_unsafe(pNode, 0);
        }
    }

    virtual Node* internal_insert_unsafe(Node*& pNode,
                                         const value_type& data,
                                         Ref ref,
                                         Node* parent) override {
        if(pNode == nullptr) {
            pNode = new Node(data, ref, parent);
            ++this->m_size;
            return pNode;
        }
        branch_type branch = this->m_comp(pNode->getData(), data) ? 1 : 0;
        Node* inserted = internal_insert_unsafe(pNode->getChildRef(branch),
                                                data, ref, pNode);

        update_height_unsafe(pNode);
        rebalance_unsafe(pNode);
        return inserted;
    }

    static void recompute_heights_unsafe(Node* n) {
        if(!n) return;
        recompute_heights_unsafe(n->getChild(0));
        recompute_heights_unsafe(n->getChild(1));
        update_height_unsafe(n);
    }

public:
    AVL() = default;

    AVL(const AVL& other) : Base(other) {
        unique_lock<shared_mutex> lock(this->m_mtx);
        recompute_heights_unsafe(this->m_pRoot);
    }

    AVL(AVL&& other) noexcept : Base(std::move(other)) {}

    AVL& operator=(const AVL& other) {
        if(this == &other) return *this;
        Base::operator=(other);
        unique_lock<shared_mutex> lock(this->m_mtx);
        recompute_heights_unsafe(this->m_pRoot);
        return *this;
    }

    AVL& operator=(AVL&& other) noexcept {
        if(this == &other) return *this;
        Base::operator=(std::move(other));
        return *this;
    }

    virtual ~AVL() = default;
};

#endif // __AVL_H__
