#include <iostream>
// g++ -std=c++2b main.cpp containers/ListsDemo.cpp containers/DemoHeap.cpp containers/DemoHashTable.cpp -o main

void ListsDemo();
void HeapDemo();
void HashTableDemo();

int main(){
    ListsDemo();
    HeapDemo();
    HashTableDemo();
    std::cout << "\n=== TODOS LOS DEMOS FINALIZADOS ===" << std::endl;
    return 0;
}