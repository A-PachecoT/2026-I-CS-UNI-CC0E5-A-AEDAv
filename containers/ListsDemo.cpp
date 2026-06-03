#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <thread>

#include "../types.h"
#include "linkedlist.h"

using namespace std;

template <typename Container>
void DemoList(Container& list, string fileName){
    list.insert(28, 15);
    list.insert(17, 25);
    list.insert(8, 35);
    list.insert(4, 45);
    list.insert(35, 55);
    cout << list << endl;
    ofstream os(fileName);
    os << list << endl;

    ifstream is(fileName);
    is >> list;
    cout << list << endl;
}

void LinkedListDemo(){
    LinkedList<AscendingLinkedListTrait<T1>> list;
    DemoList(list, "AscLL.txt");
    LinkedList<DescendingLinkedListTrait<T1>> list2;
    DemoList(list2, "DescLL.txt");
}

void TestConcurrencia() {
    cout << "\nTEST DE CONCURRENCIA (LinkedList)" << endl;
    LinkedList<AscendingLinkedListTrait<T1>> list;

    auto worker = [&list](int thread_id) {
        for(int i = 0; i < 1000; i++) {
            list.push_front(i, thread_id);
        }
    };

    thread t1(worker, 1);
    thread t2(worker, 2);
    thread t3(worker, 3);
    thread t4(worker, 4);
    thread t5(worker, 5);

    t1.join(); t2.join(); t3.join(); t4.join(); t5.join();

    cout << "5 hilos x 1000 inserts. Tam esperado=5000, real=" << list.size() << endl;
    cout << (list.size() == 5000 ? "EXITO" : "FALLO") << endl;
}

void TestOperators() {
    cout << "\nTEST DE OPERADORES (LinkedList)" << endl;
    LinkedList<AscendingLinkedListTrait<T1>> list;

    stringstream input("[(10,100),(20,200),(30,300)]");
    input >> list;
    cout << "Despues de operator>>: " << list << endl;
    cout << "list[0] = " << list[0] << endl;
    cout << "list[2] = " << list[2] << endl;
}

void ListsDemo(){
    LinkedListDemo();
    TestConcurrencia();
    TestOperators();
    cout << "\n=== FIN DE LISTSDEMO ===" << endl;
}
