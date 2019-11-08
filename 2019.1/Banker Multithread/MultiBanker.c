#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

typedef struct{
    int *alocado;
    int id;
}toRelease;

// Trava Mutex e condicional para esperar a função safe
pthread_mutex_t trava;
pthread_cond_t cond;


// Instancia os nossos Arrays, Matrizes, E variáveis globais
int *recursos; // Quantidade de recursos disponíveis de cada tipo
int **Max; // Quantidade máxima de recursos que um cliente pode pedir
int **need; // Quantidade de recursos que um cliente vai pedir em sua próxima requisição
int **assigned; // Recursos que estão alocados no momento e para quem
int *work; // Para ser usado no algoritmo de safe
int *finish; // Para ser usado no algoritmo de safe
int recursoInicial; // Numero de recursos do programa
int quantClientes; // Numero de clientes do programa
int quantCiclos; // Quantos ciclos terá o programa
int numClientes; // variavel que vai receber quantos clientes tem o arquivo
int nRandomNumber; // Variavel que vai receber os numeros randomicos da nossa função

// Esqueleto das funções
int nClientes(FILE *filename);
void *require(void *v);
int safety();
int random_number(int min_num, int max_num);
int runThread(int idCliente, int nCiclo);
void *release(void *arg);


int main(int argc, char const *argv[])
{
    srand(time(NULL));
    // O numero de recursos que teremos é argc-2 porque o primeiro argumento é o nome do programa e o ultimo a quantidade de ciclos
    recursoInicial = argc-2;

    // Alocando o tamanho do nosso Array de recursos
    recursos = (int*)malloc(recursoInicial * sizeof(*recursos));

    // Work vai ter o mesmo tamanho do Array de recursos
    work = (int*)malloc(recursoInicial * sizeof(*work));

    // Uma linha para varrer nosso arquivo é uma string do tamanho do Buffer
    char line[BUFSIZ];

    // Dois contadores para iterar nosso arquivo de texto
    int counter = 0;   
    int counter2 = 0;

    // Guardar os valores dos recursos da linha de comando pro nosso Array
    for (int i = 1; i <= recursoInicial; i++)
    {
        recursos[i-1] = atoi(argv[i]);
    }
    
    // quantCiclos vai ser igual ao ultimo argumento de linha de comando
    quantCiclos = atoi(argv[argc-1]);

    // Abrindo o nosso arquivo para pegar o numero de clientes nele
    FILE *fp = fopen("text.txt", "r");
    numClientes = nClientes(fp);

    // Alocando a quantidade de colunas da nossa matriz Max
    Max = (int**)malloc(numClientes * sizeof(*Max));
    for (int i = 0; i < numClientes; i++)
    {
        // Alocando o tamanho de cada linha da nossa matriz Max
        Max[i] = (int*)malloc(recursoInicial * sizeof(**Max));
    }

    // Alocando a quantidade de colunas da nossa matriz Assigned
    assigned = (int**)malloc(numClientes * sizeof(*assigned));
    for (int i = 0; i < numClientes; i++)
    {
        // Alocando o tamanho de cada linha da nossa matriz Assigned
        assigned[i] = (int*)malloc(recursoInicial * sizeof(**assigned));
        for(int j = 0; j < recursoInicial; j++){
            assigned[i][j] = 0;
        }
    }
    
    // Alocando a quantidade de colunas da nossa matriz need
    need = (int**)malloc(numClientes * sizeof(*need));
    for (int i = 0; i < numClientes; i++)
    {
        // Alocando o tamanho de cada linha da nossa matriz need
        need[i] = (int*)malloc(recursoInicial * sizeof(**need));
        for(int j = 0; j < recursoInicial; j++){
            need[i][j] = 0;
        }
    }

    // Finish vai ser um array que recebe uma flag para cada cliente
    finish = (int*)malloc(numClientes * sizeof(*finish));
    
    // Fechando o arquivo
    fclose(fp);

    // Abrindo o arquivo para leitura novamente o que garante que voltamos para o inicio dele
    FILE * fp2 = fopen("text.txt", "r");    


    // Iterar o arquivo e salvar os valores de cada linha em uma coluna da nossa matriz Max
    while (fgets(line, sizeof line, fp2) != NULL) 
    {
        char *start = line;
        int field;
        int n;
            
        while (sscanf(start, "%d%n", &field, &n) == 1) 
        {
            Max[counter][counter2] = field;
            printf("%d ", Max[counter][counter2]);
            start += n;
            counter2++;
        }
        
        puts("");
        counter++;
        counter2 = 0;
    }
    // Fechando o arquivo novamente já que não vamos usar ele mais
    fclose(fp2);

    // Criando as nossas threads
    pthread_t threads[numClientes];
    pthread_attr_t attr;
    pthread_attr_init(&attr);

    // Criando um array que vai identificar um cliente por um Id único
    int clientes[numClientes];

    // Preenchendo o Array clientes
    for (int i = 0; i < numClientes; i++)
    {
        clientes[i] = i;
    }
    
    // Iniciando as nossas threads - Uma para cada Cliente
    for (int i = 0; i < numClientes; i++)
    {
        pthread_create(&threads[i], &attr, require, (void*)(&clientes[i]));
    }
    
    // Nossas threads terminaram de executar
    for (size_t i = 0; i < numClientes; i++)
    {
        pthread_join(threads[i], NULL);
    }
    
    // Dando free em toda a memória que alocamos
    free(recursos);
    for(int i=0; i < numClientes; i++) {
        free(assigned[i]);
        free(Max[i]);
	    free(need[i]);
    }
    free(assigned);
    free(Max);
	free(need);
    free(work);
    free(finish);
    printf("Programa finalizado corretamente!");
    return 0;
}

