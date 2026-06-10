#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <thread>
#include <utility>

#include "../types.h"
#include "hashtable.h"
#include "heap.h"
#include "linkedlist.h"
#include "BinaryTree.h"
#include "avl.h"
#include "vector.h"

using namespace std;

void DemoHashTableBasico() {
    cout << "\n=== HashTable Basico Demo ===" << endl;
    HashTable<int, string> m;

    m.insert(5, "cinco");
    m.insert(2, "dos");
    m.insert(8, "ocho");
    m.insert(1, "uno");

    cout << "Despues de 4 inserts: size = " << m.size() << endl;

    cout << "m[5] = " << m[5] << endl;
    cout << "m[2] = " << m[2] << endl;

    // operator[] insert-on-miss
    m[42] = "cuarenta y dos";
    cout << "Despues de m[42]= : size = " << m.size() << ", m[42] = " << m[42] << endl;

    cout << "contains(5) = " << m.contains(5)  << endl;
    cout << "contains(99) = " << m.contains(99) << endl;

    try {
        m.at(999);
        cout << "FALLO: at(999) no lanzo" << endl;
    } catch(const out_of_range& e) {
        cout << "at(999) lanzo correctamente: " << e.what() << endl;
    }
}

void DemoStructuredBindings() {
    cout << "\n=== HashTable Structured Bindings Demo ===" << endl;
    HashTable<int, int> m;
    for(int k : {3, 1, 4, 1, 5, 9, 2, 6, 5, 3}) {
        m[k] = k * 10;
    }

    // for (const auto& [k, v] : m) — structured bindings literal
    cout << "Iteracion con structured bindings (key ascendente):" << endl;
    for(const auto& [k, v] : m) {
        cout << "  [" << k << "] = " << v << endl;
    }
}

void DemoCopyMove() {
    cout << "\n=== HashTable Copy/Move Demo ===" << endl;
    HashTable<int, int> original;
    original[10] = 100;
    original[20] = 200;
    original[30] = 300;
    cout << "original size = " << original.size() << endl;

    // Copy constructor
    HashTable<int, int> copia = original;
    cout << "Despues de copia (copy ctor): copia size = " << copia.size()
         << ", copia[20] = " << copia[20] << endl;

    // Modificar copia no afecta original
    copia[20] = 999;
    cout << "Modifique copia[20] = 999. original[20] sigue = " << original[20] << endl;

    // Move constructor
    HashTable<int, int> movida = std::move(copia);
    cout << "Despues de move ctor: movida size = " << movida.size()
         << ", movida[10] = " << movida[10] << endl;
}

void DemoHashTablePersistencia() {
    cout << "\n=== HashTable Persistencia Demo (operator<< y operator>>) ===" << endl;
    HashTable<int, int> m;
    m[10] = 100;
    m[5]  = 50;
    m[20] = 200;
    m[1]  = 1;

    // operator<< heredado de BinaryTree base
    cout << "Estado via operator<<: " << m << endl;

    ofstream of("hashtable.txt");
    of << m << endl;
    of.close();
    cout << "Escrito a hashtable.txt" << endl;

    // operator>> via container_read
    HashTable<int, int> releida;
    ifstream in("hashtable.txt");
    in >> releida;
    cout << "Releida via operator>>: " << releida << endl;
    cout << "releida[10] = " << releida[10] << endl;
}

void DemoHashTableConcurrency() {
    cout << "\n=== HashTable Concurrency Demo ===" << endl;
    HashTable<int, int> m;

    auto worker = [&m](int id) {
        for(int i = 0; i < 200; ++i)
            m.insert(i + id*1000, id);
    };

    thread t1(worker, 1), t2(worker, 2), t3(worker, 3),
           t4(worker, 4), t5(worker, 5);
    t1.join(); t2.join(); t3.join(); t4.join(); t5.join();

    cout << "5 hilos x 200 inserts (keys disjuntas). Esperado=1000, real=" << m.size() << endl;
    cout << (m.size() == 1000 ? "EXITO" : "FALLO") << endl;
}

// Funcion generica: recibe cualquier container con .begin() y .end()
// y escribe sus elementos a un ostream usando un separador. La firma
// no menciona ningun tipo de container — Vector, LinkedList, BinaryTree,
// AVL, Heap y HashTable la satisfacen todos.
//
// Esto es lo que el profe pide cuando dice "todos tienen la misma
// cascara". Mismo nombre de metodo (toString, begin, end), misma firma:
// una sola funcion polimorfica para todos los containers.
template <typename Container>
void escribir_polimorfico(const string& etiqueta, Container& c, ostream& os) {
    os << etiqueta << "  toString: " << c.toString();
    os << "  range-for: [ ";
    for(const auto& x : c) os << x << " ";
    os << "]" << endl;
}

