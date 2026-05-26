#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <thread>

#include "../types.h"
#include "linkedlist.h"
#include "doublelinkedlist.h"
#include "circularlinkedlist.h"

using namespace std;

template <typename Container>
void DemoFile(Container& list, string fileName){
    list.insert(28, 15);
    list.insert(17, 25);
    list.insert(8, 35);
    list.insert(4, 45);
    list.insert(35, 55);
    cout << "  Original:      " << list << endl;
    ofstream os(fileName);
    os << list << endl;
    os.close();

    Container listFromFile;
    ifstream is(fileName);
    is >> listFromFile;
    cout << "  Leida archivo: " << listFromFile << endl;
}

void LinkedListDemo(){
    cout << "\n=== LINKED LIST ===" << endl;
    LinkedList<AscendingLinkedListTrait<T1>> asc;
    DemoFile(asc, "AscLL.txt");
    LinkedList<DescendingLinkedListTrait<T1>> desc;
    DemoFile(desc, "DescLL.txt");
}

void DoubleLinkedListDemo(){
    cout << "\n=== DOUBLE LINKED LIST ===" << endl;

    cout << "\n1. INSERCION ORDENADA" << endl;
    DoubleLinkedList<AscendingDLLTrait<T1>> asc;
    asc.insert(3, 30); asc.insert(1, 10); asc.insert(5, 50);
    asc.insert(2, 20); asc.insert(4, 40);
    cout << "  Asc:  " << asc << endl;

    DoubleLinkedList<DescendingDLLTrait<T1>> desc;
    desc.insert(3, 30); desc.insert(1, 10); desc.insert(5, 50);
    desc.insert(2, 20); desc.insert(4, 40);
    cout << "  Desc: " << desc << endl;

    cout << "\n2. PUSH FRONT / PUSH BACK" << endl;
    DoubleLinkedList<AscendingDLLTrait<T1>> pushList;
    pushList.push_back(20, 2); pushList.push_back(30, 3);
    pushList.push_front(10, 1); pushList.push_front(5, 0);
    cout << "  " << pushList << endl;

    cout << "\n3. POP FRONT / POP BACK" << endl;
    auto [d1, r1] = pushList.pop_front();
    cout << "  pop_front -> (" << d1 << "," << r1 << ") | " << pushList << endl;
    auto [d2, r2] = pushList.pop_back();
    cout << "  pop_back  -> (" << d2 << "," << r2 << ") | " << pushList << endl;

    cout << "\n4. ITERADOR FORWARD" << endl;
    DoubleLinkedList<AscendingDLLTrait<T1>> it;
    it.insert(1, 10); it.insert(2, 20); it.insert(3, 30);
    it.insert(4, 40); it.insert(5, 50);
    cout << "  fwd: ";
    for (auto& v : it) cout << v << " ";
    cout << endl;

    cout << "\n5. ITERADOR BACKWARD" << endl;
    cout << "  bwd: ";
    for (auto i = it.rbegin(); i != it.rend(); ++i) cout << *i << " ";
    cout << endl;

    cout << "\n6. COPY / MOVE" << endl;
    DoubleLinkedList<AscendingDLLTrait<T1>> copy(it);
    cout << "  Original: " << it << endl;
    cout << "  Copia:    " << copy << endl;
    DoubleLinkedList<AscendingDLLTrait<T1>> moved(std::move(copy));
    cout << "  Moved:    " << moved << endl;
    cout << "  Tras move (copy size = 0): " << copy.size() << endl;

    cout << "\n7. ESCRITURA / LECTURA ARCHIVOS" << endl;
    DoubleLinkedList<AscendingDLLTrait<T1>> fileAsc;
    DemoFile(fileAsc, "AscDLL.txt");

    cout << "\n8. OPERATOR>> DESDE STREAM" << endl;
    DoubleLinkedList<AscendingDLLTrait<T1>> sl;
    stringstream ss("[(30,3),(10,1),(20,2)]");
    ss >> sl;
    cout << "  Stream desordenado -> lista: " << sl << endl;
}