int nClientes(FILE *filename)
{
  // counta o numero de linhas no arquivo                                    
  int ch = 0;
  int lines = 0;

  if (filename == NULL){
    return 0;
  }

  lines++;
  while ((ch = fgetc(filename)) != EOF)
    {
      if (ch == '\n')
        lines++;
    }
  return lines;
}

void *require(void *v)
{
    int idCliente = *(int*)v; // Converte o parametro que recebemos para uma int
    
    // Executar todos os ciclos do programa
    for(int i = 0; i < quantCiclos; i++){
        pthread_mutex_lock(&trava); // Recebe a Trava Mutex ou fica esperando receber
        
        // Chama a função que vai rodar a nossa thread e recebe quanto tempo ela vai dormir até a próxima requisição
        int sleepTime = runThread(idCliente, i); 
        
        // Printa os recursos alocados atualmente para o cliente
        printf("Recursos alocados no momento para o cliente %d: ", idCliente);
        for(int i = 0; i < recursoInicial; i++){
            printf(" %d", assigned[idCliente][i]);
        }
        printf("\n");
        sleep(1); // Sleep para garantir que o print vai sair certo
        
        // libera o cond e o mutex
        pthread_cond_broadcast(&cond);
        pthread_mutex_unlock(&trava);
        
        // dorme pelo tempo que recebeu da função runThread
        sleep(sleepTime);
    }
    // Sai da thread
    pthread_exit(NULL);
}

int safety()
{
    // Armazenar qual seria a sequencia segura de execução das threads
    int safeSequence[numClientes];
    int clientePraSeq = 0;
    
    // work vai ser igual aos recursos disponiveis no momento
    for(int i =0; i < recursoInicial; i++){
        work[i] = recursos[i];
    }

    // flag finish = falso para todos os clientes
    for (int i = 0; i < numClientes; i++)
    {
        finish[i] = 0;
    }
  
    // terminados para verificar se ja passou por todos os recursos
    int terminados = 0;
    
    // iniciar a iteração para cada recurso
    while(terminados <= recursoInicial){
        // flag para se o sistema esta seguro ou não
        int safe = 0;

        // verifica cliente por cliente
        for(int i = 0; i < numClientes; i++){
            
            // se o cliente não tiver terminado ainda
            if(finish[i] == 0){
                // flag para dizer se ele pode ser colocado nesse local da sequencia
                int possivel = 1;
                
                // verifica se é possivel alocar todos os recursos que o cliente está pedindo
                for(int j = 0; j < numClientes; j++){
                    if(need[i][j] > work[j]){
                        possivel = 0;
                        break;
                    }
                }

                // se for possivel alocar os recursos
                if(possivel == 1){
                    // consideramos que os recursos vão ser liberados e devolvemos eles pro work
                    for(int j = 0; j < numClientes; j++){
                        work[j] += assigned[i][j];
                        // consideramos que o cliente terminou seu serviço
                        finish[i] = 1;
                        // aumentamos nosso contador em +1
                        terminados++;
                        // o estado esta safe
                        safe = 1;
                        // adicionamos o cliente na sequencia segura
                        safeSequence[clientePraSeq] = j;
                        clientePraSeq++;
                    }
                }
            }
        }
        // caso não tenha sido encontrada uma sequencia segura
        if(safe == 0){
            printf("Sequencia segura não encontrada, sistema entraria em deadlock\n");
            return 0;
        }
    }
    // caso seja encontrada uma sequencia segura
    printf("Sequencia segura encontrada:");
    sleep(1); // Sleep para garantir que o print vai sair certo
    for(int i = 0; i < numClientes; i++){
        printf(" %d", safeSequence[i]);
    }
    printf("\n");
    sleep(1); // Sleep para garantir que o print vai sair certo
    return 1;
}

