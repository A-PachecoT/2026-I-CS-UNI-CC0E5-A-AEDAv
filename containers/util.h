#ifndef __UTIL_H__
#define __UTIL_H__
#include <ostream>
#include <istream>
using namespace std;

// operator<< genérico: una sola implementación reutilizable por containers
// que exponen value_type + begin/end. Formato canónico: [v1,v2,...]
template <typename Container,
          typename = typename Container::value_type>
ostream& operator<<(ostream& os, Container& c) {
    os << "[";
    bool first = true;
    for (auto& v : c) {
        if (!first) os << ",";
        os << v;
        first = false;
    }
    os << "]";
    return os;
}

// operator>> genérico: parsea [v1,v2,...] e inserta vía Container::insert(value)
template <typename Container,
          typename = decltype(std::declval<Container>().insert(std::declval<typename Container::value_type>()))>
istream& operator>>(istream& is, Container& c) {
    char ch;
    if (!(is >> ch) || ch != '[') { is.setstate(ios::failbit); return is; }
    typename Container::value_type val;
    while (is >> ch) {
        if (ch == ']') break;
        if (ch != ',') is.putback(ch);
        if (is >> val) c.insert(val);
    }
    return is;
}

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
#endif