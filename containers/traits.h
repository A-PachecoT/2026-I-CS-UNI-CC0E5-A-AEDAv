#ifndef __TRAITS_H__
#define __TRAITS_H__

#include <functional> // para std::less y std::greater
using namespace std;

template <typename T, typename _Comp, typename _Node>
struct BaseTrait {
    using value_type = T;
    using Comp       = _Comp;
    using Node       = _Node;
};

// Adaptadores para compatibilidad hacia atrás con BinaryTree y BinaryTreeAVL
template <typename _Node>
struct AscendingTrait : public BaseTrait<typename _Node::value_type, std::less<typename _Node::value_type>, _Node> {
};

template <typename _Node>
struct DescendingTrait : public BaseTrait<typename _Node::value_type, std::greater<typename _Node::value_type>, _Node> {
};

#endif // __TRAITS_H__