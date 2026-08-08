# libcds

Biblioteca de estruturas de dados genéricas em C, **header-only**. Cada estrutura vive em um único `.h` dentro de `include/libcds/`, sem `.c` para compilar separada, basta incluir o header.

## Estrutura do projeto

```
libcds/
├── examples/            # exemplos de uso
    └── example_linked_list.c
    └── example_stack.c
    └── example_linked_stack.c
└── include/
    └── libcds/
        └── linked_list.h
        └── stack.h
        └── linked_stack.h
```

## Como usar em outro projeto

Por ser header-only, não tem lib para linkar, só precisa que o compilador ache o diretório `include/`. Algumas formas de fazer isso:

### 1. Copiar direto (mais simples)

Copie `include/libcds/` para dentro do seu projeto e inclua:

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

Tipo público: `LinkedList` (`head`, `tail`, `size`, os três podem ser lidos direto, ex. `list->size`, mas não devem ser alterados manualmente fora das funções abaixo).

Todos os valores guardados são `void*`, a lista não sabe o tipo real, então **quem usa a lib é responsável por fazer o cast de volta** e por gerenciar a memória dos valores (a lista só gerencia seus próprios nós).

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

Percorre a lista chamando `callback` para cada valor, sem expor o nó interno (`LLNode`). `context` é opcional (pode ser `NULL`), serve para acumular estado (soma, contador etc.) sem variável global.

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

## API — `stack.h`
 
Pilha baseada em **array dinâmico** (não em nós ligados por ponteiro), push/pop só mexem no topo, e o array dobra de capacidade quando enche (ver a comparação com `linked_stack.h` no final desta seção).
 
Tipo público: `Stack` (`data`, `size`, `capacity` — podem ser lidos direto, ex. `stack->size`, mas não devem ser alterados manualmente fora das funções abaixo).
 
### Criação e estado
 
| Função | Descrição | Complexidade |
|---|---|---|
| `Stack* stack_create(size_t initial_capacity)` | Cria uma pilha vazia. Se `initial_capacity` for `0`, usa `STACK_DEFAULT_CAPACITY` (8). Retorna `NULL` se `malloc` falhar. | O(1) |
| `int stack_is_empty(Stack* stack)` | `1` se vazia ou `NULL`, `0` caso contrário. | O(1) |
 
### Push / Pop / Peek
 
| Função | Descrição | Complexidade |
|---|---|---|
| `int stack_push(Stack* stack, void* value)` | Empilha `value` no topo. Retorna `1` em sucesso, `0` se a alocação falhar (dobra de capacidade ou overflow de `size_t`). | O(1) amortizado |
| `void* stack_pop(Stack* stack)` | Desempilha e retorna o valor do topo. `NULL` se vazia. | O(1) |
| `void* stack_peek(Stack* stack)` | Olha o valor do topo sem remover. `NULL` se vazia. | O(1) |
 
> **Atenção:** mesma ressalva da `LinkedList`, `NULL` de retorno não
> diferencia "pilha vazia" de "valor `NULL` empilhado de propósito".
> `stack_push` retorna `int` (diferente da maioria das funções da lib,
> que falham em silêncio) porque aqui uma falha de alocação significa
> que o valor **não foi** empilhado, importante o chamador saber disso.
 
### Liberação de memória
 
| Função | Descrição |
|---|---|
| `void stack_clear(Stack* stack)` | Zera `size`, **não** libera os valores nem encolhe `capacity` (fica pronta pra reusar sem realloc). |
| `void stack_clear_with_values(Stack* stack, StackFreeValueFn free_fn)` | Igual acima, mas chama `free_fn` em cada valor restante antes de zerar. |
| `void stack_destroy(Stack* stack)` | Libera o array interno + a struct `Stack`. **Não** libera os valores. Ponteiro fica inválido depois. |
| `void stack_destroy_with_values(Stack* stack, StackFreeValueFn free_fn)` | Igual acima, mas libera os valores também via `free_fn`. |
 
```c
typedef void (*StackFreeValueFn)(void* value);
```
 
### Exemplo rápido
 
