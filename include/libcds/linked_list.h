#pragma once

#include <stdlib.h>

/* 
Nó interno da lista, não deve ser manipulado direto por quem usa a lib.
Prefixo LL em tudo (structs, typedefs, funções) para não colidir com stack.h, queue.h, etc. 
(caso eu, futuramente, adicione outros tipos de estruturas).
*/
typedef struct LLNode {
	void* value; // guarda um ponteiro genérico, a lista não sabe o tipo real.
	struct LLNode* next;
} LLNode;

/*
Struct pública. head/tail/size podem ser lidos direto (ex: list->size),
só não devem ser alterados manualmente fora das funções abaixo.
*/
typedef struct {
	LLNode* head;
	LLNode* tail; // guardado para add_last/get_last serem O(1).
	size_t size;
} LinkedList;

/*
Assinatura para o callback usado em ll_for_each.
O context é opcional (pode passar NULL) e serve para acumular estado (contador, soma, etc.),
sem precisar de variável global.
*/
typedef void (*LLForEachCallback)(void* value, void* context);

/*
Assinatura de uma função que libera um valor guardado na lista.
Usada em ll_destroy_with_values e ll_clear_with_values.
*/
typedef void (*LLFreeValueFn)(void* value);

/*
Assinatura de comparador, mesma convenção de memcmp/strcmp -> retorna 0 quando a e b são "iguais".
Usada em ll_index_of e ll_contains.
*/
typedef int (*LLCompareFn)(const void* a, const void* b);

/*
Cria uma lista vazia. Retorna NULL se o malloc falhar (sempre checar antes de usar).
*/
static inline LinkedList* ll_create() {
	LinkedList* list = (LinkedList*)malloc(sizeof(LinkedList));

	if (list != NULL) {
		list->head = NULL;
		list->tail = NULL;
		list->size = 0;
	}

	return list;
}

// Atalho para "list->size == 0", só existe para deixar o código de quem usa mais legível.
static inline int ll_is_empty(LinkedList* list) {
	return list == NULL || list->size == 0;
}

// Insere value no começo da lista - O(1).
static inline void ll_add_first(LinkedList* list, void* value) {
	if (list == NULL) return;

	LLNode* new_node = (LLNode*)malloc(sizeof(LLNode));
	if (new_node == NULL) return;

	new_node->value = value;
	new_node->next = list->head;

	list->head = new_node;

	// se a lista estava vazia, o novo nó também é o tail.
	if (list->tail == NULL) {
		list->tail = new_node;
	}

	list->size++;
}

/* 
Insere value no final da lista - O(1) porque a gente guarda o tail 
(sem isso teria que percorrer a lista inteira toda vez).
*/
static inline void ll_add_last(LinkedList* list, void* value) {
	if (list == NULL) return;

	LLNode* new_node = (LLNode*)malloc(sizeof(LLNode));
	if (new_node == NULL) return;

	new_node->value = value;
	new_node->next = NULL;

	if (list->tail == NULL) {
		// lista vazia -> o novo nó é head e tail ao mesmo tempo.
		list->head = new_node;
		list->tail = new_node;
	} else {
		list->tail->next = new_node;
		list->tail = new_node;
	}

	list->size++;
}

/*
Insere value na posição index(0 = começo, index >= size = final).
0(n) porque precisa percorrer até o nó anterior à posição.
*/
static inline void ll_insert_at(LinkedList* list, size_t index, void* value) {
	if (list == NULL) return;

	if (index == 0) {
		ll_add_first(list, value);
		return;
	}

	if (index >= list->size) {
		ll_add_last(list, value);
		return;
	}

	// acha o nó anterior à posição para encaixar o novo nó no meio.
	LLNode* prev = list->head;
	for (size_t i = 0; i < index - 1; i++) {
		prev = prev->next;
	}

	LLNode* new_node = (LLNode*)malloc(sizeof(LLNode));
	if (new_node == NULL) return;

	new_node->value = value;
	new_node->next = prev->next;
	prev->next = new_node;

	list->size++;
}

/*
Remove e retorna o valor do primeiro nó - O(1).
Retorna NULL se a lista estiver vazia ou for NULL, não dá para diferenciar isso de um valor 
NULL guardado de propósito, então cuidado se a lista puder guardar NULL como valor válido.
*/
static inline void* ll_remove_first(LinkedList* list) {
	if (list == NULL || list->head == NULL) return NULL;

	LLNode* node_to_remove = list->head;
	void* value = node_to_remove->value;

	list->head = list->head->next;
	if (list->head == NULL) {
		// lista ficou vazia, tail também precisa ser zerado.
		list->tail = NULL;
	}

	free(node_to_remove);
	list->size--;

	return value;
}

