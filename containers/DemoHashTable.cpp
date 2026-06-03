#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <thread>
#include <atomic>

#include "../types.h"
#include "hashtable.h"

using namespace std;

void DemoHashTableBasico() {
    cout << "\n=== HashTable Basico Demo ===" << endl;
    HashTable<int, string> m;

    // Insertar via insert(k, v)
    m.insert(5, "cinco");
    m.insert(2, "dos");
    m.insert(8, "ocho");
    m.insert(1, "uno");

    cout << "Despues de 4 inserts: size = " << m.size() << endl;

    // operator[] read
    cout << "m[5] = " << m[5] << endl;
    cout << "m[2] = " << m[2] << endl;

    // operator[] insert-on-miss
    m[42] = "cuarenta y dos";
    cout << "Despues de m[42]= : size = " << m.size() << ", m[42] = " << m[42] << endl;

    // contains
    cout << "contains(5) = " << m.contains(5)  << endl;
    cout << "contains(99) = " << m.contains(99) << endl;

    // at con key inexistente -> throw
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

    // range-for + structured bindings — la prueba de fuego del KVPair
    cout << "Iteracion inorder (key ascendente esperado):" << endl;
    for(auto it = m.begin(); it != m.end(); ++it) {
        // Cada deref devuelve KVPair&; structured bindings via get<I>
        auto& pair = *it;
        cout << "  [" << get<0>(pair) << "] = " << get<1>(pair) << endl;
    }
}

void DemoHashTablePersistencia() {
    cout << "\n=== HashTable Persistencia Demo ===" << endl;
    HashTable<int, int> m;
    m[10] = 100;
    m[5]  = 50;
    m[20] = 200;
    m[1]  = 1;

    // No tenemos operator<< que respete el formato KVPair;
    // mostramos por toString del AVL base.
    cout << "Estado en memoria (inorder): " << m.toString() << endl;

    ofstream of("hashtable.txt");
    of << m.toString() << endl;
    of.close();
    cout << "Escrito a hashtable.txt" << endl;
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

void HashTableDemo() {
    DemoHashTableBasico();
    DemoStructuredBindings();
    DemoHashTablePersistencia();
    DemoHashTableConcurrency();
}
