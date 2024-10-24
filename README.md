# Monitoramento de Recursos do Sistema

## Introdução

O código é um monitor de sistema gráfico, utilizando a biblioteca SDL (Simple DirectMedia Layer) e a biblioteca SDL_ttf, que permite renderizar texto com fontes TrueType. O monitor coleta informações do sistema em questão, tanto como o uso da CPU, memória RAM, armazenamento(SSD/HD), temperatura da CPU, velocidade do cooler e utilização da GPU.

## Organização do Código

### 1. Inicialização das Bibliotecas
- Começa com a inicialização da SDL e da SDL_ttf. Se ocorrer algum erro durante a inicialização, ele exibe uma mensagem de erro e encerra o programa.

### 2. Criação da Janela e Renderizador
- Uma janela é criada para exibir as informações do sistema, juntamente com um renderizador que permite desenhar na janela.

### 3. Coleta de Dados do Sistema
Define várias funções para coletar informações sobre o sistema:

- **calcular_uso_cpu**: Obtém o uso da CPU e a memória usada.
- **calcular_uso_gpu**: Calcula a utilização da GPU com base nas frequências atuais e máximas.
- **gpu_nome**: Recupera o nome da GPU usando um comando do sistema e armazena em um arquivo temporário.
- **cpu_info**: Lê informações sobre o processador a partir do arquivo `/proc/cpuinfo`.
- **obter_temperatura_cpu**: Usa a biblioteca `libsensors` para obter a temperatura da CPU.
- **velocidade_fans**: Obtém a velocidade dos coolers.
- **monitorar_uso_disco**: Calcula a utilização do armazenamento.
- **monitorar_processos**: Monitora o número de processos em execução.

### 4. Loop Principal
- O código entra em um loop onde verifica eventos (como o fechamento da janela) e atualiza as informações do sistema. Parte das informações são formatadas em uma string e exibidas na janela, outra parte apenas são inteiros do próprio sistema.

### 5. Limpeza
- Ao encerrar o loop, o programa libera os recursos utilizados, como as superfícies e texturas criadas, e finaliza a SDL.

### 6. Instruções de Compilação

Para compilar o código, use o seguinte comando:

```bash
gcc main.c -o "nome_executor" -lSDL2 -lSDL2_ttf -lsensors
```

**Nota**: o código **só rodará** se for compilado com este comando. Se você tentar compilar de outra forma, não irá funcionar corretamente ou apenas não funcionará.

## Bibliotecas Utilizadas

- **SDL2 (Simple DirectMedia Layer)**: Uma biblioteca que fornece uma abstração sobre hardware de gráficos, som e entrada.

- **SDL_ttf**: Uma extensão da SDL que permite o uso de fontes TrueType para renderização de texto, possibilitando que o texto seja exibido em diferentes estilos e tamanhos.

- **libsensors**: Usada para acessar sensores de hardware, como temperatura da CPU e velocidade dos coolers.

- **sysinfo.h e statvfs.h**: Bibliotecas padrão do Linux para acessar informações do sistema, como estatísticas de memória e de armazenamento.

- **stdio.h e stdlib.h**: Bibliotecas padrão do C para manipulação de entrada e saída e alocação de memória.

## Observação

Atualmente, o código funciona apenas para **Linux(Ubuntu 20-24)**. Tenho planos futuros para adaptá-lo e torná-lo compatível com **Windows**.