/*
Remove e retorna o valor de último nó. Ainda é O(n) porque a lista é encadeada só para frente (sem "prev" no nó),
para achar o penúltimo nó precisa percorrer tudo.
Para virar O(1) de verdade teria que ser lista duplamente encadeada, o que dobra o custo de memória por nó.
*/
static inline void* ll_remove_last(LinkedList* list) {
	if (list == NULL || list->head == NULL) return NULL;

	if (list->head->next == NULL) return ll_remove_first(list);

	LLNode* current = list->head;
	while (current->next->next != NULL) {
		current = current->next;
	}

	LLNode* last_node = current->next;
	void* value = last_node->value;

	current->next = NULL;
	list->tail = current;

	free(last_node);
	list->size--;

	return value;
}

/*
Remove e retorna o valor na posição index - O(n).
Delega para ll_remove_first/ll_remove_last nos casos de ponta, que são O(1).
*/
static inline void* ll_remove_at(LinkedList* list, size_t index) {
	if (list == NULL || index >= list->size) return NULL;

	if (index == 0) return ll_remove_first(list);
	if (index == list->size - 1) return ll_remove_last(list);

	LLNode* prev = list->head;
	for (size_t i = 0; i < index - 1; i++) {
		prev = prev->next;
	}

	LLNode* to_remove = prev->next;
	void* value = to_remove->value;

	prev->next = to_remove->next;

	free(to_remove);
	list->size--;

	return value;
}

// Só olha o valor do primeiro nó, sem remover - O(1).
static inline void* ll_get_first(LinkedList* list) {
	if (list == NULL || list->head == NULL) return NULL;

	return list->head->value;
}

// Só olha o valor do último nó, sem remover - O(1) por causa do tail.
static inline void* ll_get_last(LinkedList* list) {
	if (list == NULL || list->tail == NULL) return NULL;

	return list->tail->value;
}

/*
Acessa o valor na posição index, tipo um array[index]. 
O(n), porque lista encadeada não tem acesso direto por índice, se isso for muito usado no código, 
considerar se não seria melhor um array/vetor.
*/
static inline void* ll_get(LinkedList* list, size_t index) {
	if (list == NULL || index >= list->size) return NULL;

	LLNode* current = list->head;
	for (size_t i = 0; i < index; i++) {
		current = current->next;
	}

	return current->value;
}

/*
Procura value na lista usando cmp para comparar, retorna a posição da primeira ocorrência ou -1 se não achar.
O(n).
*/
static inline long ll_index_of(LinkedList* list, void* value, LLCompareFn cmp) {
	if (list == NULL || cmp == NULL) return -1;

	LLNode* current = list->head;
	long index = 0;

	while (current != NULL) {
		if (cmp(current->value, value) == 0) return index;
		current = current->next;
		index++;
	}

	return -1;
}

// Só checa se value existe na lista (usa ll_index_of por baixo).
static inline int ll_contains(LinkedList* list, void* value, LLCompareFn cmp) {
	return ll_index_of(list, value, cmp) != -1;
}

/*
Percorre a lista do início ao fim chamando callback para cada valor.
Existe para quem usa a lib não precisar acessar LLNode diretamente.
*/
static inline void ll_for_each(LinkedList* list, LLForEachCallback callback, void* context) {
	if (list == NULL || callback == NULL) return;

	LLNode* current = list->head;
	while (current != NULL) {
		callback(current->value, context);
		current = current->next;
	}
}

/*
Remove todos os nós, mas não libera os valores guardados (void* pode ser qualquer coisa, 
a lista não sabe como liberar). 
A struct LinkedList continua válida depois, dá para usar de novo sem precisar chamar ll_create outra vez.
*/
static inline void ll_clear(LinkedList* list) {
	if (list == NULL) return;

	LLNode* current = list->head;
	while (current != NULL) {
		LLNode* next = current->next;
		free(current);
		current = next;
	}

	list->head = NULL;
	list->tail = NULL;
	list->size = 0;
}

/*
Igual ll_clear, mas chama free_fn em cada valor antes de liberar o nó.
Usar quando os valores foram alocados dinamicamente e "pertencem" à lista (a lista é dona da memória deles).
*/
static inline void ll_clear_with_values(LinkedList* list, LLFreeValueFn free_fn) {
	if (list == NULL) return;

	LLNode* current = list->head;
	while (current != NULL) {
		LLNode* next = current->next;

		if (free_fn != NULL) {
			free_fn(current->value);
		}

		free(current);
		current = next;
	}

	list->head = NULL;
	list->tail = NULL;
	list->size = 0;
}

/*
Libera todos os nós e a struct LinkedList. Não libera os valores guardados (void*), se eles foram alocados 
dinamicamente, precisa liberar cada um manualmente antes de chamar isso, ou usar ll_destroy_with_values.
Depois de chamar, o ponteiro list fica inválido (dangling), não usar de novo.
*/
static inline void ll_destroy(LinkedList* list) {
	if (list == NULL) return;

	ll_clear(list);
	free(list);
}

// Igual ll_destroy, mas libera os valores também (via free_fn) antes de liberar cada nó.
static inline void ll_destroy_with_values(LinkedList* list, LLFreeValueFn free_fn) {
	if (list == NULL) return;

	ll_clear_with_values(list, free_fn);
	free(list);
}