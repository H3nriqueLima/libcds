#pragma once

#include "linked_list.h"

/*
Pilha implementada por composição em cima da LinkedList, não com nós próprios. 
Faz sentido porque push/pop de uma pilha são exatamente add_first/remove_first de uma lista encadeada 
usada só por uma ponta, não há motivo para duplicar a lógica de malloc/free de nó que a LinkedList já resolve.

Diferente da Stack baseada em array (stack.h), aqui há as seguintes diferenças:
  - Não existe "capacity" nem realloc, cada push aloca um nó novo, cada pop libera um nó. 
    Sem crescimento por dobro, sem desperdício de slots não usados.
  - push/pop são O(1) no pior caso, não só amortizado. Nunca existe aquele push ocasional mais lento por causa 
    de um realloc grande (útil em código sensível a latência previsível, ex: tempo real).
  - Em troca, cada elemento é um malloc/free separado (mais overhead de alocador que um array) e fica espalhado 
    pela heap (pior uso de cache de CPU que um array contíguo).

Ou seja, Stack baseada em array tende a ser mais rápida na média (cache + menos chamadas de malloc), 
Linked Stack tem latência por operação mais previsível e nunca desperdiça memória reservada à toa. 
Para maioria dos casos, a Stack baseada em array é a escolha melhor. 
Essa aqui existe para quando o pior caso importa mais que a média, ou quando não faz sentido reservar 
capacidade extra.
*/
typedef struct {
    LinkedList* list; // encapsulado, quem usa LinkedStack nunca acessa isso direto.
} LinkedStack;

/*
Cria uma pilha encadeada vazia. 
Retorna NULL se algum malloc interno falhar (o da struct LinkedStack ou o da LinkedList por baixo), 
sempre checar antes de usar.
*/
static inline LinkedStack* ls_create() {
    LinkedStack* stack = (LinkedStack*)malloc(sizeof(LinkedStack));
    if (stack == NULL) return NULL;

    stack->list = ll_create();
    if (stack->list == NULL) {
        free(stack);
        return NULL;
    }

    return stack;
}

// Atalho para "lista interna vazia", só existe para deixar o código de quem usa mais legível.
static inline int ls_is_empty(LinkedStack* stack) {
    return stack == NULL || ll_is_empty(stack->list);
}

/*
Empilha value no topo - O(1) no pior caso (sempre, não existe realloc aqui, cada push é só um malloc de nó).

ll_add_first (por baixo) é void e falha em silêncio se o malloc do nó falhar, sem retornar aviso nenhum. 
Para ainda assim conseguir avisar o chamador (mesma garantia que stack_push da Stack baseada em array dá),
comparamos o size antes/depois da chamada, se não mudou, o push não aconteceu de verdade.

Retorna 1 em sucesso, 0 se a alocação do nó falhar.
*/
static inline int ls_push(LinkedStack* stack, void* value) {
    if (stack == NULL) return 0;

    size_t size_before = stack->list->size;
    ll_add_first(stack->list, value);

    return stack->list->size != size_before;
}

/*
Desempilha e retorna o valor do topo - O(1).
Retorna NULL se a pilha estiver vazia ou for NULL, mesma ressalva de sempre -> não dá para diferenciar isso 
de um valor NULL empilhado de propósito.
*/
static inline void* ls_pop(LinkedStack* stack) {
    if (stack == NULL) return NULL;

    return ll_remove_first(stack->list);
}

// Só olha o valor do topo, sem desempilhar - O(1). Mesma ressalva do NULL acima.
static inline void* ls_peek(LinkedStack* stack) {
    if (stack == NULL) return NULL;

    return ll_get_first(stack->list);
}

// Remove todos os nós, mas não libera os valores guardados. A LinkedStack continua válida e reutilizável depois.
static inline void ls_clear(LinkedStack* stack) {
    if (stack == NULL) return;
    
    ll_clear(stack->list);
}

/*
Igual ls_clear, mas libera cada valor com free_fn antes do nó.
Reusa LLFreeValueFn (de linked_list.h) em vez de definir um tipo novo idêntico, 
já dependemos da LinkedList mesmo, então não faz sentido duplicar o typedef.
*/
static inline void ls_clear_with_values(LinkedStack* stack, LLFreeValueFn free_fn) {
    if (stack == NULL) return;

    ll_clear_with_values(stack->list, free_fn);
}

/*
Libera a lista interna e a struct LinkedStack. Não libera os valores guardados, se foram alocados dinamicamente,
libere manualmente antes ou use ls_destroy_with_values. Ponteiro stack fica inválido depois.
*/
static inline void ls_destroy(LinkedStack* stack) {
    if (stack == NULL) return;

    ll_destroy(stack->list);
    free(stack);
}

// Igual ls_destroy, mas libera os valores também (via free_fn) antes de liberar cada nó.
static inline void ls_destroy_with_values(LinkedStack* stack, LLFreeValueFn free_fn) {
    if (stack == NULL) return;

    ll_destroy_with_values(stack->list, free_fn);
    free(stack);
}