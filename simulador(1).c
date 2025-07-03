#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <time.h>

// Limites maximos para seguranca
#define MAX_ASSOC 32
#define MAX_SETS 8192

// Estrutura para representar cada bloco da cache
typedef struct {
    int valid;             // 1 se o bloco é válido, 0 se não é
    uint32_t tag;          // tag do endereço
    int lru_counter;       // contador para FIFO ou LRU
} CacheBlock;

// Cache: uma matriz [nsets][associatividade]
CacheBlock cache[MAX_SETS][MAX_ASSOC];

// Variáveis para estatísticas
long int total_acessos = 0;
long int hits = 0;
long int miss_compulsorio = 0;
long int miss_total = 0;
long int *visitado; // vetor auxiliar para verificar misses compulsórios
long int miss_conflito = 0;
long int miss_capacidade = 0;

// Calcula log base 2 inteiro (pra bits)
int log2int(int x) {
    return (int)(log(x) / log(2));
}

// Inicializa toda a cache (inicio zerado)
void inicializar_cache() {
    for (int i = 0; i < nsets; i++) {
        for (int j = 0; j < assoc; j++) {
            cache[i][j].valid = 0;
            cache[i][j].tag = 0;
            cache[i][j].lru_counter = 0;
        }
    }

    // Aloca vetor para verificar quais tags j� apareceram (miss compuls�rio)
    visitado = (int*)calloc(1 << 25, sizeof(int)); // 25 bits de tag (32 - offset - �ndice)
}

//Função para processar o nome do arquivo vindo do terminal e abri-lo em modo 
//de leitura binária
FILE *processar_arquivo(char *arquivo_de_entrada){
    FILE *fp = fopen(arquivo_de_entrada, "rb");

    if (fp == NULL){
        perror("Erro ao abrir o arquivo");
        return NULL
    }

    return fp;
}

int main (int argc, char *argv[]){
    
    //se estgiver faltando parâmetros, o programa fecha
    if (argc != 7){
		printf("Numero de argumentos incorreto. Utilize:\n");
		printf("./cache_simulator <nsets> <bsize> <assoc> <substituição> <flag_saida> arquivo_de_entrada\n");
		exit(EXIT_FAILURE);
	}

    // Parametros vindos da linha de comando
    int nsets = argv[1];
    int bsize = argv[2];
    int assoc = argv[3];
    char *substituicao = argv[4];
    int flag_saida = argv[5];
    char* arquivo_entrada = argv[6];

    // Confirmacao de dados
    printf("nsets = %d\n", nsets);
	printf("bsize = %d\n", bsize);
	printf("assoc = %d\n", assoc);
	printf("subst = %s\n", substituicao);
	printf("flagOut = %d\n", flag_saida);
	printf("arquivo = %s\n", arquivo_entrada);

    FILE *arquivo_bin = processar_arquivo(arquivo_entrada);


}