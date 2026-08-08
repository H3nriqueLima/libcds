/*
Exemplo de uso da stack.h da libcds.

Dois usos de pilha:
  1) Balanceamento de parênteses/colchetes/chaves (LIFO puro, valores na stack de verdade sao só caracteres, 
     não precisam de free).
  2) Undo de um editor de texto simples (valores alocados com malloc, mostrando stack_destroy_with_values).

Compilar (a partir da raiz do repositório libcds): 
gcc -Wall -Wextra -std=c99 -I include examples/example_stack.c -o example_stack
*/

#include "libcds/stack.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>

// Exemplo 1 - Balanceamento de parênteses.

/*
Verifica se toda abertura de (), [] ou {} tem o fechamento correto, na ordem certa. Cada abertura empilha, cada
fechamento espera achar a abertura certa no topo.
*/
int is_balanced(const char* expression) {
    Stack* stack = stack_create(0);
    if (stack == NULL) return 0;

    int balanced = 1;

    for (size_t i = 0; expression[i] != '\0' && balanced; i++) {
        char c = expression[i];

        if (c == '(' || c == '[' || c == '{') {
            /*
            guarda o próprio caractere empilhado. Como é só um char, não vale a pena alocar memória para ele, 
            assim guardamos o valor em cast direto no ponteiro em vez de um endereço de verdade 
            (funciona porque nunca vamos desreferenciar esse "ponteiro", só comparar o valor numérico dele).
            */
            stack_push(stack, (void*)(intptr_t)c);
        }
        else if (c == ')' || c == ']' || c == '}') {
            if (stack_is_empty(stack)) {
                balanced = 0; // fechou algo que nunca abriu.
                break;
            }

            char opener = (char)(intptr_t)stack_pop(stack);
            char expected = (c == ')') ? '(' : (c == ']') ? '[' : '{';

            if (opener != expected) balanced = 0; // fechou o tipo errado (ex: "(]").
        }
    }

    // se sobrou algo empilhado, teve abertura sem fechamento.
    if (!stack_is_empty(stack)) balanced = 0;

    stack_destroy(stack); // não há valores alocados para liberar aqui.
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

    printf("Exemplo 1: balanceamento de parenteses\n");
    for (size_t i = 0; i < sizeof(tests) / sizeof(tests[0]); i++) {
        printf("  \"%s\" -> %s\n", tests[i], is_balanced(tests[i]) ? "balanceado" : "quebrado");
    }
}

// Exemplo 2 - Undo de um editor de texto simples.

// cada "ação" guardada no undo é o texto completo antes da edição.
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
    printf("Exemplo 2 - Undo de editor (pilha com valores alocados)");

    Stack* undo_stack = stack_create(0);

    char current[128] = "";

    const char* edits[] = {
        "Ola",
        "Ola, mundo",
        "Ola, mundo!",
        "Ola, mundo! Tudo bem?",
    };

    for (size_t i = 0; i < sizeof(edits) / sizeof(edits[0]); i++) {
        // antes de aplicar a edição, empilha o estado atual para o undo.
        EditorState* snapshot = create_state(current);
        stack_push(undo_stack, snapshot);

        strncpy(current, edits[i], sizeof(current) - 1);
        current[sizeof(current) - 1] = '\0';

        printf("  editou -> \"%s\"\n", current);
    }

    printf("\n desfazendo 2 vezes:\n");
    for (int i = 0; i < 2; i++) {
        EditorState* previous = (EditorState*)stack_pop(undo_stack);
        strncpy(current, previous->text, sizeof(current) - 1);
        current[sizeof(current) - 1] = '\0';

        printf("  undo -> \"%s\"\n", current);

        free_state(previous); // quem chama pop é dono do valor.
    }

    // ainda sobraram estados empilhados, libera todos de uma vez.
    stack_destroy_with_values(undo_stack, free_state);
}

int main() {
    run_balance_examples();
    run_undo_example();
    
    return 0;
}