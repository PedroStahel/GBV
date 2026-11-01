# GBV — Gerenciador de Biblioteca de Arquivos Binários

Projeto desenvolvido em **C** para a disciplina de **Programação II da Universidade Federal do Paraná (UFPR)**. O GBV é um gerenciador de arquivos via linha de comando que permite reunir documentos em um único arquivo binário e consultar seus metadados.

O projeto explora manipulação de arquivos binários, alocação dinâmica de memória, estruturas de dados, operações com ponteiros e ordenação com `qsort`.

## Funcionalidades

- **Adicionar** um ou mais documentos a uma biblioteca `.gbv`.
- **Remover** documentos do índice da biblioteca.
- **Listar** os documentos armazenados, com nome, tamanho, data de inserção e posição no arquivo.
- **Visualizar** o conteúdo de um documento em blocos de 512 bytes, com navegação pelo terminal.
- **Ordenar** os documentos por nome, tamanho ou data de inserção.

## Tecnologias e conceitos

- Linguagem **C** e biblioteca padrão;
- Leitura e escrita binária (`fread`, `fwrite`, `fseek`, `ftell`);
- **Alocação dinâmica** (`malloc`, `realloc`, `free`);
- Estruturas (`struct`) para representar documentos e bibliotecas;
- **Buffers** para processamento de arquivos em blocos;
- **`qsort`** e funções de comparação;
- Compilação com **GCC** e **Makefile**.

## Estrutura do projeto

| Arquivo | Responsabilidade |
| --- | --- |
| `main.c` | Interpretação dos argumentos e execução dos comandos |
| `gbv.c` / `gbv.h` | Implementação das operações da biblioteca e definição das estruturas |
| `util.c` / `util.h` | Formatação de datas |
| `Makefile` | Compilação do executável |
| `leiame.txt` | Observações originais sobre o desenvolvimento |

## Como compilar

É necessário ter **GCC** e **Make** instalados.

```bash
make
```

O comando gera o executável `gbv`. Para remover os arquivos compilados:

```bash
make clean
```

## Como usar

Os comandos seguem o formato:

```bash
./gbv <opção> <biblioteca> [documentos...]
```

O programa tenta criar a biblioteca caso ela ainda não exista.

### Adicionar arquivos

```bash
./gbv -a biblioteca.gbv documento1.txt documento2.txt
```

### Listar documentos

```bash
./gbv -l biblioteca.gbv
```

### Visualizar um documento

```bash
./gbv -v biblioteca.gbv documento1.txt
```

Na visualização, use `n` para avançar, `p` para voltar e `q` para sair.

### Remover um documento

```bash
./gbv -r biblioteca.gbv documento1.txt
```

### Ordenar a biblioteca

```bash
./gbv -o biblioteca.gbv nome
./gbv -o biblioteca.gbv tamanho
./gbv -o biblioteca.gbv data
```

> **Atenção:** na implementação atual, as funções de remoção e visualização abrem especificamente o arquivo `biblioteca.gbv`, independentemente do nome informado na linha de comando. Para utilizar essas operações, mantenha esse nome de arquivo.

## Como o formato funciona

O arquivo `.gbv` contém:

1. Um **superbloco** com a quantidade de documentos e a posição do diretório.
2. Os **dados binários** dos documentos armazenados.
3. Um **diretório de metadados**, com nome, tamanho, data de inserção e deslocamento (`offset`) de cada documento.

As operações utilizam esses deslocamentos para localizar os conteúdos. Na ordenação, o programa cria um arquivo temporário e reescreve os documentos e o diretório na nova ordem.

## Aprendizados

O desenvolvimento envolveu desafios como entender o uso de buffers, trabalhar com offsets em arquivos binários e passar funções de comparação como parâmetros para `qsort`.

Foi uma oportunidade de aplicar conceitos de programação em C em um programa com múltiplos módulos, gerenciamento de memória e operações persistentes em disco.

## Observações

Este é um **projeto acadêmico**. Os testes relatados no `leiame.txt` foram básicos; não há uma suíte automatizada de testes nem uma validação completa de memória com Valgrind documentada no repositório. Portanto, o programa não deve ser tratado como uma ferramenta de arquivamento pronta para produção.

Além disso, a remoção atualiza o diretório, mas não necessariamente recupera o espaço ocupado pelos dados removidos.

## Autor

**Pedro Stahel de Azevedo**  
Ciência da Computação — Universidade Federal do Paraná (UFPR)
