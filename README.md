# libcds

Biblioteca de estruturas de dados genéricas em C, **header-only**. Cada estrutura vive em um único `.h` dentro de `include/libcds/`, sem `.c` pra compilar separado — basta incluir o header.

Por enquanto só tem `linked_list.h`. A ideia é ir adicionando outras (stack, queue, hashmap...) seguindo o mesmo padrão de namespacing.

## Estrutura do projeto

```
libcds/
├── examples/            # exemplos de uso (em construção)
└── include/
    └── libcds/
        └── linked_list.h
```

## Como usar em outro projeto

Por ser header-only, não tem lib pra linkar — só precisa que o compilador ache o diretório `include/`. Algumas formas de fazer isso:

### 1. Copiar direto (mais simples)

Copie `include/libcds/` pra dentro do seu projeto e inclua:

```c
#include "libcds/linked_list.h"
```

Funciona, mas você perde a atualização automática se a lib mudar.

### 2. Git submodule

Dentro do repositório do seu projeto:

```bash
git submodule add https://github.com/H3nriqueLima/libcds.git external/libcds
git submodule update --init --recursive
```

E no build (exemplo com gcc direto):

```bash
gcc main.c -I external/libcds/include -o main
```

### 3. CMake FetchContent (se não quiser submodule)

```cmake
include(FetchContent)
FetchContent_Declare(
  libcds
  GIT_REPOSITORY https://github.com/H3nriqueLima/libcds.git
  GIT_TAG main # ou uma tag/commit fixo, mais seguro pra não quebrar
)
FetchContent_MakeAvailable(libcds)

target_include_directories(seu_alvo PRIVATE ${libcds_SOURCE_DIR}/include)
```

Depois disso, em qualquer arquivo do seu projeto:

```c
#include "libcds/linked_list.h"
```

## API — `linked_list.h`

Tipo público: `LinkedList` (`head`, `tail`, `size` — os três podem ser lidos direto, ex. `list->size`, mas não devem ser alterados manualmente fora das funções abaixo).

Todos os valores guardados são `void*` — a lista não sabe o tipo real, então **quem usa a lib é responsável por fazer o cast de volta** e por gerenciar a memória dos valores (a lista só gerencia seus próprios nós).

### Criação e estado

| Função | Descrição | Complexidade |
|---|---|---|
| `LinkedList* ll_create(void)` | Cria uma lista vazia. Retorna `NULL` se `malloc` falhar. | O(1) |
| `int ll_is_empty(LinkedList* list)` | `1` se vazia ou `NULL`, `0` caso contrário. | O(1) |

### Inserção

| Função | Descrição | Complexidade |
|---|---|---|
| `void ll_add_first(LinkedList* list, void* value)` | Insere no início. | O(1) |
| `void ll_add_last(LinkedList* list, void* value)` | Insere no fim. | O(1) |
| `void ll_insert_at(LinkedList* list, size_t index, void* value)` | Insere na posição `index` (`0` = início, `index >= size` = fim). | O(n) |

### Remoção

| Função | Descrição | Complexidade |
|---|---|---|
| `void* ll_remove_first(LinkedList* list)` | Remove e retorna o primeiro valor. `NULL` se vazia. | O(1) |
| `void* ll_remove_last(LinkedList* list)` | Remove e retorna o último valor. `NULL` se vazia. | O(n)* |
| `void* ll_remove_at(LinkedList* list, size_t index)` | Remove e retorna o valor na posição `index`. `NULL` se fora do range. | O(n) |

\* A lista é encadeada só pra frente (sem ponteiro `prev`), então achar o penúltimo nó exige percorrer tudo. Viraria O(1) com lista duplamente encadeada, mas isso dobra o custo de memória por nó.

> **Atenção:** como o retorno é `void*`, não dá pra diferenciar "lista vazia" de "valor `NULL` guardado de propósito". Se seu uso guarda `NULL` como valor válido, cheque `ll_is_empty`/`list->size` antes.

### Consulta

| Função | Descrição | Complexidade |
|---|---|---|
| `void* ll_get_first(LinkedList* list)` | Olha o primeiro valor sem remover. | O(1) |
| `void* ll_get_last(LinkedList* list)` | Olha o último valor sem remover. | O(1) |
| `void* ll_get(LinkedList* list, size_t index)` | Acessa o valor na posição `index`, tipo `array[index]`. | O(n) |
| `long ll_index_of(LinkedList* list, void* value, LLCompareFn cmp)` | Posição da 1ª ocorrência de `value`, ou `-1`. | O(n) |
| `int ll_contains(LinkedList* list, void* value, LLCompareFn cmp)` | `1` se `value` existe na lista. | O(n) |

`LLCompareFn` segue a convenção `memcmp`/`strcmp`: `0` significa "igual".

```c
int cmp_int(const void* a, const void* b) {
    return *(const int*)a - *(const int*)b;
}
```

### Percurso

```c
typedef void (*LLForEachCallback)(void* value, void* context);
void ll_for_each(LinkedList* list, LLForEachCallback callback, void* context);
```

Percorre a lista chamando `callback` para cada valor, sem expor o nó interno (`LLNode`). `context` é opcional (pode ser `NULL`) — serve pra acumular estado (soma, contador etc.) sem variável global.

### Liberação de memória

| Função | Descrição |
|---|---|
| `void ll_clear(LinkedList* list)` | Remove todos os nós, **não** libera os valores. A `LinkedList` continua válida e reutilizável (`head`/`tail` viram `NULL`, `size` vira `0`). |
| `void ll_clear_with_values(LinkedList* list, LLFreeValueFn free_fn)` | Igual acima, mas chama `free_fn` em cada valor antes de liberar o nó. |
| `void ll_destroy(LinkedList* list)` | Libera nós + a struct `LinkedList`. **Não** libera os valores. Ponteiro `list` fica inválido depois. |
| `void ll_destroy_with_values(LinkedList* list, LLFreeValueFn free_fn)` | Igual acima, mas libera os valores também via `free_fn`. |

```c
typedef void (*LLFreeValueFn)(void* value);
```

## Exemplo rápido

```c
#include "libcds/linked_list.h"
#include <stdio.h>

int cmp_int(const void* a, const void* b) {
    return *(const int*)a - *(const int*)b;
}

void print_int(void* value, void* context) {
    (void)context;
    printf("%d ", *(int*)value);
}

int main() {
    LinkedList* list = ll_create();

    int a = 1, b = 2, c = 3;
    ll_add_last(list, &a);
    ll_add_last(list, &b);
    ll_add_last(list, &c);

    ll_for_each(list, print_int, NULL); // 1 2 3

    printf("\ncontains(2) = %d\n", ll_contains(list, &b, cmp_int));

    ll_destroy(list); // valores são locais (stack), não precisa liberar
    return 0;
}
```

## Convenções de nomenclatura

Funções e tipos da lista usam o prefixo `ll_`/`LL` (`ll_create`, `LLNode`, `LLCompareFn`...) de propósito: como a lib vai crescer com outras estruturas (`stack`, `queue`...), nomes genéricos colidiriam se dois headers forem incluídos no mesmo arquivo `.c`.

## Requisitos

C99 ou superior (usa `static inline`, `for` com declaração de variável no header).