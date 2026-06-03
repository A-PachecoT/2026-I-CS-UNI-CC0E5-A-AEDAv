#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <thread>
#include <utility>

#include "../types.h"
#include "hashtable.h"

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

void HashTableDemo() {
    DemoHashTableBasico();
    DemoStructuredBindings();
    DemoCopyMove();
    DemoHashTablePersistencia();
    DemoHashTableConcurrency();
}
