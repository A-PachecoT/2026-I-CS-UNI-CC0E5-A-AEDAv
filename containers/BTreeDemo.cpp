#include <iostream>
#include "BTree.h"

using namespace std;

const char *keys1 = "D1XJ2xTg8zKL9AhijOPQcEowRSp0NbW567BUfCqrs4FdtYZakHIuvGV3eMylmn";

template <typename Trait>
void DemoBTree(const string &label)
{
       cout << "===== BTree " << label << " =====" << endl;
       BTree<Trait> bt(3);
       for (int i = 0; keys1[i]; i++)
               bt.Insert(keys1[i], i * i);

       cout << "size=" << bt.size() << " height=" << bt.height() << endl;
       cout << "--- Print (ForEach) ---" << endl;
       bt.Print(cout);

       char target = 'K';
       long id = bt.Search(target);
       cout << "Search('" << target << "') -> ObjID=" << id << endl;

       char missing = '~';
       cout << "Search('" << missing << "') -> ObjID=" << bt.Search(missing) << endl;

       char umbral = 'M';
       auto found = bt.FirstThat(
               [](typename BTree<Trait>::ObjectInfo &info, int, char th) {
                       return info.key > th;
               },
               umbral);
       if (found)
               cout << "FirstThat(key > '" << umbral << "') -> '" << found->key << "'" << endl;
       else
               cout << "FirstThat(key > '" << umbral << "') -> nullptr" << endl;
       cout << endl;
}

void BTreeDemo()
{
       DemoBTree<AscendingBTreeTrait<char>>("Ascendente");
       DemoBTree<DescendingBTreeTrait<char>>("Descendente");
}
