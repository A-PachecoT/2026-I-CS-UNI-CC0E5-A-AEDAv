#include <iostream>
#include <sstream>
#include "BTree.h"

using namespace std;

const char *keys1 = "D1XJ2xTg8zKL9AhijOPQcEowRSp0NbW567BUfCqrs4FdtYZakHIuvGV3eMylmn";

template <typename Trait>
void DemoBTree(const string &label)
{
       using value_type = typename Trait::value_type;
       cout << "===== BTree " << label << " =====" << endl;
       BTree<Trait> bt(3);
       for (size_t i = 0; keys1[i]; i++)
               bt.Insert(keys1[i], i * i);

       cout << "size=" << bt.size() << " height=" << bt.height() << endl;
       cout << "--- Print (ForEach) ---" << endl;
       bt.Print(cout);

       value_type target = 'K';
       auto id = bt.Search(target);
       cout << "Search('" << target << "') -> ObjID=" << id << endl;

       value_type missing = '~';
       cout << "Search('" << missing << "') -> ObjID=" << bt.Search(missing) << endl;

       value_type umbral = 'M';
       auto found = bt.FirstThat(
               [](typename BTree<Trait>::ObjectInfo &info, value_type th) {
                       return info.key > th;
               },
               umbral);
       if (found)
               cout << "FirstThat(key > '" << umbral << "') -> '" << found->key << "'" << endl;
       else
               cout << "FirstThat(key > '" << umbral << "') -> nullptr" << endl;

       cout << "--- operator<< ---" << endl;
       cout << bt << endl;

       cout << "--- ReverseForEach ---" << endl;
       bt.ReverseForEach([](typename BTree<Trait>::ObjectInfo &info){ cout << info.key; });
       cout << endl;

       cout << "--- operator>> ---" << endl;
       BTree<Trait> bt2(3);
       istringstream iss("(a,1)(b,2)(c,3)");
       iss >> bt2;
       cout << bt2 << endl;

       cout << "--- Remove (una de cada dos) ---" << endl;
       for (size_t i = 0; keys1[i]; i += 2)
               bt.Remove(keys1[i], i * i);
       cout << "size tras remove=" << bt.size() << endl;
       cout << bt << endl;
       cout << endl;
}

void BTreeDemo()
{
       DemoBTree<AscendingBTreeTrait<char>>("Ascendente");
       DemoBTree<DescendingBTreeTrait<char>>("Descendente");
}
