// SIMULADOR DE CACHE 
// ARQUITETURA E ORGANIZAÇÃO DE COMPUTADORES II
// Larissa Gabriela e Vitória Santa Lucia

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
long int *visitado = NULL; // vetor auxiliar para verificar misses compulsórios
long int miss_conflito = 0;
long int miss_capacidade = 0;


// Calcula log base 2 inteiro (pra bits)
int log2int(int x) {
    return (int)(log(x) / log(2));
}

// Inicializa toda a cache (inicio zerado)
void inicializar_cache(int nsets, int assoc) {
    for (int i = 0; i < nsets; i++) {
        for (int j = 0; j < assoc; j++) {
            cache[i][j].valid = 0;
            cache[i][j].tag = 0;
            cache[i][j].lru_counter = 0;
        }
    }

    // aloca vetor para verificar quais tags já apareceram (miss compulsório)
    visitado = (int*)calloc(1 << 25, sizeof(int)); // 25 bits de tag (32 - offset - �ndice)
    if (!visitado){
        printf("Erro ao alocar memória");
        return 1;
    }
}

//Função para processar o nome do arquivo vindo do terminal e abri-lo em modo 
//de leitura binária
FILE *processar_arquivo(char *arquivo_de_entrada){
    FILE *fp = fopen(arquivo_de_entrada, "rb");

    if (!fp){
        perror("Erro ao abrir o arquivo");
        return NULL;
    }

    return fp;
}

void simular_acesso_cache(uint32_t endereco, int nsets, int bsize, int assoc, char *substituicao) {
    total_acessos++;
    
    // calculo dos bits de offset, índice e tag
    int offset_bits = log2int(bsize);
    int index_bits = log2int(nsets);
    int tag_bits = 32 - offset_bits - index_bits;
    
    // Extrai o índice e a tag do endereço
    uint32_t index = (endereco >> offset_bits) & (nsets - 1);
    uint32_t tag = endereco >> (offset_bits + index_bits);

    if (assoc == 1) //se mapeamento direto
    {
        if (cache[index][0].valid && cache[index][0].tag == tag){
            hits ++;
        } else {
            miss(index, tag, 0, substituicao);
        }
    }
    // Verifica se é hit (mapeamento direto usa apenas a primeira via - índice 0)
    if (cache[index][0].valid && cache[index][0].tag == tag) {
        hits++;
    } else {
        miss_total++;
        
        // Verifica se é miss compulsório (primeiro acesso a essa tag)
        if (visitado[tag] == 0) {
            miss_compulsorio++;
            visitado[tag] = 1;
        }
        
        // Atualiza a cache (mapeamento direto sobrescreve o bloco diretamente)
        cache[index][0].valid = 1;
        cache[index][0].tag = tag;
    }
}

void miss (int index, uint32_t tag, int assoc, char *substituicao){
    miss_total ++;

    if (visitado[tag] == 0){
        miss_compulsorio++;
        visitado[tag] = 1;
        return;
    }
     
    if (assoc < (MAX_SETS * MAX_ASSOC)) {
        miss_conflito++;
    } else {
        miss_capacidade++;
    }

}

int main (int argc, char *argv[]){
    
    //se estiver faltando parâmetros, o programa fecha
    if (argc != 7){
		printf("Numero de argumentos incorreto. Utilize:\n");
		printf("./cache_simulator <nsets> <bsize> <assoc> <substituição> <flag_saida> arquivo_de_entrada\n");
		return 1;
	}

    // Inicialização da função aleatória 
    srand(time(NULL)); 

    // Parâmetros vindos da linha de comando
    int nsets = atoi(argv[1]);
    int bsize = atoi(argv[2]);
    int assoc = atoi(argv[3]);
    char *substituicao = argv[4];
    int flag_saida = atoi(argv[5]);
    char* arquivo_entrada = argv[6];   

    // verificações quanto ao tamanho da cache
    if (nsets > MAX_SETS) {
        printf("Número de conjuntos excede o máximo permitido (%d)\n", MAX_SETS);
        return 1;
    }
    if (assoc > MAX_ASSOC) {
        printf("Associatividade excede o máximo permitido (%d)\n", MAX_ASSOC);
        return 1;
    }

    // Confirmacao de dados
    printf("nsets = %d\n", nsets);
	printf("bsize = %d\n", bsize);
	printf("assoc = %d\n", assoc);
	printf("subst = %s\n", substituicao);
	printf("flagOut = %d\n", flag_saida);
	printf("arquivo = %s\n", arquivo_entrada);

    FILE *arquivo_bin = processar_arquivo(arquivo_entrada);

    inicializar_cache(nsets, assoc);


}