#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
#include <vector>

#include "../types.h"
#include "heap.h"

using namespace std;

void DemoMinHeap() {
    cout << "\n=== MinHeap Demo ===" << endl;
    Heap<MinHeapTrait<T1>> h;

    // Insertar desordenado
    int valores[]    = {50, 20, 80, 10, 30, 5, 70};
    int refs[]       = {1,  2,  3,  4,  5, 6,  7};
    for(size_t i = 0; i < 7; ++i)
        h.insert(valores[i], refs[i]);

    cout << "Heap interno (array): " << h << endl;
    cout << "size = " << h.size() << endl;

    // Extraer todo — debe salir en orden ASCENDENTE
    cout << "Extracciones (orden ascendente esperado): ";
    while(!h.empty()) {
        auto [v, r] = h.extract();
        cout << "(" << v << "," << r << ") ";
    }
    cout << endl;

    // Persistir
    ofstream of("minheap.txt");
    for(size_t i = 0; i < 7; ++i) h.insert(valores[i], refs[i]);
    of << h << endl;
    of.close();

    // Releer en otro heap
    Heap<MinHeapTrait<T1>> h2;
    ifstream in("minheap.txt");
    in >> h2;
    cout << "Releido desde minheap.txt: " << h2 << endl;
    cout << "Peek del releido: ";
    auto [pv, pr] = h2.peek();
    cout << "(" << pv << "," << pr << ")" << endl;
}

void DemoMaxHeap() {
    cout << "\n=== MaxHeap Demo ===" << endl;
    Heap<MaxHeapTrait<T1>> h;

    int valores[] = {50, 20, 80, 10, 30, 5, 70};
    int refs[]    = {1,  2,  3,  4,  5, 6,  7};
    for(size_t i = 0; i < 7; ++i)
        h.insert(valores[i], refs[i]);

    cout << "Heap interno: " << h << endl;
    cout << "Extracciones (orden DESCENDENTE esperado): ";
    while(!h.empty()) {
        auto [v, r] = h.extract();
        cout << "(" << v << "," << r << ") ";
    }
    cout << endl;

    ofstream of("maxheap.txt");
    for(size_t i = 0; i < 7; ++i) h.insert(valores[i], refs[i]);
    of << h << endl;
}

void DemoHeapConcurrency() {
    cout << "\n=== Heap Concurrency Demo ===" << endl;
    Heap<MinHeapTrait<T1>> h;

    auto worker = [&h](int id) {
        for(int i = 0; i < 200; ++i)
            h.insert(i + id*1000, id);
    };

    thread t1(worker, 1), t2(worker, 2), t3(worker, 3),
           t4(worker, 4), t5(worker, 5);
    t1.join(); t2.join(); t3.join(); t4.join(); t5.join();

    cout << "5 hilos x 200 inserts. Esperado=1000, real=" << h.size() << endl;
    cout << (h.size() == 1000 ? "EXITO" : "FALLO") << endl;

    // El menor de todos debe ser 1001 (worker 1, i=1) -> NO, worker 1 inserta
    // 0+1000=1000, 1+1000=1001, ... el min es 1000.
    // Worker 0 no existe. Worker 1 -> 1000..1199. Min global = 1000.
    auto [v, r] = h.peek();
    cout << "Peek (esperado v=1000, ref=1): v=" << v << " r=" << r << endl;
}

void HeapDemo() {
    DemoMinHeap();
    DemoMaxHeap();
    DemoHeapConcurrency();
}
