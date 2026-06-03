#ifndef __HASHTABLE_H__
#define __HASHTABLE_H__

#include "avl.h"
#include <tuple>
#include <utility>

using namespace std;

// =====================================================================
// KVPair<Key, Value>
//
// Es el value_type que vive dentro del AVL. La comparacion solo mira
// la Key — el AVL queda ordenado por Key.
//
// Para soportar `for (const auto& [k, v] : table)` (structured bindings)
// se especializan tuple_size, tuple_element y get<I> al final del archivo
// dentro de `namespace std`.
// =====================================================================
template <typename Key, typename Value>
class KVPair {
public:
    Key   m_key;
    Value m_value;

    KVPair() : m_key(), m_value() {}
    KVPair(const Key& k, const Value& v) : m_key(k), m_value(v) {}

    // Solo compara por key — esto es lo que hace que el AVL se
    // ordene por la key.
    bool operator<(const KVPair& other) const { return m_key < other.m_key; }
    bool operator>(const KVPair& other) const { return m_key > other.m_key; }
    bool operator==(const KVPair& other) const { return m_key == other.m_key; }
};

// Para que operator<< de container_write imprima (k:v) en lugar de
// solo el key. NO se usa por container_write actual — pero los demos
// que imprimen KVPair directamente se ven mejor.
template <typename K, typename V>
ostream& operator<<(ostream& os, const KVPair<K,V>& p) {
    return os << p.m_key << ":" << p.m_value;
}

template <typename K, typename V>
istream& operator>>(istream& is, KVPair<K,V>& p) {
    char colon;
    return is >> p.m_key >> colon >> p.m_value;
}

// =====================================================================
// HashTable<Key, Value>
//
// Hereda AVL< AscendingAVLTrait< KVPair<Key,Value> > >.
//
// La operacion clave es operator[](key) — busca; si existe devuelve
// ref al value; si no, inserta KVPair(key, Value{}) y devuelve ref.
//
// Decisión: "HashTable sobre AVL" se interpreta como — el AVL ES el
// contenedor; no hay array de buckets ni funcion hash. Trade-off: el
// nombre "HashTable" es semanticamente un AVLMap. Beneficio: cero
// duplicacion, hereda balanceo + concurrencia + Big Five de AVL.
// =====================================================================
template <typename Key, typename Value>
class HashTable : public AVL< AscendingAVLTrait< KVPair<Key, Value> > > {
public:
    using Pair       = KVPair<Key, Value>;
    using Trait      = AscendingAVLTrait<Pair>;
    using Base       = AVL<Trait>;
    using Node       = typename Base::Node;
    using value_type = Pair;

private:
    // Busca el nodo con la key dada (asume lock externo).
    Node* find_node_unsafe(const Key& key) const {
        Node* n = this->m_pRoot;
        while(n) {
            const Key& k = n->getData().m_key;
            if(key == k) return n;
            n = (key < k) ? n->getChild(0) : n->getChild(1);
        }
        return nullptr;
    }

public:
    HashTable() = default;
    // Big Five heredado de Base — funciona porque AVL solo agrega
    // m_height a los nodos y todo lo demas se copia/mueve via Base.

    // operator[](key) — find-or-insert en una sola pasada.
    // internal_insert_unsafe ahora retorna el Node* del nodo creado;
    // las rotaciones del AVL no invalidan el puntero (reordenan
    // referencias, no objetos).
    Value& operator[](const Key& key) {
        unique_lock<shared_mutex> lock(this->m_mtx);
        if(Node* found = find_node_unsafe(key))
            return found->getDataRef().m_value;
        Node* inserted = this->internal_insert_unsafe(
            this->m_pRoot, Pair(key, Value{}), Ref{}, nullptr);
        return inserted->getDataRef().m_value;
    }

    // get const — tira out_of_range si no esta
    const Value& at(const Key& key) const {
        shared_lock<shared_mutex> lock(this->m_mtx);
        Node* found = find_node_unsafe(key);
        if(!found) throw std::out_of_range("HashTable::at — key no existe");
        return found->getDataRef().m_value;
    }

    bool contains(const Key& key) const {
        shared_lock<shared_mutex> lock(this->m_mtx);
        return find_node_unsafe(key) != nullptr;
    }

    // insert(key, value) — sobrescribe si existe.
    // Trae el insert(value, Ref) del Base AVL al scope publico — evita el
    // warning de Woverloaded-virtual al introducir nuestra sobrecarga
    // insert(Key, Value).
    using Base::insert;

    void insert(const Key& key, const Value& value) {
        unique_lock<shared_mutex> lock(this->m_mtx);
        Node* found = find_node_unsafe(key);
        if(found) {
            found->getDataRef().m_value = value;
            return;
        }
        Pair newPair(key, value);
        this->internal_insert_unsafe(this->m_pRoot, newPair, Ref{}, nullptr);
    }
};

// =====================================================================
// Especializaciones para structured bindings:
//   for (const auto& [k, v] : table)
//
// Tienen que vivir DENTRO de namespace std. Es la unica forma soportada
// por C++17+.
// =====================================================================
namespace std {

template <typename K, typename V>
struct tuple_size< ::KVPair<K, V> > : std::integral_constant<size_t, 2> {};

template <typename K, typename V>
struct tuple_element<0, ::KVPair<K, V>> { using type = K; };

template <typename K, typename V>
struct tuple_element<1, ::KVPair<K, V>> { using type = V; };

} // namespace std

template <size_t I, typename K, typename V>
auto& get(KVPair<K, V>& p) {
    if constexpr (I == 0) return p.m_key;
    else                  return p.m_value;
}

template <size_t I, typename K, typename V>
const auto& get(const KVPair<K, V>& p) {
    if constexpr (I == 0) return p.m_key;
    else                  return p.m_value;
}

#endif // __HASHTABLE_H__
