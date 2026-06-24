// btree.h

#ifndef BTREE_H
#define BTREE_H

#include <iostream>
#include "BTreePage.h"

#define DEFAULT_BTREE_ORDER 3

template <typename Trait>
class BTree
// this is the full version of the BTree
{
       typedef CBTreePage<Trait> BTNode;// useful shorthand

public:
       using value_type = typename Trait::value_type;
       using ObjIDType  = typename Trait::ObjIDType;
       using ObjectInfo = typename BTNode::ObjectInfo;

public:
       BTree(int order = DEFAULT_BTREE_ORDER, bool unique = true);
       ~BTree();
       bool            Insert (const value_type key, const ObjIDType ObjID);
       bool            Remove (const value_type key, const ObjIDType ObjID);
       ObjIDType       Search (const value_type key);
       long            size()  { return m_NumKeys; }
       long            height() { return m_Height;      }
       long            GetOrder() { return m_Order;     }

       void            Print (ostream &os);

       template <typename Func, typename... Args>
       void ForEach(Func func, Args&&... args)
       {
               m_Root.ForEach(func, 0, std::forward<Args>(args)...);
       }

       template <typename Func, typename... Args>
       ObjectInfo* FirstThat(Func func, Args&&... args)
       {
               return m_Root.FirstThat(func, 0, std::forward<Args>(args)...);
       }

protected:
       BTNode          m_Root;
       int             m_Height;  // height of tree
       int             m_Order;   // order of tree
       long            m_NumKeys; // number of keys
       bool            m_Unique;  // Accept the elements only once ?
};

const int MaxHeight = 5;
template <typename Trait>
BTree<Trait>::BTree(int order, bool unique)
                               : m_Root(2 * order  + 1, unique),
                                 m_Order(order),
                                 m_NumKeys(0),
                                 m_Unique(unique)
{
       m_Root.SetMaxKeysForChilds(order);
       m_Height = 1;
}

template <typename Trait>
BTree<Trait>::~BTree()
{
}

template <typename Trait>
bool BTree<Trait>::Insert(const value_type key, const ObjIDType ObjID)
{
       bt_ErrorCode error = m_Root.Insert(key, ObjID);
       if( error == bt_duplicate )
               return false;
       m_NumKeys++;
       if( error == bt_overflow )
       {
               m_Root.SplitRoot();
               m_Height++;
       }
       return true;
}

template <typename Trait>
bool BTree<Trait>::Remove (const value_type key, const ObjIDType ObjID)
{
       bt_ErrorCode error = m_Root.Remove(key, ObjID);
       if( error == bt_duplicate || error == bt_nofound )
               return false;
       m_NumKeys--;

       if( error == bt_rootmerged )
               m_Height--;
       return true;
}

template <typename Trait>
typename BTree<Trait>::ObjIDType BTree<Trait>::Search (const value_type key)
{
       ObjIDType ObjID = -1;
       m_Root.Search(key, ObjID);
       return ObjID;
}

template <typename Trait>
void BTree<Trait>::Print(ostream &os){
       m_Root.Print(os);
}

#endif
