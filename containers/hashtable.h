#ifndef __HASHTABLE_H__
#define __HASHTABLE_H__

#include <iostream>
#include <tuple>
#include <utility>
#include <mutex>
#include <shared_mutex>
#include <stdexcept>
#include "../types.h"

using namespace std;

// KVPair<Key, Value> — value_type del HashTable.
// El AVL ordena por key porque operator< mira solo key.
template <typename Key, typename Value>
class KVPair {
public:
    Key   key;
    Value value;

    KVPair() : key(), value() {}
    KVPair(const Key& k, const Value& v) : key(k), value(v) {}

    bool operator<(const KVPair& other) const { return key < other.key; }
    bool operator>(const KVPair& other) const { return key > other.key; }
    bool operator==(const KVPair& other) const { return key == other.key; }
};

template <typename K, typename V>
ostream& operator<<(ostream& os, const KVPair<K,V>& p) {
    return os << p.key << ":" << p.value;
}

template <typename K, typename V>
istream& operator>>(istream& is, KVPair<K,V>& p) {
    Token colon;
    return is >> p.key >> colon >> p.value;
}

// HashTable<Trait>.
// El Trait debe proveer:
//   - Trait::Key, Trait::Value
//   - Trait::value_type  (== KVPair<Key, Value>)
//   - Trait::Storage     (estructura subyacente — el demo decide cual)
//
// El header no menciona AVL, LinkedList, ni ningun tipo concreto. Todo
// llega via el Trait.
template <typename Trait>
class HashTable : public Trait::Storage {
public:
    using Storage    = typename Trait::Storage;
    using Key        = typename Trait::Key;
    using Value      = typename Trait::Value;
    using value_type = typename Trait::value_type;
    using Pair       = value_type;
    using Node       = typename Storage::Node;

private:
    Node* find_node_unsafe(const Key& key) const {
        Node* n = this->m_pRoot;
        while(n) {
            const Key& k = n->getData().key;
            if(key == k) return n;
            n = (key < k) ? n->getChild(0) : n->getChild(1);
        }
        return nullptr;
    }

public:
    HashTable() = default;

    Value& operator[](const Key& key) {
        unique_lock<shared_mutex> lock(this->m_mtx);
        if(Node* found = find_node_unsafe(key))
            return found->getDataRef().value;
        Node* inserted = this->internal_insert_unsafe(
            this->m_pRoot, Pair(key, Value{}), Ref{}, nullptr);
        return inserted->getDataRef().value;
    }

    const Value& at(const Key& key) const {
        shared_lock<shared_mutex> lock(this->m_mtx);
        Node* found = find_node_unsafe(key);
        if(!found) throw std::out_of_range("HashTable::at — key no existe");
        return found->getDataRef().value;
    }

    bool contains(const Key& key) const {
        shared_lock<shared_mutex> lock(this->m_mtx);
        return find_node_unsafe(key) != nullptr;
    }

    using Storage::insert;

    void insert(const Key& key, const Value& value) {
        unique_lock<shared_mutex> lock(this->m_mtx);
        Node* found = find_node_unsafe(key);
        if(found) {
            found->getDataRef().value = value;
            return;
        }
        Pair newPair(key, value);
        this->internal_insert_unsafe(this->m_pRoot, newPair, Ref{}, nullptr);
    }
};

// structured bindings: KVPair tiene campos publicos key, value — C++17
// destructura via Form C (orden de declaracion). Sin especializaciones de
// tuple_size/tuple_element/get<I> — son redundantes.

#endif // __HASHTABLE_H__
