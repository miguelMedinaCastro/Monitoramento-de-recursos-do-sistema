#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <sys/sysinfo.h>
    #include <sys/statvfs.h>
    #include <sensors/sensors.h>
    #include <string.h>

    #define ARQ_TEMP "gpu_temp_output.txt"

    void calcular_uso_cpu(float* memoria_usada, float* uso_cpu) 
    {    
        struct sysinfo info;
        sysinfo(&info);

        unsigned long total_memoria = info.totalram + info.totalswap;
        total_memoria *= info.mem_unit;
        unsigned long ja_usada = total_memoria - info.freeram - info.freeswap;
        ja_usada *= info.mem_unit;

        *memoria_usada = (float)(ja_usada * 100) / total_memoria;

        FILE* arquivo = fopen("/proc/stat", "r");
        if (arquivo == NULL) {
            perror("Erro ao abrir /proc/stat");
            return;
        }

        long double a[4], b[4], media;
        fscanf(arquivo, "%*s %Lf %Lf %Lf %Lf", &a[0], &a[1], &a[2], &a[3]);
        fclose(arquivo);

        sleep(1);
        
        arquivo = fopen("/proc/stat", "r");
        if (arquivo == NULL) {
            perror("Erro ao abrir /proc/stat");
            return;
        }

        fscanf(arquivo, "%*s %Lf %Lf %Lf %Lf", &b[0], &b[1], &b[2], &b[3]);
        fclose(arquivo);

        media = ((b[0] + b[1] + b[2]) - (a[0] + a[1] + a[2])) / (b[0] + b[1] + b[2] + b[3] - (a[0] + a[1] + a[2] + a[3]));

        *uso_cpu = media * 100.0;
    }

    float calcular_uso_gpu(){
        FILE *arquivo;
        char buffer[20];
        char buffer_2[20];
        float valor_atual, valor_maximo, gpu;

        arquivo = fopen("/sys/class/drm/card1/gt_cur_freq_mhz", "r");
        if (arquivo == NULL){
            perror("Erro ao abrir ao arquivo");
            return -1.0f;
        }

        if (fgets(buffer, sizeof(buffer), arquivo) != NULL)
            sscanf(buffer, "%f", &valor_atual);
        else
            perror("Erro ao ler o arquivo");

        fclose(arquivo);

        arquivo = fopen("/sys/class/drm/card1/gt_max_freq_mhz", "r");
        if (arquivo == NULL){
            perror("Error ao abrir arquivo.");
            return -1.0f;
        }

        if (fgets(buffer_2, sizeof(buffer_2), arquivo) != NULL)
            sscanf(buffer_2, "%f", &valor_maximo);
        else
            perror("Erro ao ler o arquivo");
        
        fclose(arquivo);

        gpu = (valor_atual / valor_maximo) * 100;
        return gpu;        
    }

    char* gpu_nome() {
        FILE *arquivo;
        char buffer[256];
        char *gpu_nome = malloc(256);
        char comando[256];

        if (gpu_nome == NULL) {
            perror("Erro ao alocar memória");
            return NULL;
        }

        snprintf(comando, sizeof(comando), "lspci | grep -i vga | cut -d' ' -f 5- | cut -d'(' -f 1 > %s", ARQ_TEMP);

        if (system(comando) == -1) {
            perror("Erro ao executar o comando");
            free(gpu_nome);
            return NULL;
        }

        arquivo = fopen(ARQ_TEMP, "r");
        if (arquivo == NULL) {
            perror("Erro ao abrir o arquivo temporário");
            free(gpu_nome);
            return NULL;
        }

        if (fgets(buffer, sizeof(buffer) - 1, arquivo) != NULL) {
            buffer[strcspn(buffer, "\n")] = '\0';
            strncpy(gpu_nome, buffer, 256 - 1);
            gpu_nome[256 - 1] = '\0'; 
        } else {
            perror("Erro ao ler a saída do arquivo temporário");
            fclose(arquivo);
            free(gpu_nome);
            return NULL;
        }

        fclose(arquivo);

        if (remove(ARQ_TEMP) != 0) 
            perror("Erro ao remover o arquivo temporário");

        return gpu_nome;
    }   

    char *cpu_info(void) {
        FILE *arquivo;
        char line[256];
        char *nome = NULL;
        size_t len = 0;

        arquivo = fopen("/proc/cpuinfo", "r");
        if (arquivo == NULL) {
            perror("fopen");
            return NULL;
        }

        while (fgets(line, sizeof(line), arquivo)) {
            if (strstr(line, "model name")) {
                char *inicio = strchr(line, ':');
                
                if (inicio != NULL) {
                    inicio += 2;
                    char *fim = strchr(inicio, '\n');
                    if (fim != NULL) 
                        *fim = '\0';
                    
                    nome = malloc(strlen(inicio) + 1); 
                    
                    if (nome != NULL) 
                        strcpy(nome, inicio);
                }
                break;
            }
        }

        fclose(arquivo);
        return nome; 
    }

    float obter_temperatura_cpu() {
        const sensors_chip_name *chip;
        const sensors_feature *caracteristica;
        const sensors_subfeature *subcaracteristica;

        int i = 0;
        int j, k;
        double temperatura = 0.0;
        
        sensors_init(NULL);
        
        while ((chip = sensors_get_detected_chips(NULL, &i)) != NULL){
            j = 0;
            while ((caracteristica = sensors_get_features(chip, &j)) != NULL){
                if (caracteristica -> type == SENSORS_FEATURE_TEMP){
                    k = 0;
                    while ((subcaracteristica = sensors_get_subfeature(chip, caracteristica, SENSORS_SUBFEATURE_TEMP_INPUT)) != NULL) {
                        if (subcaracteristica -> type == SENSORS_SUBFEATURE_TEMP_INPUT){
                            if (sensors_get_value(chip, subcaracteristica -> number, &temperatura) == 0){
                                sensors_cleanup();
                                return (float)temperatura;  
                            }
                        }
                    }
                }
            }
            sensors_cleanup();
            return 0.0f;
        }
    }

    float velocidade_fans(){
        const sensors_chip_name *chip;
        const sensors_feature *caracteristica;
        const sensors_subfeature *subcaracteristica;
        double valor;
        int aviso, chip_nr = 0;
        
        sensors_init(NULL);
    
        while ((chip = sensors_get_detected_chips(NULL, &chip_nr)) != NULL){
            int caracteristica_nr = 0;

            while ((caracteristica = sensors_get_features(chip, &caracteristica_nr)) != NULL){
                if (caracteristica -> type == SENSORS_FEATURE_FAN){
                    int subcaracteristica_nr = 0;

                    while ((subcaracteristica = sensors_get_all_subfeatures(chip, caracteristica, &subcaracteristica_nr))){
                        if (subcaracteristica -> type == SENSORS_SUBFEATURE_FAN_INPUT){
                            aviso = sensors_get_value(chip, subcaracteristica -> number, &valor);
                            if (aviso == 0 && valor > 0){
                                sensors_cleanup();
                                return (float)valor;
                            }
                        }
                    }
                }
            }
        }

        sensors_cleanup();
        return 0.0f;
    }

    double monitorar_uso_disco()
    {
        struct statvfs stat;

        if (statvfs("/", &stat) != 0){
            perror("Erro ao obter informações sobre o disco");
            return 0.0;
        }

        unsigned long total_disco = stat.f_blocks * stat.f_frsize;
        unsigned long disco_usado = (stat.f_blocks - stat.f_bfree) * stat.f_frsize;

        return ((double)disco_usado * 100) / total_disco;
    }

    void monitorar_processos(int* tarefas_rolando, int* total_tarefas)
    {
        FILE* arquivo = fopen("/proc/loadavg", "r");
        if (arquivo == NULL){
            perror("Erro ao abrir /proc/loadavg");
            return;
        }

        double load1, load5, load15;
        fscanf(arquivo, "%lf %lf %lf %d/%d", &load1, &load5, &load15, tarefas_rolando, total_tarefas);
        fclose(arquivo);
    }

    char *obter_sistema(){
        FILE *arquivo;
        char buffer[256];
        char *descricao = NULL;

        arquivo = popen("lsb_release -a", "r");
        if (arquivo == NULL){
            perror("erro ao abrir arquivo");
            return NULL;
        }

        while (fgets(buffer, sizeof(buffer), arquivo) != NULL){
            if (strncmp(buffer, "Description:", 11) == 0){
                buffer[strcspn(buffer, "\n")] = '\0';

                char *inicio = strchr(buffer, ':');
                if (inicio != NULL)
                    inicio += 2;
                
                descricao = malloc(strlen(buffer + 1));
                if (descricao != NULL)
                    strcpy(descricao, inicio);
                
                break;
            }
        }

        pclose(arquivo);
        return descricao;        
    }

