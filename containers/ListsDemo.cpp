#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <thread>

#include "../types.h"
#include "linkedlist.h"
#include "doublelinkedlist.h"
#include "circularlinkedlist.h"
#include "circulardoublelinkedlist.h"

using namespace std;

template <typename Container>
void DemoList(Container& list, string fileName) {
    list.insert(28, 15);
    list.insert(17, 25);
    list.insert(8,  35);
    list.insert(4,  45);
    list.insert(35, 55);
    cout << "  insert ordenado: " << list << endl;

    ofstream os(fileName);
    os << list << endl;

    Container fromFile;
    ifstream is(fileName);
    is >> fromFile;
    cout << "  roundtrip op<</op>>: " << fromFile << endl;
}

void LinkedListDemo() {
    cout << "\n--- LinkedList ---" << endl;
    LinkedList<AscendingLinkedListTrait<T1>>  asc;   DemoList(asc,  "AscLL.txt");
    LinkedList<DescendingLinkedListTrait<T1>> desc;  DemoList(desc, "DescLL.txt");
}

void DoubleLinkedListDemo() {
    cout << "\n--- DoubleLinkedList ---" << endl;
    DoubleLinkedList<AscendingDLLTrait<T1>>  asc;   DemoList(asc,  "AscDLL.txt");
    DoubleLinkedList<DescendingDLLTrait<T1>> desc;  DemoList(desc, "DescDLL.txt");

    // Recorrido forward via rango nativo + backward via rbegin/rend.
    DoubleLinkedList<AscendingDLLTrait<T1>> dll;
    dll.push_back(10, 1);
    dll.push_back(20, 2);
    dll.push_back(30, 3);
    cout << "  bidireccional: ";
    dll.dumpBidirectional(cout);
    cout << endl;
    cout << "  fwd con for(:) -> ";
    for (auto& x : dll) cout << x << " ";
    cout << endl;
    cout << "  bwd con op++ -> ";
    for (auto it = dll.rbegin(); it != dll.rend(); ++it) cout << *it << " ";
    cout << endl;

    // Rule of Five: copy + move.
    DoubleLinkedList<AscendingDLLTrait<T1>> copia(dll);
    cout << "  copia: " << copia << endl;
    DoubleLinkedList<AscendingDLLTrait<T1>> movida(std::move(copia));
    cout << "  movida: " << movida << endl;
    cout << "  origen post-move size = " << copia.size() << endl;
}

void CircularLinkedListDemo() {
    cout << "\n--- CircularLinkedList ---" << endl;
    CircularLinkedList<AscendingCLLTrait<T1>> cll;
    cll.push_back(1, 10);
    cll.push_back(2, 20);
    cll.push_back(3, 30);
    cout << "  estado: " << cll << endl;

    // Bucle nativo for(:) con centinela: corta exactamente al cerrar 1 vuelta.
    cout << "  for(:) (1 vuelta exacta) -> ";
    for (auto& x : cll) cout << x << " ";
    cout << endl;

    // Demo de circularidad real: 2 vueltas con circularForEach.
    cout << "  circularForEach(2 vueltas) -> ";
    cll.circularForEach(2, [](T1& v) { cout << v << " "; });
    cout << endl;

    // insert ordenado + op>>.
    DemoList(cll, "AscCLL.txt");
}

void CircularDoubleLinkedListDemo() {
    cout << "\n--- CircularDoubleLinkedList ---" << endl;
    CircularDoubleLinkedList<AscendingCDLLTrait<T1>> cdll;
    cdll.push_back(1, 10);
    cdll.push_back(2, 20);
    cdll.push_back(3, 30);
    cout << "  estado: " << cdll << endl;

    // for(:) en sentido forward — centinela corta a 1 vuelta exacta.
    cout << "  for(:) fwd (1 vuelta) -> ";
    for (auto& x : cdll) cout << x << " ";
    cout << endl;

    // Recorrido backward via rbegin/rend con centinela.
    cout << "  rbegin->rend bwd (1 vuelta) -> ";
    for (auto it = cdll.rbegin(); it != cdll.rend(); ++it) cout << *it << " ";
    cout << endl;

    // Prueba de circularidad bidireccional: 2 vueltas en ambas direcciones.
    cout << "  circularForEach(2 vueltas, fwd) -> ";
    cdll.circularForEach(2, [](T1& v) { cout << v << " "; });
    cout << endl;
    cout << "  circularReverseForEach(2 vueltas, bwd) -> ";
    cdll.circularReverseForEach(2, [](T1& v) { cout << v << " "; });
    cout << endl;

    // insert ordenado + op>>.
    DemoList(cdll, "AscCDLL.txt");
}

void TestConcurrencia() {
    cout << "\n--- Concurrencia (5 hilos x 1000 push_front) ---" << endl;
    DoubleLinkedList<AscendingDLLTrait<T1>> list;

    auto worker = [&list](int tid) {
        for (int i = 0; i < 1000; ++i) list.push_front(i, tid);
    };

    thread t1(worker, 1), t2(worker, 2), t3(worker, 3), t4(worker, 4), t5(worker, 5);
    t1.join(); t2.join(); t3.join(); t4.join(); t5.join();

    cout << "  size esperado 5000, obtenido " << list.size()
         << (list.size() == 5000 ? "  [OK]" : "  [RACE!]") << endl;
}

void TestOperators() {
    cout << "\n--- Operadores op>> / op[] / op<< ---" << endl;
    DoubleLinkedList<AscendingDLLTrait<T1>> list;
    stringstream sin("[(10,100),(20,200),(30,300)]");
    sin >> list;
    cout << "  parseado op>>: " << list << endl;
    cout << "  list[0] = " << list[0] << ", list[2] = " << list[2] << endl;
}

void ListsDemo() {
    cout << "==========================================" << endl;
    cout << "  EP1 — DoubleLinkedList + Circular LL"     << endl;
    cout << "  Andre Pacheco Taboada — 20222189G"        << endl;
    cout << "==========================================" << endl;
    LinkedListDemo();
    DoubleLinkedListDemo();
    CircularLinkedListDemo();
    CircularDoubleLinkedListDemo();
    TestOperators();
    TestConcurrencia();
    cout << "\n=== FIN ===" << endl;
}
