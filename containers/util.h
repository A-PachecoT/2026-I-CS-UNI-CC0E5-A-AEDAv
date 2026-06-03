#ifndef __UTIL_H__
#define __UTIL_H__
#include <ostream>
#include <istream>
#include <ios>
#include "../types.h"  // Ref
using namespace std;

template <typename Container>
void Print(Container& c, ostream &os){
    os << c << endl;
}

template <typename T>
void PrintX(T& elem, ostream &os, string sep){
    os << elem << sep;
}

template <typename Container, typename Func>
void ForEach(Container& c, Func func){
    for(auto it = c.begin(); it != c.end(); ++it)
        func(*it);
}

template <typename Iterator, typename Func, typename... Args>
void ForEach(Iterator begin, Iterator end, Func func, Args&&... args){
    for(auto it = begin; it != end; ++it)
        func(*it, forward<Args>(args)...);
}

template <typename Container, typename Func, typename... Args>
void ForEach(Container& container, Func func, Args&&... args){
    ForEach(container.begin(), container.end(),
            func, forward<Args>(args)...);
}

// container_write / container_read — serialización cross-cutting.
//
// Contrato:
//   - Container expone begin() y end() que devuelven iteradores tipo
//     general_iterator<...> con getNode() -> Node*.
//   - Node expone getData() -> value_type  y  getRef() -> Ref.
//   - Container expone push_back(value_type, Ref) para container_read.
//
// Formato: "[(d1,r1),(d2,r2),...]"
//
// Cualquier contenedor nuevo solo necesita:
//   friend ostream& operator<<(ostream& os, const X& x){ return container_write(os, x); }
//   friend istream& operator>>(istream& is, X& x){       return container_read(is, x);  }
//
// Extrae el boilerplate de operator<< / operator>> que cada contenedor
// repetiria de manera identica.

template <typename Container>
ostream& container_write(ostream& os, const Container& c){
    os << "[";
    bool first = true;
    for(auto it = c.begin(); it != c.end(); ++it){
        if(!first) os << ",";
        auto* node = it.getNode();
        os << "(" << node->getData() << "," << node->getRef() << ")";
        first = false;
    }
    os << "]";
    return os;
}

template <typename Container>
istream& container_read(istream& is, Container& c){
    char ch;
    if(!(is >> ch) || ch != '['){
        is.clear(ios_base::failbit);
        return is;
    }
    typename Container::value_type val;
    Ref ref;
    char comma, paren;
    while(is >> ch && ch != ']'){
        if(ch == '('){
            if(is >> val >> comma >> ref >> paren){
                if(comma == ',' && paren == ')'){
                    c.push_back(val, ref);
                }
            }
        }
    }
    return is;
}

#endif