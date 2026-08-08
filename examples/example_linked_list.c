/*
Exemplo de uso da linked_list.h da libcds.

Objetivo: mostrar as funções principais em um cenário realista (uma lista de "tarefas"), 
incluindo o caso mais delicado: valores alocados dinamicamente (malloc) e como liberá-los certo 
com destroy_with_values.

Compilar (a partir da raiz do repositório libcds): 
gcc -Wall -Wextra -std=c99 -I include examples/example_linked_list.c -o example_linked_list
*/

#include "libcds/linked_list.h"
#include <stdio.h>
#include <string.h>

// Struct de exemplo (Tarefa).
typedef struct {
    char title[64];
    int done;
} Task;


// Funções auxiliares para esse exemplo.

Task* create_task(const char* title) {
    Task* task = (Task*)malloc(sizeof(Task));

    if (task == NULL) return NULL;

    strncpy(task->title, title, sizeof(task->title) - 1);

    task->title[sizeof(task->title) - 1] = '\0';
    task->done = 0;

    return task;
}

void print_task(void* value, void* context) {
    (void)context; // não usamos context nesse exemplo, mas a assinatura pede.

    Task* task = (Task*)value;

    printf("  [%s] %s\n", task->done ? "x" : "", task->title);
}

// Comparador para ll_index_of/ll_contains (duas tasks são "iguais" se o título for igual).
int compare_task_title(const void* a, const void* b) {
    const Task* task_a = (const Task*)a;
    const Task* task_b = (const Task*)b;

    return strcmp(task_a->title, task_b->title);
}

// Callback para ll_destroy_with_values (como liberar uma Task).
void free_task(void* value) {
    free(value); // Task não tem nenhum ponteiro interno alocado, free simples resolve.
}

// Execução
int main() {
    LinkedList* tasks = ll_create();

    if (tasks == NULL) {
        fprintf(stderr, "falha ao criar a lista\n");
        return 1;
    }

    // inserindo tarefas.
    ll_add_last(tasks, create_task("Fazer tarefa de casa"));
    ll_add_last(tasks, create_task("Arrumar a cama"));
    ll_add_last(tasks, create_task("Almocar"));
    ll_add_first(tasks, create_task("Tomar cafe")); // essa entra na frente.

    printf("Lista apos insercoes (size = %zu):\n", tasks->size);
    ll_for_each(tasks, print_task, NULL);

    // marcando uma tarefa como concluída, buscando por título.
    Task search_key;
    strncpy(search_key.title, "Tomar cafe", sizeof(search_key.title));

    long index = ll_index_of(tasks, &search_key, compare_task_title);

    if (index != -1) {
        Task* found = (Task*)ll_get(tasks, (size_t)index);
        found->done = 1;
    }

    printf("\nApos marcar 'Tomar cafe' como feita:\n");
    ll_for_each(tasks, print_task, NULL);

    // inserindo no meio (posição 2).
    ll_insert_at(tasks, 2, create_task("Revisar tarefas"));

    printf("\nApos insert_at(2, \"Revisar tarefas\"):\n");
    ll_for_each(tasks, print_task, NULL);

    // removendo por posição.
    Task* removed = (Task*)ll_remove_at(tasks, 0);
    printf("\nRemovida da posicao 0: %s\n", removed->title);
    free_task(removed); // quem chama remove_* é dono do valor e precisa liberar.

    printf("\nLista final (size = %zu):\n", tasks->size);
    ll_for_each(tasks, print_task, NULL);

    // limpando tudo - como cada Task foi alocada com malloc (create_task), usamos a variante _with_values para não vazar memória.
    ll_destroy_with_values(tasks, free_task);

    return 0;
}