// Função que vai gerar um numero Randomico entre o min e o max
int random_number(int min, int max)
{
    int result = 0, low = 0, high = 0;
    
    if(min == 0 && max == 0) return 0;

    if (min < max)
    {
        low = min;
        high = max + 1; // incluir max no output
    } else {
        low = max + 1; // incluir max no output
        high = min;
    }

    srand(time(NULL));
    result = (rand() % (high - low)) + low;
    return result;
}

/*
    Função para executar tudo da thread
    (Criamos ela para permitir rodar novamente no mesmo ciclo quando o random for 0)
    fazemos isso através do uso de recursão
*/
int runThread(int idCliente, int nCiclo){
        
        printf("Cliente: %d - No Ciclo: %d\n", idCliente, nCiclo);
        sleep(1); // Sleep para garantir que o print vai sair certo
        printf("Recursos requisitados nesse ciclo: ");
        sleep(1); // Sleep para garantir que o print vai sair certo
        
        
        // Cria aleatoriamente a quantidade de recursos que vamos requisitar
        for(int j = 0; j < recursoInicial; j++){
            /*
                Os recursos requisitados vão ser no máximo a quantidade máxima de recursos
                de um cliente menos os recursos que já foram alocados para ele
            */
            nRandomNumber = random_number(0, Max[idCliente][j]-assigned[idCliente][j]);
            need[idCliente][j] = nRandomNumber;
            printf(" %d", need[idCliente][j]);
        }
        printf("\n");
        sleep(1); // Sleep para garantir que o print vai sair certo
        
        /*
            chama nossa função de safety para verificar se vai ocorrer deadlock ou não
            e ao mesmo tempo vai chamar a thread novamente quando ela puder rodar sem
            causar deadlock
        */
        while(safety() != 1){
            pthread_cond_wait(&cond, &trava);
            printf("Tentando novamente com o cliente %d\n", idCliente);
            sleep(1); // sleep pro print sair certinho
        }    
        // retira os recursos do pool global           
        for(int k = 0; k < recursoInicial; k++)
            recursos[k] -= need[idCliente][k];    
         
        // Aloca os recursos para o cliente   
        for(int k = 0; k < recursoInicial; k++)
            assigned[idCliente][k] += need[idCliente][k];
        
        /*
            Cria um struct que vai receber as variaveis que queremos mandar para a thread
            que vai lidar com a liberação dos nossos recursos
        */
        toRelease *liberar = malloc(sizeof(toRelease));
        liberar->alocado = malloc(recursoInicial * sizeof(*liberar->alocado));
        liberar->id = idCliente;
        
        for(int i = 0; i < recursoInicial; i++)
            liberar->alocado[i] = need[idCliente][i];
        
        // Criando nossa threads de release
        pthread_t thread;
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        
        pthread_create(&thread, &attr, release, (void*)(liberar));
        sleep(3); // sleep para garantir que os print vão sair certo
        
        // pega o tempo de sleep da thread
        nRandomNumber = random_number(0,9);
        /*
            se o tempo de sleep for igual a zero executa uma nova requisição
            dentro do mesmo ciclo através de recursão
        */
        if(!nRandomNumber) runThread(idCliente, nCiclo);
        
        // retorna o tempo de sleep
        return nRandomNumber;
}

// função de execução das threads de liberação de recursos
void *release(void *arg){
    // recebe as variaveis em formato de struct
    toRelease *liberador = (toRelease*) arg;
    
    // vai passar as variaveis da struct para variaveis locais
    int paraLiberar[recursoInicial];
    for(int i = 0; i < recursoInicial; i++) paraLiberar[i] = liberador->alocado[i];
    int idCliente = liberador->id;
    free(liberador);
    
    // pega o tempo que vai demorar para liberar os recursos
    int sleepTime = random_number(0,9);
    
    // printa quanto tempo vai demorar para o cliente liberar os recursos
    printf("O cliente %d vai demorar %d segundos para liberar os seguintes recursos:", idCliente, sleepTime);
    for(int i = 0; i < recursoInicial; i++)
        printf(" %d", paraLiberar[i]);
    printf("\n");
    
    // dorme pelo tempo que foi calculado
    sleep(sleepTime);
    
    // espera o mutex liberar para poder alterar as pools globais
    pthread_mutex_lock(&trava);
    
    // printa a liberação
    printf("Liberando os seguintes recursos do cliente %d:", idCliente);
    sleep(1); // sleep para garantir os prints
    
    // fala quanto recurso vai liberar
    for(int i = 0; i < recursoInicial; i++)
        printf(" %d", paraLiberar[i]);
    printf("\n");
    sleep(1); // sleep para garantir os prints
    
    // libera os recursos
    for(int j = 0; j < recursoInicial; j++){
        assigned[idCliente][j] -= paraLiberar[j];
        recursos[j] += paraLiberar[j];
    }    
    sleep(1); // sleep para garantir os prints
    
    // libera o mutex e finaliza a thread
    pthread_mutex_unlock(&trava);
    pthread_exit(NULL);
}
