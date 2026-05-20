# Mejoras propuestas al branch `21-BinaryTree`

## Mejora 1: `branch` invertido en `internal_insert` original

**Problema** (`BinaryTree.h:84` del baseline):

```cpp
auto branch = !m_comp(pNode->m_data, data);
internal_insert(pNode->m_pChild[branch], data, ref);
```

Con `m_comp = std::less<T>`, `m_comp(pNode, data)` = `pNode < data`. La negación da `pNode >= data`, lo que ubica valores **mayores o iguales** en la izquierda — inverso al BST ascendente. El archivo `ascendenteBinST.txt` del demo terminaba escribiéndose en orden descendente.

**Impacto:** correctness (BST queda invertido — inorder devuelve descendente).

**Propuesta:**

```cpp
auto branch = !m_comp(data, pNode->m_data);  // branch=1 si data >= pNode → derecha
```

**Trade-off:** ninguno; es estrictamente correcto.

---

## Mejora 2: `general_iterator` no define `operator!=`

**Problema** (`containers/general_iterator.h:30`): solo declara `operator==`. Cualquier `for (it = c.begin(); it != c.end(); ++it)` falla a compilar en C++17, y aunque C++20 sintetiza `!=` desde `==`, el proyecto compila como C++23 y el LSP del repo no resuelve la síntesis en todas las versiones de clangd que usan los compañeros.

**Impacto:** correctness + portabilidad. Bloquea todos los iteradores nuevos del PC3.

**Propuesta:**

```cpp
friend bool operator==(const IteratorBase &a, const IteratorBase &b) { return a.getNode() == b.getNode(); }
friend bool operator!=(const IteratorBase &a, const IteratorBase &b) { return a.getNode() != b.getNode(); }
```

**Trade-off:** ninguno; complementa `operator==` simétricamente.

---

## Mejora 3: `traits.h` usa `less`/`greater` sin calificar

**Problema** (`containers/traits.h:13,16`):

```cpp
struct AscendingTrait : public BaseTrait<_Node, less<typename _Node::value_type>>{
```

`less` y `greater` están en `std::` y `traits.h` no tiene `using namespace std;`. Compila por casualidad cuando el TU que incluye `traits.h` ya tiene `using namespace std;` upstream, pero rompe si `traits.h` se incluye primero o desde un header limpio.

**Impacto:** legibilidad + portabilidad.

**Propuesta:**

```cpp
struct AscendingTrait : public BaseTrait<_Node, std::less<typename _Node::value_type>>{
```

O alternativa: agregar `using namespace std;` después del `#include <functional>` (consistente con el resto del repo, aunque cuestionable como diseño).

**Trade-off:** la calificación explícita es la mejor opción a largo plazo; `using namespace` propaga el namespace a todo TU que incluya `traits.h`.

---

## Mejora 4 (nice-to-have, no aplicada): `internal_insert` debería retornar el nuevo nodo

**Problema:** el `internal_insert` actual recibe `Node*&` por referencia y modifica el árbol in-place. Esto fuerza a la subclase AVL a duplicar la firma con un `avl_insert(Node*, value_type) → Node*` que **sí** retorna. Si el padre retornara `Node*`, la AVL podría hacer:

```cpp
Node* internal_insert(Node*& pNode, value_type data) override {
    pNode = BinaryTree::internal_insert(pNode, data);  // o reescribir
    update_height(pNode);
    ...  // rotaciones
    return pNode;
}
```

**Impacto:** API consistente, evita método paralelo (`avl_insert` vs `internal_insert`).

**Trade-off:** rompe la firma del baseline; lo dejo como propuesta no aplicada para preservar compatibilidad con PRs de compañeros.
