# PC3 — BinaryTree + AVL — Pacheco Taboada

Entrega de PC3 sobre el branch `21-BinaryTree`. Implementa BST genérico con 6 iteradores (in/pre/post × fwd/bwd), regla de los cinco con concurrencia (`shared_mutex`), persistencia a archivos, y subclase AVL con auto-balanceo por rotaciones.

## Tareas completadas

### BinaryTree (16/16)

- [x] Constructor copia (deep copy recursivo con `shared_lock` en source)
- [x] Move constructor (`std::exchange` + `unique_lock`)
- [x] Destructor seguro (recursión postorder sobre `m_pChild[0]` y `m_pChild[1]`)
- [x] Forward iterator inorder (stack + `push_left`)
- [x] Backward iterator inorder (stack + `push_right`)
- [x] Foreach nativo (`for (auto& v : tree)`)
- [x] `ToString()` (`ostringstream`, formato `[v1,v2,...]`)
- [x] `operator<<` (delega en `ToString`, persiste a archivo)
- [x] `operator>>` (parser de `[v1,v2,...]`, usa `setstate(failbit)` en formato inválido)
- [x] Concurrency (`shared_mutex`: `shared_lock` para lectura, `unique_lock` para escritura)
- [x] Forward iterator preorder (stack, root-left-right)
- [x] Backward iterator preorder (precompute en `vector`, traverse en reversa)
- [x] Forward iterator postorder (técnica de dos stacks)
- [x] Backward iterator postorder (root-right-left iterativo)
- [x] Mejora libre #1: `contains(val)` iterativo sin recursión (O(log n) amortizado, sin overhead de stack frames)
- [x] Mejora libre #2: `height()` + `balance_factor()` con cache implícito a través de `internal_height`

### AVL (2/2)

- [x] Adaptar `insert` de BinaryTree (override virtual con `avl_insert` recursivo + rotaciones)
- [x] Extender `BinaryTreeNode` con altura (`AVLNode` hereda de `BinaryTreeNodeBase<Derived,T>` CRTP con `m_height`)

### Reutilización (5/5)

- [x] Hacer un Demo (`containers/BinaryTreeDemo.cpp` muestra los 6 iteradores + persistencia + concurrencia + AVL)
- [x] `operator>>` reutilizado (mismo formato `[v1,v2,...]` que el `operator<<` para round-trip)
- [x] Node en CLL — no aplica directamente; el `BinaryTreeNodeBase` CRTP es la abstracción equivalente para árboles
- [x] Node en DLL (`Node + pPrev`) — análogo: `BinaryTreeNodeBase` con `Derived*` permite que `AVLNode` reuse la estructura sin re-declarar `m_pChild[]`
- [x] Iterator reutilizado (los 6 iteradores heredan de `general_iterator<Container, IteratorBase>` CRTP — único cambio: agregamos `operator!=` faltante)

### Errores propuestos al branch (3/3)

Detallados en `PROPUESTAS.md`:

1. `branch = !m_comp(pNode->m_data, data)` invierte el sentido del BST — el archivo "ascendenteBinST.txt" del baseline salía descendente. Corregido a `branch = !m_comp(data, pNode->m_data)`.
2. `general_iterator` no expone `operator!=` — todos los `for(it != end; ...)` fallaban a compilar. Agregado como `friend`.
3. `traits.h` usaba `less<...>` sin `std::` — fallaba si el header se incluía antes de cualquier `using namespace std`. Calificado.

### Meta-deliverables (2/2)

- [x] Descripción del PR (este archivo)
- [x] Gráfico Mermaid (sección abajo)

## Gráfico de jerarquía

```mermaid
classDiagram
    direction TB

    %% Nodos
    class BinaryTreeNodeBase {
        <<CRTP base>>
        +T m_data
        +Derived* m_pChild[2]
        +getDataRef()
    }
    class BinaryTreeNode {
        BinaryTreeNode(T)
    }
    class AVLNode {
        +int m_height
        AVLNode(T)
    }

    BinaryTreeNodeBase <|-- BinaryTreeNode : CRTP
    BinaryTreeNodeBase <|-- AVLNode        : CRTP

    %% Árboles
    class BinaryTree~Trait~ {
        -Node* m_pRoot
        -Comp m_comp
        -size_t m_size
        -shared_mutex m_mtx
        +insert(value) virtual
        +ToString()
        +operator<<()
        +operator>>()
        +begin()/end()/rbegin()/rend()
        +preorder_begin()/end()
        +postorder_begin()/end()
        +ForEach(func)
    }
    class BinaryTreeAVL~Trait~ {
        +insert(value) override
        -avl_insert()
        -rotate_left/right()
        -update_height()
    }

    BinaryTree <|-- BinaryTreeAVL : public inheritance

    BinaryTree --> BinaryTreeNode : Node = Trait::Node
    BinaryTreeAVL --> AVLNode     : Node = Trait::Node

    %% Iteradores
    class general_iterator~Container,IteratorBase~ {
        <<CRTP base>>
        #Container* m_pContainer
        #Node* m_pNode
        +operator*()
        +operator==()
        +operator!=()
    }
    class BTInorderForwardIterator
    class BTInorderBackwardIterator
    class BTPreorderForwardIterator
    class BTPreorderBackwardIterator
    class BTPostorderForwardIterator
    class BTPostorderBackwardIterator

    general_iterator <|-- BTInorderForwardIterator
    general_iterator <|-- BTInorderBackwardIterator
    general_iterator <|-- BTPreorderForwardIterator
    general_iterator <|-- BTPreorderBackwardIterator
    general_iterator <|-- BTPostorderForwardIterator
    general_iterator <|-- BTPostorderBackwardIterator

    %% Traits
    class BaseTrait~Node,Comp~ {
        +Node
        +value_type
        +Comp
    }
    class AscendingTrait
    class DescendingTrait
    class AscendingBSTrait
    class AscendingAVLTrait

    BaseTrait <|-- AscendingTrait
    BaseTrait <|-- DescendingTrait
    AscendingTrait <.. AscendingBSTrait   : alias BinaryTreeNode<T>
    AscendingTrait <.. AscendingAVLTrait  : alias AVLNode<T>
```

## Output del demo

```
=== BinaryTree Demo ===
BST inorder (forward): [1,3,4,5,6,7,8]
Size: 7
Height: 3
Cargado desde archivo: [1,3,4,5,6,7,8]
range-for: 1 3 4 5 6 7 8
ForEach:   1 3 4 5 6 7 8
backward:  8 7 6 5 4 3 1
preorder:  5 3 1 4 7 6 8
pre-back:  8 6 7 4 1 3 5
postorder: 1 4 3 6 8 7 5
post-back: 5 7 8 6 3 4 1
contains(7): true
contains(9): false
balance_factor(root): 0
Copy ctor:       [1,3,4,5,6,7,8]
Move ctor:       [1,3,4,5,6,7,8]

--- Concurrency test (5 threads x 1000 inserts) ---
size=5000 (esperado 5000) -> OK

=== AVL Demo ===
AVL inorder (1..7 insertados en orden): [1,2,3,4,5,6,7]
Altura: 3 (esperado <= 3, sin AVL seria 7)
AVL desc inorder: [7,6,5,4,3,2,1]
Altura: 3 (esperado <= 3)
```

## Notas

- Compila con `g++ -std=c++2b -Wall -g -pthread` sin warnings (`make clean && make`).
- 5 threads × 1000 inserts → 5000 nodos consistentes (concurrency OK).
- AVL en peor caso (1..7 ordenado o 7..1 ordenado) mantiene altura 3 en lugar de 7 — auto-balanceo verificado.
