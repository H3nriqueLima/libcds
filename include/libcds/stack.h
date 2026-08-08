#pragma once

#include <stdlib.h>
#include <stdint.h> // SIZE_MAX, usado para checar overflow antes de dobrar a capacidade.

/*
Capacidade inicial do array interno quando stack_create(0) é chamado.
Escolhida por ser pequena o suficiente pra não desperdiçar memória em pilhas curtas e grande o suficiente 
para evitar realloc logo no começo.
*/
#define STACK_DEFAULT_CAPACITY 8

/*
Pilha baseada em array dinâmico, não em nós ligados por ponteiro (ao contrário da LinkedList). 
Só existe uma ponta ativa (o topo), então não faz sentido pagar o preço de um malloc por elemento, 
um array contíguo que cresce por dobro é mais rápido na prática:
- push/pop só mexem no final do array -> O(1) amortizado.
- sem malloc a cada push, só quando o array enche.
- memória contígua -> melhor uso de cache da CPU do que nós espalhados pela memória heap.

data/size/capacity podem ser lidos direto (ex: stack->size), só não devem ser alterados 
manualmente fora das funções abaixo.
*/
typedef struct {
	void** data; // array de ponteiros genéricos.
	size_t size; // quantos elementos tem empilhados agora.
	size_t capacity; // quantos elementos cabem sem precisar de realloc.
} Stack;

/*
Assinatura de uma função que libera um valor guardado na pilha.
Usada em stack_destroy_with_values e stack_clear_with_values.
*/
typedef void (*StackFreeValueFn)(void* value);

/*
Cria uma pilha vazia com a capacidade inicial pedida. Se initial_capacity for 0, usa STACK_DEFAULT_CAPACITY.
Retorna NULL se algum malloc falhar (sempre checar antes de usar).
*/
static inline Stack* stack_create(size_t initial_capacity) {
	if (initial_capacity == 0) initial_capacity = STACK_DEFAULT_CAPACITY;

	Stack* stack = (Stack*)malloc(sizeof(Stack));
	if (stack == NULL) return NULL;

	stack->data = (void**)malloc(initial_capacity * sizeof(void*));
	if (stack->data == NULL) {
		free(stack);
		return NULL;
	}

	stack->size = 0;
	stack->capacity = initial_capacity;

	return stack;
}

// Atalho para "stack->size == 0", só existe para deixar o código de quem usa mais legível.
static inline int stack_is_empty(Stack* stack) {
	return stack == NULL || stack->size == 0;
}

/*
Empilha value no topo. O(1) amortizado
só é O(n) nas vezes que precisa dobrar a capacidade via realloc (que são raras).

Diferente das funções de LinkedList (que "falham em silêncio" se o malloc falhar), 
essa retorna int porque aqui uma falha de realloc significa que o valor NÃO foi empilhado, 
o chamador precisa saber disso para não achar que empilhou algo que não empilhou.

Retorna 1 em sucesso, 0 se a alocação falhar (a pilha continua com o conteúdo anterior, só não cresce).
*/
static inline int stack_push(Stack* stack, void* value) {
	if (stack == NULL) return 0;

	if (stack->size == stack->capacity) {
		/*
		dobra a capacidade, garante O(1) amortizado mesmo com muitos pushes seguidos 
		(em vez de crescer +1 por vez, o que seria O(n) por push no pior caso.
		*/
		size_t new_capacity = stack->capacity * 2;

		/*
		checagem de overflow, se "capacity * 2" estourou size_t, 
		o resultado dá menor que o valor original (deu a volta). 
		E mesmo sem estourar aqui, "new capacity * sizeof(void*)" (linha de baixo) pode estourar sozinho. 
		Sem essa checagem, o realloc alocaria menos bytes do que o código pensa que alocou, 
		e o próximo acesso a data[i] escreveria fora do array (heap overflow silencioso). 
		*/
		if (new_capacity < stack->capacity || new_capacity > SIZE_MAX / sizeof(*stack->data)) return 0;

		void** new_data = (void**)realloc(stack->data, new_capacity * sizeof(void*));
		
		if (new_data == NULL) return 0;
		
		stack->data = new_data;
		stack->capacity = new_capacity;
	}

	stack->data[stack->size] = value;
	stack->size++;

	return 1;
}

/*
Desempilha e retorna o valor do topo - O(1).
Retorna NULL se a pilha estiver vazia ou for NULL - não dá para diferenciar isso de um 
valor NULL empilhado de propósito, então cuidado se seu uso puder empilhar NULL como valor válido.
*/
static inline void* stack_pop(Stack* stack) {
	if (stack == NULL || stack->size == 0) return NULL;

	stack->size--;
	
	return stack->data[stack->size];
}

// Só olha o valor do topo, sem desempilhar - O(1). Mesma ressalva do NULL acima.
static inline void* stack_peek(Stack* stack) {
	if (stack == NULL || stack->size == 0) return NULL;

	return stack->data[stack->size - 1];
}

/*
Zera a pilha (size = 0), mas não libera os valores guardados nem encolhe o array interno. 
A capacity é mantida de propósito, pra reusar a pilha sem pagar realloc de novo caso ela volte a crescer.
*/
static inline void stack_clear(Stack* stack) {
	if (stack == NULL) return;

	stack->size = 0;
}

// Igual stack_clear, mas chama free_fn em cada valor restante antes de zerar.
static inline void stack_clear_with_values(Stack* stack, StackFreeValueFn free_fn) {
	if (stack == NULL) return;

	if (free_fn != NULL) {
		for (size_t i = 0; i < stack->size; i++) {
			free_fn(stack->data[i]);
		}
	}

	stack->size = 0;
}

/*
Libera o array interno e a struct Stack. Não libera os valores guardados (void*). 
Se eles foram alocados dinamicamente, precisa liberar cada um manualmente antes, 
ou usar stack_destroy_with_values. 
Depois de chamar, o ponteiro stack fica inválido (dangling), não usar de novo.
*/
static inline void stack_destroy(Stack* stack) {
	if (stack == NULL) return;

	free(stack->data);
	free(stack);
}

// Igual stack_destroy, mas libera os valores também (via free_fn) antes de liberar o array.
static inline void stack_destroy_with_values(Stack* stack, StackFreeValueFn free_fn) {
	if (stack == NULL) return;

	if (free_fn != NULL) {
		for (size_t i = 0; i < stack->size; i++) {
			free_fn(stack->data[i]);
		}
	}

	free(stack->data);
	free(stack);
}