#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <alloca.h>

#define FRAME_SIZE        256 // Tamanho do frame em bytes
#define FRAMES            256 // Numero de Frames
#define PAGE_MASK         65280 // Valor para page mask
#define OFFSET_MASK       255 // valor para offset mask
#define SHIFT_AMOUNT      8  // valor para usar em shift
#define TLB_SIZE          16 // tamanho da tlb
#define PAGES             256 // numero de paginas na page table

typedef struct{
  int pagina;
  int frame;
}tabela;

tabela TLBtable[FRAMES];
tabela PageTable[PAGES]; 
int fisica[FRAMES][FRAME_SIZE];

int contadorTLB = 0; // contador para o proximo endereço de entrada na tlb
int contadorPagina = 0;  // contador para a proxima pagina que vamos usar na page table
int contadorFrame = 0; // contador para o proximo frame que vamos usar na tlb ou page table
int flagContadorPagina = 0; //Vamos alterar se o array de Paginas for preenchido completamente 

int tlbCounter[PAGES];
int pageCounter[PAGES];

// Tornando nossas variáveis de entrada globais
FILE* address;
FILE* backing;
FILE* placeholder;

signed char buffer[FRAME_SIZE]; // buffer para leitura de arquivo bin
int logico; // endereço logico
int pageNumber; // numero da página
int offset; // valor do offset

// o valor traduzido do endereço logico
signed char traduzido;

int nAddr; // Contador de endereços traduzidos
int pageFaults = 0; // Contador de Page Faults
float pageFaultRate = 0; // Para o Rate de Page Faults
int TLBhitCount = 0; // Contador de hits na TLB
float TLBhitRate; // Para o Rate de Hits

int algoritmo;

// Protótipo das nossas funções
void iniciar();
void readFromStore(int pageNumber);
void tlbFIFO(int pageNumber, int frameNumber);
void tlbLRU(int pageNumber, int frameNumber);
void pageFifo();
void pageLru();

int main(int argc, char *argv[])
{
  backing = fopen("BACKING_STORE.bin", "rb");
  char *endereco= argv[1];
  address = fopen(endereco, "r");
  placeholder = fopen("output.txt", "w");

    if(!address){
      printf("Erro ao ler o arquivo de address.");
      return -1;
    }
    if(!backing){
      printf("Erro ao ler o arquivo backing store.");
      return -2;
    }
        
    if(!placeholder) {
      printf("Erro ao criar o arquivo de output.");
      return -3;
    }    
  
  if(strcmp(argv[2],"FIFO") == 0){
    algoritmo = 1;
  }else if(strcmp(argv[2], "LRU") == 0){
    algoritmo = 2;
  }else{
    printf("Você deve selecionar FIFO ou LRU!");
    fclose(address);
    fclose(backing);
    fclose(placeholder);
    return 0;
  }
  
  while (fscanf(address, "%d", &logico)==1) {
      pageNumber = (logico & PAGE_MASK) >> SHIFT_AMOUNT;

      offset = logico & OFFSET_MASK;

      iniciar();
      nAddr++;
  }

  fprintf(placeholder, "Number of Translated Addresses = %d\n", nAddr);
  fprintf(placeholder, "Page Faults = %d\n", pageFaults);
  pageFaultRate = pageFaults*1.0f/nAddr;
  fprintf(placeholder, "Page Fault Rate = %.3f\n", pageFaultRate);
  fprintf(placeholder, "TLB Hits = %d\n", TLBhitCount);
  TLBhitRate = TLBhitCount*1.0f / nAddr;
  fprintf(placeholder, "TLB Hit Rate = %.3f", TLBhitRate);

  
  fclose(address);
  fclose(backing);
  fclose(placeholder);

  return 0;
}

void iniciar()
{
    int frame_number = -1;

    for (int i = 0; i < TLB_SIZE; i++) {
        if (TLBtable[i].pagina == pageNumber) {
            frame_number = TLBtable[i].frame;
            TLBhitCount++;

            if(algoritmo == 2){
              for(int i = 0; i < PAGES; i++){
                if(pageNumber == PageTable[i].pagina){
                  pageCounter[i] = 0;
                  for(int k = 0; k < PAGES; k++){
                    if(k != i) pageCounter[k]++;
                  }
                  break;
                }
              }
            }

            break;
        }
    }

    if (frame_number == -1) {
      if(flagContadorPagina == 0){
        for(int i = 0; i < contadorPagina; i++){
            if(PageTable[i].pagina == pageNumber){
                frame_number = PageTable[i].frame; // 
                break;
            }
        }
        
        if(frame_number == -1) {  
            pageFaults++;   
            readFromStore(pageNumber);
            frame_number = contadorFrame - 1;
        }
      }else{
                for(int i = 0; i < FRAME_SIZE; i++){
            if(PageTable[i].pagina == pageNumber){
                frame_number = PageTable[i].frame; // 
                break;
            }
        }
        
        if(frame_number == -1) {  
            pageFaults++;   
            readFromStore(pageNumber);
            frame_number = contadorFrame - 1;
        }
      }
    }

    if (algoritmo == 1) {
        tlbFIFO(pageNumber, frame_number);
    }
    else {
        tlbLRU(pageNumber, frame_number);
    }

    traduzido = fisica[frame_number][offset];

    fprintf(placeholder, "Virtual address: %d ", logico);
    fprintf(placeholder, "Physical address: %d ", (frame_number << SHIFT_AMOUNT | offset));
    fprintf(placeholder, "Value: %d\n", traduzido);
}

