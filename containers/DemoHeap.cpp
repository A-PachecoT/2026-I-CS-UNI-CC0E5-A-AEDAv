#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "../types.h"
#include "heap.h"

using namespace std;

// Demo parametrizado: la misma logica para MinHeap y MaxHeap. El Trait
// (MinHeapTrait<T> o MaxHeapTrait<T>) determina el ordenamiento; el resto
// es identico — antes habia dos funciones casi gemelas (DemoMinHeap y
// DemoMaxHeap) con un solo diferencial: el Trait + nombre de archivo.
template <typename Trait>
static void DemoHipGenerico(const string& titulo,
                            const string& archivo,
                            const string& tagOrden) {
    cout << "\n=== " << titulo << " ===" << endl;
    Heap<Trait> h;

    const int valores[] = {50, 20, 80, 10, 30, 5, 70};
    const int refs[]    = {1,  2,  3,  4,  5, 6,  7};
    const size_t N = sizeof(valores) / sizeof(valores[0]);

    for(size_t i = 0; i < N; ++i)
        h.insert(valores[i], refs[i]);

    cout << "Heap interno (array): " << h << endl;
    cout << "size = " << h.size() << endl;

    cout << "Extracciones (orden " << tagOrden << " esperado): ";
    while(!h.empty()) {
        auto [v, r] = h.extract();
        cout << "(" << v << "," << r << ") ";
    }
    cout << endl;

    // Reinsertar para persistir
    for(size_t i = 0; i < N; ++i) h.insert(valores[i], refs[i]);
    ofstream of(archivo);
    of << h << endl;
    of.close();

    Heap<Trait> releido;
    ifstream in(archivo);
    in >> releido;
    cout << "Releido desde " << archivo << ": " << releido << endl;
    auto [pv, pr] = releido.peek();
    cout << "Peek del releido: (" << pv << "," << pr << ")" << endl;
}

void DemoMinHeap() {
    DemoHipGenerico<MinHeapTrait<T1>>("MinHeap Demo", "minheap.txt", "ASCENDENTE");
}

void DemoMaxHeap() {
    DemoHipGenerico<MaxHeapTrait<T1>>("MaxHeap Demo", "maxheap.txt", "DESCENDENTE");
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

    auto [v, r] = h.peek();
    cout << "Peek (esperado v=1000, ref=1): v=" << v << " r=" << r << endl;
}

void HeapDemo() {
    DemoMinHeap();
    DemoMaxHeap();
    DemoHeapConcurrency();
}