void CircularLinkedListDemo(){
    cout << "\n=== CIRCULAR LINKED LIST ===" << endl;

    cout << "\n1. INSERCION ORDENADA" << endl;
    CircularLinkedList<AscendingCLLTrait<T1>> asc;
    asc.insert(3, 30); asc.insert(1, 10); asc.insert(5, 50);
    asc.insert(2, 20); asc.insert(4, 40);
    cout << "  Asc: " << asc << endl;

    cout << "\n2. PUSH/POP" << endl;
    CircularLinkedList<AscendingCLLTrait<T1>> p;
    p.push_back(20, 2); p.push_back(30, 3);
    p.push_front(10, 1); p.push_front(5, 0);
    cout << "  Despues de pushes: " << p << endl;
    auto [d1, r1] = p.pop_front();
    cout << "  pop_front -> (" << d1 << "," << r1 << ") | " << p << endl;
    auto [d2, r2] = p.pop_back();
    cout << "  pop_back  -> (" << d2 << "," << r2 << ") | " << p << endl;

    cout << "\n3. NATURALEZA CIRCULAR (1.5 vueltas)" << endl;
    CircularLinkedList<AscendingCLLTrait<T1>> c;
    c.insert(1, 10); c.insert(2, 20); c.insert(3, 30); c.insert(4, 40);
    cout << "  Lista: " << c << endl;
    cout << "  1.5 vueltas (pasos = size*1 + size/2): ";
    {
        // 1.5 vueltas == 4*1 + 4/2 = 6 pasos manuales
        size_t pasos = c.size() + c.size()/2;
        // Recorremos manualmente para no requerir un floating-point en circularForEach
        c.circularForEach(2, [&pasos](T1 &v){
            if (pasos == 0) return;
            cout << v << " ";
            pasos--;
        });
    }
    cout << endl;

    cout << "\n4. RANGED-FOR (1 vuelta exacta, sin loopear infinito)" << endl;
    cout << "  ";
    for (auto& v : c) cout << v << " ";
    cout << endl;

    cout << "\n5. FOREACH" << endl;
    cout << "  ";
    c.ForEach([](T1 &v){ cout << v << " "; });
    cout << endl;

    cout << "\n6. COPY / MOVE" << endl;
    CircularLinkedList<AscendingCLLTrait<T1>> orig;
    orig.insert(10, 1); orig.insert(20, 2); orig.insert(30, 3);
    CircularLinkedList<AscendingCLLTrait<T1>> copied(orig);
    cout << "  Orig:   " << orig << endl;
    cout << "  Copied: " << copied << endl;
    CircularLinkedList<AscendingCLLTrait<T1>> moved(std::move(orig));
    cout << "  Moved:  " << moved << endl;
    cout << "  Orig tras move (size=0): " << orig.size() << endl;

    cout << "\n7. ESCRITURA / LECTURA ARCHIVOS" << endl;
    CircularLinkedList<AscendingCLLTrait<T1>> fileAsc;
    DemoFile(fileAsc, "AscCLL.txt");
}

void TestConcurrencia(){
    cout << "\n=== CONCURRENCIA (LinkedList) ===" << endl;
    LinkedList<AscendingLinkedListTrait<T1>> list;
    auto worker = [&list](int id){
        for (int i = 0; i < 1000; i++) list.push_front(i, id);
    };
    thread t1(worker, 1), t2(worker, 2), t3(worker, 3), t4(worker, 4), t5(worker, 5);
    t1.join(); t2.join(); t3.join(); t4.join(); t5.join();
    cout << "  Tamano esperado 5000: " << list.size() << endl;
    cout << "  ESTADO: " << (list.size() == 5000 ? "EXITO" : "FALLO") << endl;
}

void ListsDemo(){
    LinkedListDemo();
    DoubleLinkedListDemo();
    CircularLinkedListDemo();
    TestConcurrencia();
    cout << "\n=== FIN ===" << endl;
}