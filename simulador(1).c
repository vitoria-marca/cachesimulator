// SIMULADOR DE CACHE 
// ARQUITETURA E ORGANIZAÇÃO DE COMPUTADORES II
// Larissa Gabriela e Vitória Santa Lucia

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>

// limites maximos para seguranca
#define MAX_ASSOC 32
#define MAX_SETS 8192

// estrutura para representar cada bloco da cache
typedef struct {
    int valid;             // 1 se o bloco é válido, 0 se não é
    uint32_t tag;          // tag do endereço
    int lru_counter;       // contador para FIFO ou LRU
} CacheBlock;
CacheBlock **cache;

//estrutura que rastreia os pares (index, tag) já acessados
typedef struct {
    uint32_t *tags;
    int size;
    int capacity;
} VisitSet;
VisitSet set_visitado;

// variáveis para estatísticas
int total_acessos = 0;
int hits = 0;
int miss_compulsorio = 0;
int miss_total = 0;
int miss_conflito = 0;
int miss_capacidade = 0;
int blocos_validos = 0;

//protótipo de funções
int is_potencia2(int x);
void inicializar_visitado();
void liberar_visitado();
void inicializar_cache(int nsets, int assoc);
FILE* processar_arquivo(char *f);
void simular_acesso_cache(uint32_t _endereco, int nsets, int bsize, int assoc, char *sub);
void imprimir_estatisticas(int flag, int flag_out);
uint32_t inverter_big_endian(uint32_t x);

//corpo de funções

