#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>


// Estrutra para guardar o recurso
typedef struct Recurso{
  struct Recurso *proximo; // Ponteiro pro proximo recurso da fila caso exista um
  int data; // valor de quantidade de recurso
}recurso;

// Estrutura para a fila com todos os recursos disponíveis
typedef struct FilaRecurso{
  recurso *dados; // Ponteiro pro primeiro elemento da fila
  recurso *ultimo; // Ponteiro pro ultimo elemento da fila
  int tamanho; // Tamanho da fila
}filaRecursos;

// Definindo nossa estrutura de cliente
typedef struct cliente{
    filaRecursos *max;      // fila que contem o valor máximo de recursos que o cliente pode pedir
    filaRecursos *alocados; // fila que contem o valor de recursos alocados ao cliente no momento
    int tReq; // valor para a quantidade de ciclos até a próxima requisição do cliente
    int tFim; // valor para a quantidade de ciclos até que o cliente libere os recursos que foram alocados
}cliente;

// Inicializa as variaveis que talvez vamos precisar
int ciclos; // Quantidade de ciclos do programa
pthread_mutex_t trava; // trava mutex
pthread_cond_t condicional; // condicional

// Esqueleto para as funções
filaRecursos* criarFila();
int adicionar(filaRecursos *fila, int info);
void print_fila(filaRecursos *fila);

// Main com argc | argv
int main(int argc, char const *argv[]) {
    filaRecursos *recursos = criarFila(); // Fila de recursos do programa
    
    for(int i = 1; i < argc-1; i++) // Pega todos os argumentos passados até o penultimo
        adicionar(recursos, atoi(argv[i])); // adiciona todos os argumentos na fila de recursos
        
    ciclos = atoi(argv[argc-1]); // pega o numero de ciclos do ultimo argumento de argv
    
    print_fila(recursos);
    printf("\nCiclos = %d", ciclos);
}

// Função para criar a fila vazia
filaRecursos* criarFila(){
  filaRecursos *fila;
  fila = malloc(sizeof(filaRecursos));
  if(fila != NULL){
    fila->tamanho = 0;
    fila->dados = NULL;
    fila->ultimo = NULL;
  }
  return(fila);
}

// Função para adicionar um elemento na fila sempre na ultima posição vazia
int adicionar(filaRecursos *fila, int info){
  recurso *novo;
  novo = malloc(sizeof(recurso));
  fila->tamanho = fila->tamanho + 1;
  novo->data = info;

  if(fila->dados == NULL){
    fila->dados = novo;
    fila->ultimo = novo;
    novo->proximo = NULL;
  }else{
    fila->ultimo->proximo = novo;
    fila->ultimo = novo;
    novo->proximo = NULL;
  }
  return(fila->tamanho);
}

// Função para imprimir os valores de uma fila
void print_fila(filaRecursos *fila){
    recurso *temp = fila->dados;

    for(temp = fila->dados; temp != NULL; temp = temp->proximo){
        if(temp->proximo == NULL){
          printf("%i", temp->data);
        }else{
          printf("%i ", temp->data);
        }
    }
    return;
}
