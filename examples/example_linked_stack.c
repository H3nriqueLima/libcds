/*
Exemplo de uso da linked_stack.h da libcds.

Mesmos dois cenários do example_stack.c (balanceamento de parênteses e undo de editor), 
agora usando LinkedStack em vez da Stack baseada em array. A API muda muito pouco (ls_ em vez de stack_), 
o que mostra na prática como as duas pilhas sao intercambiáveis do ponto de vista de quem usa, 
mesmo com implementações internas bem diferentes.

Compilar (a partir da raiz do repositório libcds):
gcc -Wall -Wextra -std=c99 -I include examples/example_linked_stack.c -o example_linked_stack
*/

#include "libcds/linked_stack.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>

// Exemplo 1: balanceamento de parênteses.

int is_balanced(const char* expression) {
	LinkedStack* stack = ls_create();
	if (stack == NULL) return 0;

	int balanced = 1;

	for (size_t i = 0; expression[i] != '\0' && balanced; i++) {
		char c = expression[i];

		if (c == '(' || c == '[' || c == '{') {
			/*
			mesmo truque do example_stack.c -> guarda o char em cast direto no ponteiro, sem malloc, 
			já que nunca vamos desreferenciar de verdade.
			*/
			ls_push(stack, (void*)(intptr_t)c);
		}
		else if (c == ')' || c == ']' || c == '}') {
			if (ls_is_empty(stack)) {
				balanced = 0; // fechou algo que nunca abriu.
				break;
			}

			char opener = (char)(intptr_t)ls_pop(stack);
			char expected = (c == ')') ? '(' : (c == ']') ? '[' : '{';

			if (opener != expected) {
				balanced = 0; // fechou o tipo errado (ex: "(]").
			}
		}
	}

	if (!ls_is_empty(stack)) {
		balanced = 0; // sobrou abertura sem fechamento.
	}

	ls_destroy(stack); // sem valores alocados para liberar aqui.

	return balanced;
}

void run_balance_examples() {
	const char* tests[] = {
		"(a + b) * [c - d]",
		"{[()]}",
		"(a + b]",
		"((a + b)",
		"a + b) + (c",
	};

	printf("Exemplo 1: balanceamento de parenteses (LinkedStack)\n");
	for (size_t i = 0; i < sizeof(tests) / sizeof(tests[0]); i++) {
		printf("  \"%s\" -> %s\n", tests[i], is_balanced(tests[i]) ? "balanceado" : "quebrado");
	}
}

// Exemplo 2: undo de um editor de texto simples

typedef struct {
	char text[128];
} EditorState;

EditorState* create_state(const char* text) {
	EditorState* state = (EditorState*)malloc(sizeof(EditorState));
	if (state == NULL) return NULL;

	strncpy(state->text, text, sizeof(state->text) - 1);
	state->text[sizeof(state->text) - 1] = '\0';

	return state;
}

void free_state(void* value) {
	free(value);
}

void run_undo_example() {
	printf("\nExemplo 2: undo de editor (LinkedStack com valores alocados)\n");

	LinkedStack* undo_stack = ls_create();

	char current[128] = "";

	const char* edits[] = {
		"Ola",
		"Ola, mundo",
		"Ola, mundo!",
		"Ola, mundo! Tudo bem?",
	};

	for (size_t i = 0; i < sizeof(edits) / sizeof(edits[0]); i++) {
		EditorState* snapshot = create_state(current);

		/*
		aqui checamos o retorno do push de propósito: diferente de ll_add_first (void), 
		ls_push avisa se a alocação do nó falhou.
		*/
		if (!ls_push(undo_stack, snapshot)) {
			fprintf(stderr, "falha ao empilhar snapshot, abortando\n");
			free_state(snapshot);
			break;
		}

		strncpy(current, edits[i], sizeof(current) - 1);
		current[sizeof(current) - 1] = '\0';

		printf("  editou -> \"%s\"\n", current);
	}

	printf("\n  desfazendo 2 vezes:\n");
	for (int i = 0; i < 2; i++) {
		EditorState* previous = (EditorState*)ls_pop(undo_stack);
		strncpy(current, previous->text, sizeof(current) - 1);
		current[sizeof(current) - 1] = '\0';

		printf("  undo -> \"%s\"\n", current);

		free_state(previous); // quem chama pop é dono do valor.
	}

	// ainda sobraram estados empilhados, libera todos de uma vez.
	ls_destroy_with_values(undo_stack, free_state);
}

int main() {
	run_balance_examples();
	run_undo_example();
	return 0;
}