void readFromStore(int pageNumber)
{
    
    if (fseek(backing, pageNumber * FRAME_SIZE, SEEK_SET) != 0) {
        printf("Erro dando seek no BACKING_STORE\n");
    }

    if (fread(buffer, sizeof(signed char), FRAME_SIZE, backing) == 0) {
        printf("Erro lendo o BACKING_STORE\n");
    }

    for (int i = 0; i < FRAME_SIZE; i++) {
        fisica[contadorFrame][i] = buffer[i];
    }  

    if(algoritmo == 1){
      pageFifo();
    }else{
      pageLru();
    }
}

void tlbFIFO(int pageNumber, int frameNumber)
{
    int i;

    for(i = 0; i < contadorTLB; i++){
        if(TLBtable[i].pagina == pageNumber){
            break;
        }
    }

    if(i == contadorTLB){
        if(contadorTLB < TLB_SIZE){
            TLBtable[contadorTLB].pagina = pageNumber;
            TLBtable[contadorTLB].frame = frameNumber;
        }
        else{      
            TLBtable[contadorTLB - 1].pagina = pageNumber;
            TLBtable[contadorTLB - 1].frame = frameNumber;

            for(i = 0; i < TLB_SIZE - 1; i++){
                TLBtable[i].pagina = TLBtable[i + 1].pagina;
                TLBtable[i].frame = TLBtable[i + 1].frame;
            }
        }
    }

    else{

        for(i = i; i < contadorTLB - 1; i++){
            TLBtable[i].pagina = TLBtable[i + 1].pagina;
            TLBtable[i].pagina = TLBtable[i + 1].pagina;
        }
        if(contadorTLB < TLB_SIZE){
            TLBtable[contadorTLB].pagina = pageNumber;
            TLBtable[contadorTLB].frame = frameNumber;

        }
        else{
            TLBtable[contadorTLB - 1].pagina = pageNumber;
            TLBtable[contadorTLB - 1].frame = frameNumber;
        }
    }
    if(contadorTLB < TLB_SIZE) { 
        contadorTLB++;
    }
}

void tlbLRU(int pageNumber, int frameNumber)
{
   if(contadorTLB < TLB_SIZE){
    for(int i = 0; i < contadorTLB; i++){
      if(pageNumber == TLBtable[i].pagina){
        tlbCounter[i] = 0;
        for(int k = 0; k < contadorTLB; k++){
          if(k != i) tlbCounter[k]++;
        }
        contadorTLB++;
        return;
      }
    }

    TLBtable[contadorTLB].pagina = pageNumber;
    TLBtable[contadorTLB].frame = frameNumber;

    for(int k = 0; k < contadorTLB; k++){
      tlbCounter[k]++;
    }
    tlbCounter[contadorTLB] = 0;
    contadorTLB++;
    return;
  }else{
    for(int i = 0; i < TLB_SIZE; i++){
      if(pageNumber == TLBtable[i].pagina){
        tlbCounter[i] = 0;
        for(int k = 0; k < TLB_SIZE; k++){
          if(k != i) tlbCounter[k]++;
        }
        return;
      }
    }

    int maior = 0;
    int maiorPos = 0;

    for(int j = 0; j < TLB_SIZE; j++){
      if(tlbCounter[j] > maior){
        maior = tlbCounter[j];
        maiorPos = j;
      }
    }

    TLBtable[maiorPos].pagina = pageNumber;
    TLBtable[maiorPos].frame = frameNumber;
    for(int k = 0; k < TLB_SIZE; k++){
      tlbCounter[k]++;
    }
    tlbCounter[maiorPos] = 0;
    return;
  }
}

void pageFifo() {
  if (contadorPagina != PAGES-1) 
  {   
    PageTable[contadorPagina].pagina = pageNumber;
    PageTable[contadorPagina].frame = contadorFrame;
    contadorPagina++;
    contadorFrame++;
    return;
  }else{
    PageTable[contadorPagina].pagina = pageNumber;
    PageTable[contadorPagina].frame = contadorFrame;
    contadorPagina = 0;
    contadorFrame = 0;
    return;
  }
}

void pageLru(){
  if(flagContadorPagina == 0){
    PageTable[contadorPagina].pagina = pageNumber;
    PageTable[contadorPagina].frame = contadorFrame;
    for(int k = 0; k < contadorPagina; k++){
      pageCounter[k]++;
    }
    pageCounter[contadorPagina] = 0;
    contadorPagina++;
    contadorFrame++;
    if(contadorPagina == 256){
      flagContadorPagina = 1;
      contadorPagina = 0;
      contadorFrame = 0;
    }
    return;
  }else{
    int maior = 0;
    int maiorPos = 0;

    for(int j = 0; j < PAGES; j++){
      if(pageCounter[j] > maior){
        maior = pageCounter[j];
        maiorPos = j;
      } 
    }

    PageTable[maiorPos].pagina = pageNumber;
    PageTable[maiorPos].frame = contadorFrame;
    for(int k = 0; k < PAGES; k++){
      pageCounter[k]++;
    }
    pageCounter[maiorPos] = 0;

    contadorPagina++;
    contadorFrame++;
    if(contadorPagina == 256){
      contadorPagina = 0;
      contadorFrame = 0;
    }

    return;
  }
}