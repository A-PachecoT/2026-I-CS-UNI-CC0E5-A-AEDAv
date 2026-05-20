#include <iostream>
#include <fstream>
#include <thread>
#include <vector>

#include "../BinaryTree.h"
#include "../BinaryTreeAVL.h"

using namespace std;

void BinaryTreeAVLDemo() {
    cout << "\n=== AVL Demo ===" << endl;

    BinaryTreeAVL<AscendingAVLTrait<int>> avl;
    for (int v : {1, 2, 3, 4, 5, 6, 7}) avl.insert(v);

    cout << "AVL inorder (1..7 insertados en orden): " << avl << endl;
    cout << "Altura: " << avl.height() << " (esperado <= 3, sin AVL seria 7)" << endl;

    BinaryTreeAVL<DescendingAVLTrait<int>> avl2;
    for (int v : {7, 6, 5, 4, 3, 2, 1}) avl2.insert(v);
    cout << "AVL desc inorder: " << avl2 << endl;
    cout << "Altura: " << avl2.height() << " (esperado <= 3)" << endl;
}

void BinaryTreeDemo() {
    cout << "\n=== BinaryTree Demo ===" << endl;

    BinaryTree<AscendingBSTrait<int>> tree;
    for (int v : {5, 3, 7, 1, 4, 6, 8}) tree.insert(v);

    cout << "BST inorder (forward): " << tree << endl;
    cout << "Size: "   << tree.size()   << endl;
    cout << "Height: " << tree.height() << endl;

    // Persistencia a archivos
    ofstream f("ascendenteBinST.txt");
    f << tree;
    f.close();

    BinaryTree<DescendingBSTrait<int>> tree2;
    for (int v : {5, 3, 7, 1, 4, 6, 8}) tree2.insert(v);
    ofstream f2("descendenteBinST.txt");
    f2 << tree2;
    f2.close();

    // operator>> desde archivo
    BinaryTree<AscendingBSTrait<int>> tree3;
    ifstream fin("ascendenteBinST.txt");
    fin >> tree3;
    fin.close();
    cout << "Cargado desde archivo: " << tree3 << endl;

    cout << "range-for: ";
    for (auto& v : tree) cout << v << " ";
    cout << endl;

    cout << "ForEach:   ";
    tree.ForEach([](int& v) { cout << v << " "; });
    cout << endl;

    cout << "backward:  ";
    for (auto it = tree.rbegin(); it != tree.rend(); ++it) cout << *it << " ";
    cout << endl;

    cout << "preorder:  ";
    for (auto it = tree.preorder_begin(); it != tree.preorder_end(); ++it) cout << *it << " ";
    cout << endl;

    cout << "pre-back:  ";
    for (auto it = tree.preorder_rbegin(); it != tree.preorder_rend(); ++it) cout << *it << " ";
    cout << endl;

    cout << "postorder: ";
    for (auto it = tree.postorder_begin(); it != tree.postorder_end(); ++it) cout << *it << " ";
    cout << endl;

    cout << "post-back: ";
    for (auto it = tree.postorder_rbegin(); it != tree.postorder_rend(); ++it) cout << *it << " ";
    cout << endl;

    cout << "contains(7): " << boolalpha << tree.contains(7) << endl;
    cout << "contains(9): " << tree.contains(9) << endl;
    cout << "balance_factor(root): " << tree.balance_factor(nullptr) << endl;

    // Constructor copia
    BinaryTree<AscendingBSTrait<int>> copy(tree);
    cout << "Copy ctor:       " << copy << endl;

    // Move constructor
    BinaryTree<AscendingBSTrait<int>> moved(std::move(copy));
    cout << "Move ctor:       " << moved << endl;

    // Concurrencia: 5 hilos x 1000 inserts
    cout << "\n--- Concurrency test (5 threads x 1000 inserts) ---" << endl;
    BinaryTree<AscendingBSTrait<int>> ctree;
    auto worker = [&ctree](int start) {
        for (int i = start; i < start + 1000; ++i) ctree.insert(i);
    };
    vector<thread> threads;
    for (int i = 0; i < 5; ++i) threads.emplace_back(worker, i * 1000);
    for (auto& t : threads) t.join();
    cout << "size=" << ctree.size() << " (esperado 5000) -> "
         << (ctree.size() == 5000 ? "OK" : "FALLO") << endl;

    BinaryTreeAVLDemo();
}