```c
#include "libcds/stack.h"
#include <stdio.h>
 
int main() {
    Stack* stack = stack_create(0);
 
    int a = 1, b = 2, c = 3;
    stack_push(stack, &a);
    stack_push(stack, &b);
    stack_push(stack, &c);
 
    printf("%d\n", *(int*)stack_pop(stack));  // 3
    printf("%d\n", *(int*)stack_peek(stack)); // 2 (nao remove)
 
    stack_destroy(stack); // valores sao locais (stack), nao precisa liberar
    return 0;
}
```

## API — `linked_stack.h`
 
Pilha implementada por **composição** em cima da `LinkedList` (o header já inclui `linked_list.h` sozinho), cada `push`/`pop` é um `ll_add_first`/`ll_remove_first` por baixo, sem `capacity` nem realloc.
 
Tipo público: `LinkedStack` (`list` — um `LinkedList*` interno; não deveria ser acessado diretamente fora das funções abaixo, só é exposto porque C não tem `private` de verdade).
 
### Criação e estado
 
| Função | Descrição | Complexidade |
|---|---|---|
| `LinkedStack* ls_create(void)` | Cria uma pilha vazia. Retorna `NULL` se algum `malloc` interno falhar. | O(1) |
| `int ls_is_empty(LinkedStack* stack)` | `1` se vazia ou `NULL`, `0` caso contrário. | O(1) |
 
### Push / Pop / Peek
 
| Função | Descrição | Complexidade |
|---|---|---|
| `int ls_push(LinkedStack* stack, void* value)` | Empilha `value` no topo (aloca um nó novo). Retorna `1` em sucesso, `0` se o `malloc` do nó falhar. | O(1) sempre, sem amortização |
| `void* ls_pop(LinkedStack* stack)` | Desempilha e retorna o valor do topo. `NULL` se vazia. | O(1) |
| `void* ls_peek(LinkedStack* stack)` | Olha o valor do topo sem remover. `NULL` se vazia. | O(1) |
 
> `ll_add_first` (usada por baixo do `ls_push`) é `void` e falha em
> silêncio se o `malloc` do nó falhar. Pra ainda assim sinalizar a
> falha pro chamador, `ls_push` compara o `size` da lista interna
> antes/depois da chamada.
 
### Liberação de memória
 
| Função | Descrição |
|---|---|
| `void ls_clear(LinkedStack* stack)` | Remove todos os nós, não libera os valores. Continua válida e reutilizável. |
| `void ls_clear_with_values(LinkedStack* stack, LLFreeValueFn free_fn)` | Igual acima, mas libera cada valor com `free_fn`. |
| `void ls_destroy(LinkedStack* stack)` | Libera a lista interna + a struct `LinkedStack`. Não libera os valores. |
| `void ls_destroy_with_values(LinkedStack* stack, LLFreeValueFn free_fn)` | Igual acima, mas libera os valores também. |
 
Reusa `LLFreeValueFn` de `linked_list.h` em vez de definir um tipo novo idêntico, já que `linked_stack.h` depende da `LinkedList` mesmo.
 
### Exemplo rápido
 
```c
#include "libcds/linked_stack.h"
#include <stdio.h>
 
int main() {
    LinkedStack* stack = ls_create();
 
    int a = 1, b = 2, c = 3;
    ls_push(stack, &a);
    ls_push(stack, &b);
    ls_push(stack, &c);
 
    printf("%d\n", *(int*)ls_pop(stack));  // 3
    printf("%d\n", *(int*)ls_peek(stack)); // 2 (nao remove)
 
    ls_destroy(stack); // valores sao locais (stack), nao precisa liberar
    return 0;
}
```

### `stack.h` vs `linked_stack.h` — qual usar
 
| | `Stack` (array) | `LinkedStack` (encadeada) |
|---|---|---|
| Push/pop | O(1) amortizado (realloc ocasional) | O(1) sempre, sem exceção |
| Alocação | Só nos momentos de crescimento | Um malloc/free por elemento, sempre |
| Cache da CPU | Ótimo (memória contígua) | Pior (nós espalhados na heap) |
| Memória ociosa | Pode sobrar capacidade não usada após um pico | Nunca sobra, cada nó existe só enquanto o valor existe |
| Melhor pra | Uso geral, throughput médio (escolha default) | Latência previsível por operação, ou quando reservar capacidade extra não faz sentido |

## Requisitos

C99 ou superior (usa `static inline`, `for` com declaração de variável no header).
