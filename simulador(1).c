// SIMULADOR DE CACHE 
// ARQUITETURA E ORGANIZAÇÃO DE COMPUTADORES II
// Larissa Gabriela e Vitória Santa Lucia

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
static uint32_t ntohl(uint32_t x) {
    return ((x & 0xFF) << 24) |
           ((x & 0xFF00) << 8) |
           ((x & 0xFF0000) >> 8) |
           ((x & 0xFF000000) >> 24);
}

// Limites maximos para seguranca
#define MAX_ASSOC 32
#define MAX_SETS 8192

// Estrutura para representar cada bloco da cache
typedef struct {
    int valid;             // 1 se o bloco é válido, 0 se não é
    uint32_t tag;          // tag do endereço
    int lru_counter;       // contador para FIFO ou LRU
} CacheBlock;
CacheBlock **cache;

typedef struct {
    uint32_t *tags;
    int size;
    int capacity;
} VisitSet;
VisitSet set_visitado;

// Variáveis para estatísticas
long total_acessos = 0;
long hits = 0;
long miss_compulsorio = 0;
long miss_total = 0;
long miss_conflito = 0;
long miss_capacidade = 0;
long blocos_validos = 0;

//Protótipo de funções
int is_potencia2(int x);
int log2int(int x);
void inicializar_visitado();
void liberar_visitado();
void inicializar_cache(int nsets, int assoc);
FILE* processar_arquivo(char *f);
void simular_acesso_cache(uint32_t raw, int nsets, int bsize, int assoc, char *sub);
void imprimir_estatisticas(int flag);

//Corpo de funções
int is_potencia2(int x) {
    return x && !(x & (x - 1)); //operação bit a bit
}

void inicializar_visitado(){
    set_visitado.tags = (uint32_t*)malloc(100 * sizeof(uint32_t));
    set_visitado.capacity = 100;
    set_visitado.size = 0;
}

void liberar_visitado(){
    free(set_visitado.tags);
}

// Calcula log base 2 inteiro (pra bits)
int log2int(int x) {
    return (int)(log(x) / log(2));
}

void inicializar_cache(int nsets, int assoc) {
    for (int i = 0; i < nsets; i++) {
        for (int j = 0; j < assoc; j++) {
            cache[i][j].valid = 0;
            cache[i][j].tag = 0;
            cache[i][j].lru_counter = 0;
        }
    }

    blocos_validos = 0;
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
    uint32_t index = (endereco >> offset_bits) & (nsets - 1);
    uint32_t tag = endereco >> (offset_bits + index_bits);

    int hit = 0;

    for (int i = 0; i < assoc; i++){
        if (cache[index][i].valid && cache[index][i].tag == tag){
            hits ++;
            hit = 1;

            if (substituicao[0] == 'L'){
                cache[index][i].lru_counter = total_acessos;
            }
            break;
        }
    }

    if (!hit){
        miss(index, tag, nsets, assoc, substituicao);
    }
}

void miss (int index, uint32_t tag, int nsets, int assoc, char *substituicao){
    miss_total ++;
    int encontrado = 0;

    for (int i = 0; i < set_visitado.size; i++){
        if (set_visitado.tags[i] == tag){
            encontrado = 1;
            break;
        }
    }

    if (!encontrado){
        miss_compulsorio++;
        if (set_visitado.size == set_visitado.capacity){
            set_visitado.capacity *= 2;
            set_visitado.tags = realloc (set_visitado.tags,
                                        set_visitado.capacity * sizeof(uint32_t));
        }
        set_visitado.tags[set_visitado.size++] = tag;
    } else {
        if (blocos_validos == nsets * assoc){
            miss_capacidade ++;
        } else {
            miss_conflito++;
        }
    }

    int substituir_via = -1;

    for (int i = 0; i < assoc; i++){
        if (!cache[index][i].valid){
            substituir_via = i;
            break;
        }
    }

    if (substituir_via == -1){
        if (substituicao[0] == 'R'){
            substituir_via = rand() % assoc;
        } else {
            int min = cache[index][0].lru_counter;
            substituir_via = 0;
            for (int i = 0; i < assoc; i++){
                if (cache[index][i].lru_counter < min){
                    min = cache[index][i].lru_counter;
                    substituir_via = i;
                }
            }
        }
    }

    int era_valido = cache[index][substituir_via].valid;
    if (!era_valido){
        blocos_validos++;
    }

    cache[index][substituir_via].valid = 1;
    cache[index][substituir_via].tag = tag;
    cache[index][substituir_via].lru_counter = total_acessos;

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