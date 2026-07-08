// btree.h

#ifndef BTREE_H
#define BTREE_H

#include <iostream>
#include <vector>
#include <utility>
#include <functional>
#include <type_traits>
#include <mutex>
#include <shared_mutex>
#include "BTreePage.h"

#define DEFAULT_BTREE_ORDER 3

template <typename PageType, Bool IsForward>
class btree_iterator
{
       using ObjectInfo = typename PageType::ObjectInfo;
       vector<pair<PageType *, size_t>> m_stack;

       void Descend(PageType *p)
       {
               while( p && p->m_KeyCount > 0 )
               {
                       size_t i = IsForward ? 0 : p->m_KeyCount - 1;
                       m_stack.push_back({p, i});
                       p = IsForward ? p->m_SubPages[0] : p->m_SubPages[p->m_KeyCount];
               }
       }
public:
       btree_iterator() {}
       btree_iterator(PageType *root, Bool atEnd)
       {
               if( !atEnd && root && root->m_KeyCount > 0 )
                       Descend(root);
       }
       ObjectInfo& operator*()  { return m_stack.back().first->m_Keys[m_stack.back().second]; }
       ObjectInfo* operator->() { return &(**this); }
       Bool operator!=(const btree_iterator &o) const
       {
               if( m_stack.empty() || o.m_stack.empty() )
                       return m_stack.size() != o.m_stack.size();
               return m_stack.back() != o.m_stack.back();
       }
       Bool operator==(const btree_iterator &o) const { return !(*this != o); }
       btree_iterator& operator++()
       {
               if( m_stack.empty() )
                       return *this;
               PageType *page = m_stack.back().first;
               size_t    i    = m_stack.back().second;
               PageType *child = IsForward ? page->m_SubPages[i+1] : page->m_SubPages[i];
               m_stack.back().second = IsForward ? i+1 : i-1;
               if( child )
                       Descend(child);
               else
                       while( !m_stack.empty() &&
                              m_stack.back().second >= m_stack.back().first->m_KeyCount )
                               m_stack.pop_back();
               return *this;
       }
};

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
       BTree(size_t order = DEFAULT_BTREE_ORDER, Bool unique = true);
       ~BTree();
       Bool            Insert (const value_type key, const ObjIDType ObjID);
       Bool            Remove (const value_type key, const ObjIDType ObjID);
       ObjIDType       Search (const value_type key);
       size_t          size()  { return m_NumKeys; }
       size_t          height() { return m_Height;      }
       size_t          GetOrder() { return m_Order;     }

       void            Print (ostream &os);

       using forward_iterator  = btree_iterator<BTNode, true>;
       using backward_iterator = btree_iterator<BTNode, false>;
       forward_iterator  begin()  { return forward_iterator(&m_Root, false); }
       forward_iterator  end()    { return forward_iterator(); }
       backward_iterator rbegin() { return backward_iterator(&m_Root, false); }
       backward_iterator rend()   { return backward_iterator(); }

       template <typename It, typename Func, typename... Args>
       ObjectInfo* traverse(It it, It fin, Func func, Args&&... args)
       {
               for( ; it != fin; ++it )
               {
                       if constexpr( is_void_v<invoke_result_t<Func, ObjectInfo&, Args...>> )
                               invoke(func, *it, args...);
                       else if( invoke(func, *it, args...) )
                               return &(*it);
               }
               return nullptr;
       }

       template <typename Func, typename... Args>
       void ForEach(Func func, Args&&... args)
       {
               shared_lock<shared_mutex> lock(m_mtx);
               traverse(begin(), end(), func, std::forward<Args>(args)...);
       }

       template <typename Func, typename... Args>
       ObjectInfo* FirstThat(Func func, Args&&... args)
       {
               shared_lock<shared_mutex> lock(m_mtx);
               return traverse(begin(), end(), func, std::forward<Args>(args)...);
       }

       template <typename Func, typename... Args>
       void ReverseForEach(Func func, Args&&... args)
       {
               shared_lock<shared_mutex> lock(m_mtx);
               traverse(rbegin(), rend(), func, std::forward<Args>(args)...);
       }

       friend ostream& operator<<(ostream &os, BTree &bt)
       {
               bt.ForEach([&os](ObjectInfo &info){ os << "(" << info.key << "," << info.ObjID << ")"; });
               return os;
       }

       friend istream& operator>>(istream &is, BTree &bt)
       {
               value_type key; ObjIDType id; char ch;
               while( is >> ch && ch == '(' )
               {
                       is >> key >> ch >> id >> ch;
                       bt.Insert(key, id);
               }
               return is;
       }

protected:
       BTNode          m_Root;
       size_t          m_Height;  // height of tree
       size_t          m_Order;   // order of tree
       size_t          m_NumKeys; // number of keys
       Bool            m_Unique;  // Accept the elements only once ?
       mutable shared_mutex m_mtx;
};

const size_t MaxHeight = 5;
template <typename Trait>
BTree<Trait>::BTree(size_t order, Bool unique)
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
Bool BTree<Trait>::Insert(const value_type key, const ObjIDType ObjID)
{
       unique_lock<shared_mutex> lock(m_mtx);
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
Bool BTree<Trait>::Remove (const value_type key, const ObjIDType ObjID)
{
       unique_lock<shared_mutex> lock(m_mtx);
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
       shared_lock<shared_mutex> lock(m_mtx);
       ObjIDType ObjID = ObjIDType(-1);
       m_Root.Search(key, ObjID);
       return ObjID;
}

template <typename Trait>
void BTree<Trait>::Print(ostream &os){
       ForEach([&os](ObjectInfo &info){ os << info.key << "->" << info.ObjID << "\n"; });
}

#endif