// função fread (main) lê os bytes na ordem em que aparecem no arquivo (big-endian)
// e arquiteturas Intel utilizam little-endian (leem primeiro o byte menos significativo)
uint32_t inverter_big_endian(uint32_t x) {
    unsigned char *bytes = (unsigned char*)&x;

    unsigned char temp;

    temp = bytes[0];
    bytes[0] = bytes[3];
    bytes[3] = temp;

    temp = bytes[1];
    bytes[1] = bytes[2];
    bytes[2] = temp;

    return x;
}
int is_potencia2(int x) {
    if ( x <= 0 ){ // n < ou = 0 não é potencia de 2
        return 0;
    }

    while ( x > 1 ){
        if ( x % 2 != 0 ){
            return 0; // resto = 0 não é potência de 2 
        }
        x = x / 2;
    }

    return 1;
}
//vetor de acessos que cresce dinamicamente
void inicializar_visitado(){
    set_visitado.tags = malloc(100 * sizeof(uint32_t));
    set_visitado.capacity = 100;
    set_visitado.size = 0;
}
void liberar_visitado(){
    free(set_visitado.tags);
}
//cache = nsets linhas x assoc colunas
//blocos_validos = blocos preenchidos
void inicializar_cache(int nsets, int assoc) {
    cache = malloc(nsets * sizeof(CacheBlock*));

    for (int i = 0; i < nsets; i++) {
        cache[i] = malloc(assoc * sizeof(CacheBlock));

        for (int j = 0; j < assoc; j++) {
            cache[i][j] = (CacheBlock){0,0,0};
        }
    }
    blocos_validos = 0;
}
//função para processar o nome do arquivo vindo do terminal e abri-lo em modo 
//de leitura binária
FILE *processar_arquivo(char *arquivo_de_entrada){
    FILE *fp = fopen(arquivo_de_entrada, "rb");

    if (!fp){
        perror("Erro ao abrir o arquivo");
        return NULL;
    }

    return fp;
}
void simular_acesso_cache(uint32_t _endereco, int nsets, int bsize, int assoc, char *substituicao) {
    total_acessos++;
    
    //inversão de bits para leitura correta dos endereços
    uint32_t endereco = inverter_big_endian( _endereco );

    // cálculo dos bits de offset, índice e tag
    // desloca os bits para a direita, removendo o offset
    uint32_t endereco_sem_offset = endereco / bsize;
    // o módulo separa os bits menos significativos = índice
    uint32_t index = endereco_sem_offset % nsets;
    // o restante é o tag
    uint32_t tag = endereco_sem_offset / nsets;
    

    //verificação de hit    
    int hit = 0;

    for (int i = 0; i < assoc; i++){
        if (cache[index][i].valid && cache[index][i].tag == tag){
            hits ++;
            hit = 1;

            //atualiza contador de LRU
            if (substituicao[0] == 'L'){
                cache[index][i].lru_counter = total_acessos;
                break;
            }
        }
    }

    if (hit) {
        return;
    }

    //se não entrou no if anterior, é miss
    miss_total++;
    //chave para referenciar um bloco na cache (com 64 bits)
    uint64_t chave = ((uint64_t)index<<32)|tag;
    int novo = 1;
    
    //a chave já foi acessada antes?
    for (int i = 0 ; i < set_visitado.size; i++){
        if (set_visitado.size == set_visitado.capacity){
            novo = 0;
            break;
        }
    }

    //se não foi acessada:
    if (novo){
        if (set_visitado.size == set_visitado.capacity){
            set_visitado.capacity *= 2;
            set_visitado.tags = realloc(
                set_visitado.tags,
                set_visitado.capacity * sizeof(uint64_t)
            );
        }
        //armazena a tag visitada
        set_visitado.tags[set_visitado.size++] = chave;
    }

    //verificação se o conjunto está cheio
    int set_cheio = 1;
    for (int i = 0; i < assoc; i++){
        if (!cache[index][i].valid){
            set_cheio = 0;
            break;
        }
    }

    //classificação dos misses

    if (novo){
        if (!set_cheio){
            miss_compulsorio++; //primeiro acesso e há vias livres
        } else if (blocos_validos < (long)nsets*assoc){
            miss_conflito++; // tag nova e set cheio, mas tem espaço na cache
        } else {
            miss_capacidade++; //tag nova, set cheio e sem espaço na cache
        }
    } else {
        if (set_cheio){
            if (blocos_validos < (long)nsets*assoc){
                miss_conflito++; //conflito pelo set estar cheio
            } else {
                miss_capacidade++; //cache cheia
            }
        } else {
            //chave revista e set com espaço, ou seja, reutilização de via livre
            miss_conflito++;
        }
    }

    //SUBSTITUIÇÕES

    int via = -1;

    //procurando via livre
    for (int i = 0; i < assoc; i++){
        if (!cache[index][i].valid) {
            via = i; //achou via livre
            break;
        }
    }
    if (via < 0){ //não há via livre, então precisamos chamar as políticas de substituição
        if (substituicao[0] == 'R'){
            via = rand() % assoc;

        } else {
            via = 0; //procurar menor bloco com lru counter
            for (int i = 0; i < assoc; i++){
                if (cache[index][i].lru_counter < cache[index][via].lru_counter){
                    via = i; //substituir via a ser substituída pelo menor lru counter
                }
            }
        }
    }

    // se a via estava livre, incrementa contagem de blocos válidos
    if (!cache[index][via].valid) {
        blocos_validos++;
    }

    // carrega novo bloco na via selecionada
    cache[index][via].valid = 1;
    cache[index][via].tag = tag;
    cache[index][via].lru_counter = total_acessos;
}
void imprimir_estatisticas(int flag, int flag_out) {
    double t_hit = (double)hits / total_acessos;
    double t_miss = (double)miss_total / total_acessos;
    double t_compu, t_confl, t_capac;

    if (miss_total){
        t_compu = (double)miss_compulsorio / miss_total;
        t_confl = (double)miss_conflito / miss_total;
        t_capac = (double)miss_capacidade / miss_total;
    } else {
        t_compu = t_confl = t_capac = 0;
    }
    
    if (flag == 0) {
        if (flag_out == 0 || flag_out == 1){
            printf(
            "Total: %d \n"
            "hits: %d \n"
            "misses: %d \n"
            "compulsorios: %ld \n"
            "conflito: %ld \n"
            "capacidade: %ld\n",
            total_acessos,
            hits,
            miss_total,
            miss_compulsorio,
            miss_conflito,
            miss_capacidade);
        } 
        if (flag_out == 0 || flag_out == 2){
            printf(" ");
            printf("##### TAXAS ######\n");
            printf(
            "hit: %.4f \n"
            "miss: %.4f \n"
            "compulsorios: %.4f \n"
            "conflito: %.4f \n"
            "capacidade: %.4f \n",
            t_hit,
            t_miss,
            t_compu,
            t_confl,
            t_capac);
        } 
    } else {
        printf(
            "%d %.4f %.4f %.4f %.4f %.4f\n",
            total_acessos,
            t_hit,
            t_miss,
            t_compu,
            t_confl,
            t_capac
        );
    }
}
int main (int argc, char *argv[]){
    
    //se estiver faltando parâmetros, o programa fecha
    if (argc != 7){
		printf("Numero de argumentos incorreto. Utilize:\n");
		printf("./cache_simulator.exe <nsets> <bsize> <assoc> <substituição> <flag_saida> arquivo_de_entrada\n");
		return 1;
	}

    // inicialização da função aleatória 
    srand(time(NULL)); 

    // parâmetros vindos da linha de comando
    int nsets = atoi(argv[1]);
    int bsize = atoi(argv[2]);
    int assoc = atoi(argv[3]);
    char *substituicao = argv[4];
    int flag_saida = atoi(argv[5]);
    char* arquivo_entrada = argv[6];   

    // verifica se todos os parâmetros são potências de 2
    if (!is_potencia2(nsets) || !is_potencia2(bsize) || !is_potencia2(assoc)) {
        fprintf(stderr, "Erro: nsets, bsize e assoc devem ser potências de 2.\n");
        return 1;
    }

    FILE *f = processar_arquivo(arquivo_entrada);

    inicializar_cache(nsets, assoc);
    inicializar_visitado();

    uint32_t end;
    // lê cada endereço de 32 bits e simula acesso
    while (fread(&end, sizeof(end), 1, f) == 1) {
        simular_acesso_cache(end, nsets, bsize, assoc, substituicao);
    }

    fclose(f);

    int flag_out = 3;
    if (!flag_saida){
            printf(
        "Exibir dados:\n"
        "[1] - numeros absolutos  [2] - taxas    [0] - ambos\n");
        scanf("%d", &flag_out);
    }

    imprimir_estatisticas(flag_saida, flag_out);
    liberar_visitado();
    // libera memória de cada set e da matriz
    for (int i = 0; i < nsets; i++) {
        free(cache[i]);
    }
    free(cache);

    return 0;
}