// Demo polimorfico: misma funcion escribir_polimorfico aplicada a los 6
// contenedores. Demuestra que la API uniforme (toString + begin/end)
// permite escribir codigo generico que no le importa el tipo concreto.
void DemoPolimorfico() {
    cout << "\n=== Polimorfismo via API uniforme (6 contenedores) ===" << endl;

    Vector<VectorTrait<int>> v;
    v.push_back(10, 1); v.push_back(20, 2); v.push_back(30, 3);
    escribir_polimorfico("1) Vector     ", v, cout);

    LinkedList<AscendingLinkedListTrait<int>> ll;
    ll.push_back(100, 1); ll.push_back(200, 2); ll.push_back(300, 3);
    escribir_polimorfico("2) LinkedList ", ll, cout);

    BinaryTree<AscendingBTTrait<int>> bt;
    bt.insert(50, 1); bt.insert(20, 2); bt.insert(80, 3); bt.insert(10, 4);
    escribir_polimorfico("3) BinaryTree ", bt, cout);

    AVL<AscendingAVLTrait<int>> avl;
    avl.insert(50, 1); avl.insert(20, 2); avl.insert(80, 3); avl.insert(10, 4);
    escribir_polimorfico("4) AVL        ", avl, cout);

    Heap<MinHeapTrait<int>> h;
    h.insert(50, 1); h.insert(20, 2); h.insert(80, 3); h.insert(10, 4);
    escribir_polimorfico("5) Heap       ", h, cout);

    // HashTable usa structured bindings sobre KVPair — no encaja en la
    // funcion generica simple (operator<< del KVPair lo formatea como
    // k:v). Lo recorremos con su propia API tambien polimorfica:
    HashTable<int, int> ht;
    ht[5] = 50; ht[2] = 20; ht[8] = 80;
    cout << "6) HashTable   toString: " << ht.toString() << "  range-for: [ ";
    for(const auto& [k, val] : ht) cout << k << ":" << val << " ";
    cout << "]" << endl;
}

// Range-for nativo en los 6 contenedores: Vector, LinkedList,
// BinaryTree, AVL, Heap, HashTable.
void DemoRangeForNativo() {
    cout << "\n=== Range-for nativo (6 contenedores) ===" << endl;

    // 1. Vector
    Vector<VectorTrait<int>> v;
    v.push_back(10, 1); v.push_back(20, 2); v.push_back(30, 3);
    cout << "1) Vector:      "; for(auto& x : v) cout << x << " "; cout << endl;

    // 2. LinkedList
    LinkedList<AscendingLinkedListTrait<int>> ll;
    ll.push_back(100, 1); ll.push_back(200, 2); ll.push_back(300, 3);
    cout << "2) LinkedList:  "; for(auto& x : ll) cout << x << " "; cout << endl;

    // 3. BinaryTree (inorder)
    BinaryTree<AscendingBTTrait<int>> bt;
    bt.insert(50, 1); bt.insert(20, 2); bt.insert(80, 3); bt.insert(10, 4);
    cout << "3) BinaryTree:  "; for(auto& x : bt) cout << x << " "; cout << endl;

    // 4. AVL (inorder balanceado)
    AVL<AscendingAVLTrait<int>> avl;
    avl.insert(50, 1); avl.insert(20, 2); avl.insert(80, 3); avl.insert(10, 4);
    cout << "4) AVL:         "; for(auto& x : avl) cout << x << " "; cout << endl;

    // 5. Heap (orden interno del array, no ordenado por prioridad)
    Heap<MinHeapTrait<int>> h;
    h.insert(50, 1); h.insert(20, 2); h.insert(80, 3); h.insert(10, 4);
    cout << "5) Heap array:  "; for(auto& x : h) cout << x << " "; cout << endl;

    // 6. HashTable con structured bindings
    HashTable<int, int> ht;
    ht[5] = 50; ht[2] = 20; ht[8] = 80;
    cout << "6) HashTable:   ";
    for(const auto& [k, val] : ht) cout << k << "->" << val << "  ";
    cout << endl;
}

// toString(Traversal) — el profe pidio que toString aceptara un modo de
// recorrido con inorder por defecto. Mostramos los 3 sobre un AVL.
void DemoTraversals() {
    cout << "\n=== toString parametrizado por traversal (inorder default) ===" << endl;
    AVL<AscendingAVLTrait<int>> avl;
    for(int v : {50, 20, 80, 10, 30, 5, 70}) avl.insert(v, v);

    cout << "  Inorder  (default): " << avl.toString() << endl;
    cout << "  Preorder          : " << avl.toString(Traversal::Preorder)  << endl;
    cout << "  Postorder         : " << avl.toString(Traversal::Postorder) << endl;
}

void HashTableDemo() {
    DemoHashTableBasico();
    DemoStructuredBindings();
    DemoCopyMove();
    DemoHashTablePersistencia();
    DemoHashTableConcurrency();
    DemoRangeForNativo();
    DemoPolimorfico();
    DemoTraversals();
}