int main() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "Erro ao inicializar SDL: %s\n", SDL_GetError());
        return 1;
    }

    if (TTF_Init() == -1) {
        fprintf(stderr, "Erro ao inicializar SDL_ttf: %s\n", TTF_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Window *janela = SDL_CreateWindow("Monitor de Sistema", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 750, 350, SDL_WINDOW_SHOWN);
    if (janela == NULL) {
        fprintf(stderr, "Erro ao criar janela: %s\n", SDL_GetError());
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Renderer *renderizacao = SDL_CreateRenderer(janela, -1, SDL_RENDERER_ACCELERATED);
    if (renderizacao == NULL) {
        fprintf(stderr, "Erro ao criar renderizador: %s\n", SDL_GetError());
        SDL_DestroyWindow(janela);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    TTF_Font *fonte = TTF_OpenFont("/home/medina/Downloads/extra_serif/EXTRASerif-Regular.ttf", 30);
    if (fonte == NULL) {
        fprintf(stderr, "Erro ao abrir fonte: %s\n", TTF_GetError());
        SDL_DestroyRenderer(renderizacao);
        SDL_DestroyWindow(janela);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Color corPadrao = {255, 255, 255, 255};

    SDL_SetWindowAlwaysOnTop(janela, SDL_TRUE);
    SDL_RaiseWindow(janela);

    SDL_Event evento;
    int sair = 0;

    while (!sair) {
        while (SDL_PollEvent(&evento) != 0) {
            if (evento.type == SDL_QUIT) {
                sair = 1;
            }
        }

        char buffer[1024];

        int tarefas_rolando, total_tarefas;
        monitorar_processos(&tarefas_rolando, &total_tarefas);

        double uso_disco = monitorar_uso_disco();

        float memoria_usada, uso_cpu;

        calcular_uso_cpu(&memoria_usada, &uso_cpu);

        float temperatura = obter_temperatura_cpu();

        float ventoinha = velocidade_fans();

        char *processador = cpu_info();

        float gpu = calcular_uso_gpu();

        char *nome_gpu = gpu_nome();

        char *sistema = obter_sistema();

        snprintf(buffer, sizeof(buffer), "%s\nCPU: %s\n       Uso CPU: %.2f%%  %.1f C\nRAM: %.2f%%\nUso Disco: %.2lf%%\nTarefas: %d/%d\nFans: %.0f RPM\nGPU: %s: %.0f%%", 
        sistema, processador, uso_cpu, temperatura, memoria_usada, uso_disco, tarefas_rolando, total_tarefas, ventoinha, nome_gpu, gpu);

        SDL_SetRenderDrawColor(renderizacao, 0, 0, 0, 255); 
        SDL_RenderClear(renderizacao);

        SDL_Surface *textSurface = TTF_RenderText_Blended_Wrapped(fonte, buffer, corPadrao, 800); 
        SDL_Texture *textTexture = SDL_CreateTextureFromSurface(renderizacao, textSurface);

        if (textSurface == NULL || textTexture == NULL) {
            fprintf(stderr, "Erro ao criar superfície ou textura de texto: %s\n", SDL_GetError());
            SDL_FreeSurface(textSurface);
            SDL_DestroyTexture(textTexture);
            continue;
        }

        SDL_Rect renderQuad = {50, 50, textSurface->w, textSurface->h};

        SDL_FreeSurface(textSurface);

        SDL_RenderCopy(renderizacao, textTexture, NULL, &renderQuad);
        SDL_DestroyTexture(textTexture);

        SDL_RenderPresent(renderizacao);

    }

    TTF_CloseFont(fonte);
    SDL_DestroyRenderer(renderizacao);
    SDL_DestroyWindow(janela);
    TTF_Quit();
    SDL_Quit();

    return 0;
}

#endif