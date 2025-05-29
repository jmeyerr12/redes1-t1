# Caça ao Tesouro - Rede com Raw Sockets

Este projeto implementa um jogo em rede chamado **Caça ao Tesouro**, utilizando **raw sockets** e um protocolo próprio inspirado no **Kermit**, com controle de fluxo do tipo **Stop-and-Wait**. O jogador se movimenta por um mapa e, ao encontrar um tesouro, recebe arquivos como recompensa (texto, imagem ou vídeo).

## Estrutura do Projeto

- `cliente.c` — Lê comandos do usuário (`W`, `A`, `S`, `D`), envia movimentos e recebe arquivos.
- `servidor.c` — Processa os movimentos dos jogadores e envia os arquivos dos tesouros.
- `kermit.h` / `kermit.c` — Define o protocolo e empacotamento dos dados.
- `util.c` — Funções auxiliares para mapa, tempo, arquivos e controle do jogo.

## Como Jogar

### 1. Compile os arquivos com:

```bash
gcc cliente.c kermit.c util.c -o cliente
gcc servidor.c kermit.c util.c -o servidor
```

### 2. Rode o servidor:

```bash
sudo ./servidor <interface>
```

### 3. Rode o cliente em outro terminal:

```bash
sudo ./cliente <interface>
```

### 4. Comandos do jogador:

- `W` — mover para cima
- `A` — mover para a esquerda
- `S` — mover para baixo
- `D` — mover para a direita
- `Q` — sair do jogo

## Funcionalidades

- Protocolo confiável com **Stop-and-Wait**
- Tratamento de perdas com **ACK/NACK**
- Envio de arquivos em pacotes segmentados
- Mapa 8x8 com tesouros aleatórios
- Ping automático após 3 segundos de inatividade

## Requisitos

- Compilador C (GCC)
- Acesso root (necessário para usar raw sockets)
- Sistema operacional Linux

## Observações

Este projeto foi desenvolvido para fins acadêmicos, simulando comunicação de baixo nível e mecanismos confiáveis de transmissão de dados.

## To-do
- Consegui fazer funcionar andar desconectando cabo, mas na transmissão de arquivos isso ainda não funciona efetivamente
- Arrumar recebimento/envio de vídeo
- Tratamento de erros fora NACK comum (tamanho do arquivo maior que o tamanho disponível por exemplo precisa fazer ainda)
- Fazer algoritmo pro seq: "Para verificar se uma sequência vem depois da outra, pense na diferença das sequências: É grande o suficiente pra estarem no meio, ou nas bordas? O algoritmo que você vai acabar criando é aquele de aritmética de números seriais." Não lembro de ter implementado isso
- Em algum momento ta tendo movimento duplicado (APARENTEMENTE RESOLVI)
- Usar ultimo tipo livre pra fazer sincronização de dados com o servidor (localização do player, tesouros encontrados)
