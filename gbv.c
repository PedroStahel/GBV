#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gbv.h"
#include "util.h"

typedef struct {
    int numDocs; /* Número de documentos armazenados. */
    long offsetDir; /* Posição do diretório. */
} superBloco;

/* Retorna 1 se sucesso e 0 se fracasso. */
int leSuperBloco( FILE *gbv, superBloco *sb ) {
    if ( fread( sb, sizeof( superBloco ), 1, gbv ) != 1 ) {
        perror( "Erro ao ler o super bloco" );
        
        return 0;
    }

    return 1;
}

int gbv_create(const char *filename) {
    superBloco sb;
    FILE *gbv;

    /* Cria o arquivo para escrita binária. */
    if ( !( gbv = fopen( filename, "wb" ) ) ) {
        perror( "Erro ao criar o arquivo" );
        fclose( gbv );
        
        return 0;
    }

    /* Inicializa o super bloco. */
    sb.numDocs = 0;
    sb.offsetDir = sizeof(superBloco);
    
    /* Escreve o super bloco no arquivo. */
    if ( fwrite( &sb, sizeof(superBloco), 1, gbv ) != 1 ) {
        perror( "Erro ao escrever no arquivo" );
        fclose( gbv );

        return 0;
    }
    fclose( gbv );
    
    return 1;
}

/* Abre um arquivo de biblioteca(GBV)
 * Lê o super bloco e o diretório para a estrutura de biblioteca.
 * Retorna 1 se sucesso e 0 se fracasso. */
int gbv_open(Library *lib, const char *filename) {
    FILE *gbv;
    superBloco sb;

    /* Abre o arquivo para leitura binária */
    if ( !( gbv = fopen( filename, "rb" ) ) ) {
        perror( "Erro ao abrir o arquivo" );
        
        return 0;
    }

    if ( leSuperBloco( gbv, &sb ) != 1 ) {
        fclose( gbv );
        
        return 0;
    }

    /* Inicializa a estrutura de biblioteca. */
    lib -> count = sb.numDocs;
    if ( lib -> count == 0 ) {
        fclose( gbv );

        return 1;
    }

    /* Aloca memória para o vetor de documentos. */
    if ( !( lib -> docs = malloc( lib -> count * sizeof( Document ) ) ) ) {
        perror( "Erro ao alocar memória para os documentos" );
        fclose( gbv );

        return 0;
    }

    /* Busca o início do diretório de documentos. 
     * Baseia-se no offset do diretório, que está armazenado no super bloco. */
    if ( fseek( gbv, sb.offsetDir, SEEK_SET ) != 0 ) {
        perror( "Erro ao posicionar o ponteiro do arquivo" );
        free( lib -> docs );
        fclose( gbv );

        return 0;
    }

    /* Lê os documentos e os armazena no vetor da biblioteca. */
    if ( fread( lib -> docs, sizeof( Document ), lib -> count, gbv ) != lib -> count ) {
        perror( "Erro ao ler os documentos" );
        free( lib -> docs );
        fclose( gbv );

        return 0;
    }

    fclose( gbv );
    return 1;

}

int gbv_add(Library *lib, const char *archive, const char *docname) {
    FILE *gbv, *novoDoc;
    Document *docs = NULL;
    superBloco sb;
    long total = 0, offset;
    int aux = -1;

    if ( !( gbv = fopen(archive, "r+b" ) ) ) {
        perror( "Erro ao abrir o arquivo" );
        
        return 0;
    }

    if ( !( novoDoc = fopen( docname, "rb" ) ) ) {
        perror( "Erro ao abrir o documento" );
        fclose( gbv );
        
        return 0;
    }

    if ( leSuperBloco( gbv, &sb ) != 1 ) {
        fclose( gbv );
        fclose( novoDoc );
        
        return 0;
    }

    /* Carrega diretório no vetor docs. */
    if ( sb.numDocs > 0 ) {
        if ( !( docs = malloc( sb.numDocs * sizeof( Document ) ) ) ) {
            perror( "Erro ao alocar memória" );
            fclose( gbv );
            fclose( novoDoc );
        
            return 0;
        }

        if ( fseek( gbv, sb.offsetDir, SEEK_SET ) != 0 ) {
            perror( "Erro no fseek" );
            free( docs );
            fclose( gbv );
            fclose( novoDoc );
        
            return 0;
        }

        if ( fread( docs, sizeof( Document ), sb.numDocs, gbv ) != sb.numDocs ) {
            perror( "Erro ao ler diretório" );
            free( docs );
            fclose( gbv );
            fclose( novoDoc );
        
            return 0;
        }
    }

    /* Checa se o documento já existe. */
    for ( int i = 0; i < sb.numDocs; i++ ) {
        if ( strcmp( docs[i].name, docname ) == 0 ) {
            aux = i;
            break;
        }
    }

    /* Vai para o fim da área de dados. */
    if ( fseek( gbv, sb.offsetDir, SEEK_SET ) != 0 ) {
        perror( "Erro ao posicionar no fim dos dados" );
        free( docs );
        fclose( gbv );
        fclose( novoDoc );
        
        return 0;
    }

    /* O offset para o novo documento é a posição atual do ponteiro. */
    offset = ftell( gbv );

    /* Copia arquivo usando o buffer. */
    char buffer[BUFFER_SIZE];
    size_t lidos;

    while ( ( lidos = fread( buffer, 1, BUFFER_SIZE, novoDoc ) ) > 0 ) {
        if ( fwrite( buffer, 1, lidos, gbv ) != lidos ) {
            perror( "Erro ao escrever dados" );
            free( docs );
            fclose( gbv );
            fclose( novoDoc );
        
            return 0;
        }
        total += lidos;
    }

    /* Cria novo documento. */
    Document novo;
    strncpy( novo.name, docname, MAX_NAME );
    novo.name[MAX_NAME - 1] = '\0';
    novo.size = total;
    novo.date = time( NULL );
    novo.offset = offset;

    /* Atualiza vetor. */
    if ( aux != -1 ) {
        docs[aux] = novo;
    } else { /* Documento não encontrado, adiciona ao final. */
        if ( !( docs = realloc( docs, ( sb.numDocs + 1 ) * sizeof( Document ) ) ) ) {
            perror( "Erro no realloc" );
            free( docs );
            fclose( gbv );
            fclose( novoDoc );
        
            return 0;
        }
        docs[ sb.numDocs ] = novo;
        sb.numDocs++;
    }

    /* Atualiza offset do diretório no super bloco. */
    sb.offsetDir = ftell(gbv);

    /* Escreve o diretório novo na GBV. */
    if ( fwrite( docs, sizeof( Document ), sb.numDocs, gbv) != sb.numDocs ) {
        perror( "Erro ao escrever diretório" );
        free( docs );
        fclose( gbv );
        fclose( novoDoc );
        
        return 0;
    }

    /* Volta ao início para atualizar o superbloco. */
    if ( fseek( gbv, 0, SEEK_SET ) != 0 ) {
        perror( "Erro ao voltar ao início" );
        free( docs );
        fclose( gbv );
        fclose( novoDoc );
        
        return 0;
    }

    /* Escreve o superbloco atualizado. */
    if ( fwrite( &sb, sizeof( superBloco ), 1, gbv ) != 1 ) {
        perror( "Erro ao escrever superbloco" );
        free( docs );
        fclose( gbv );
        fclose( novoDoc );

        return 0;
    }

    free( docs );
    fclose( gbv );
    fclose( novoDoc );

    return 1;
}

int gbv_remove( Library *lib, const char *docname ) {
    FILE *gbv = NULL;
    superBloco sb;
    Document *docs = NULL; /* Vetor de documentos. */
    int aux = -1;

    /* Abre o arquivo da biblioteca. 
     * Foi assumido um valor padrão para o nome do arquivo. */
    if ( !( gbv = fopen( "biblioteca.gbv", "r+b" ) ) )
        return 0;

    if ( leSuperBloco( gbv, &sb ) != 1 ) {
        fclose( gbv );
        
        return 0;
    }

    /* Checa se o diretório está vazio. */
    if ( sb.numDocs == 0 ) {
        fclose( gbv );

        return 0;
    }

    if ( !( docs = malloc( sb.numDocs * sizeof( Document ) ) ) ) {
        fclose( gbv );
        
        return 0;
    }

    /* Vai para o offset do diretório. */
    if ( fseek( gbv, sb.offsetDir, SEEK_SET ) != 0 ) {
        free( docs );
        fclose( gbv );
        
        return 0;
    }

    /* Lê o diretório. */
    if ( fread( docs, sizeof( Document ), sb.numDocs, gbv ) != sb.numDocs ) {
        free( docs );
        fclose( gbv );
        
        return 0;
    }

    /* Procura pelo documento. */
    for ( int i = 0; i < sb.numDocs; i++ )
        if ( strcmp( docs[i].name, docname ) == 0 ) {
            aux = i;
            break;
        }

    /* Não encontrado. */
    if ( aux == -1 ) {
        free( docs );
        fclose( gbv );
        
        return 0;
    }

    /* Sobrescreve o documento. */
    for ( int i = aux; i < sb.numDocs - 1; i++ )
        docs[i] = docs[i + 1];

    sb.numDocs--;

    /* Retorna para o início do diretório. */
    if ( fseek( gbv, sb.offsetDir, SEEK_SET ) != 0 ) {
        free( docs );
        fclose( gbv );
        
        return 0;
    }

    /* Escreve o diretório atualizado. */
    if ( fwrite( docs, sizeof( Document ), sb.numDocs, gbv ) != sb.numDocs ) {
        free( docs );
        fclose( gbv );
        
        return 0;
    }

    /* Volta ao início para atualizar o superbloco. */
    if ( fseek( gbv, 0, SEEK_SET ) != 0 ) {
        free( docs );
        fclose( gbv );
        
        return 0;
    }

    /* Escreve o superbloco atualizado. */
    if ( fwrite( &sb, sizeof( superBloco ), 1, gbv ) != 1 ) {
        free( docs );
        fclose( gbv );
        
        return 0;
    }

    free( docs );
    fclose( gbv );

    lib -> count = sb.numDocs;

    return 1;
}

int gbv_list( const Library *lib ) {
    char data[64];

    /* Formata a data e escreve os metadados do componente. */
    for ( int i = 0; i < lib -> count; i++ ) {
        format_date( lib -> docs[i].date, data, 64 );

        printf( "%s | %ld bytes | %s | %ld\n",
            lib -> docs[i].name,
            lib -> docs[i].size,
            data,
            lib -> docs[i].offset
        );
    }

    return 1;
}

int gbv_view( const Library *lib, const char *docname ) {
    FILE *gbv;
    char buffer[ BUFFER_SIZE ];
    int aux = -1, bloco = 0;
    char comando;

    /* Abre o arquivo da biblioteca.
     * Foi assumido um valor padrão para o nome do arquivo. */
    if ( !( gbv = fopen( "biblioteca.gbv", "rb" ) ) )
        return 0;

    /* Procura pelo documento. */
    for ( int i = 0; i < lib -> count; i++ ) {
        if ( strcmp( lib -> docs[i].name, docname ) == 0 ) {
            aux = i;
            break;
        }
    }

    /* Documento não encontrado. */
    if ( aux == -1 ) {
        fclose( gbv );
        return 0;
    }

    /* Documento encontrado. */
    Document doc = lib -> docs[ aux ];

    while ( 1 ) {
        long pos = doc.offset + bloco * BUFFER_SIZE;

        /* Não há mais blocos para exibir. */
        if ( pos >= doc.offset + doc.size )
            break;

        if ( fseek( gbv, pos, SEEK_SET ) != 0 ) {
            fclose( gbv );
            return 0;
        }

        /* Calcula quantos bytes ainda faltam do documento. */
        long restantes = doc.size - bloco * BUFFER_SIZE;
        size_t tamanhoLeitura;

        if ( restantes > BUFFER_SIZE )
            tamanhoLeitura = BUFFER_SIZE;
        else
            tamanhoLeitura = restantes;

        size_t lidos = fread( buffer, 1, tamanhoLeitura, gbv );

        fwrite( buffer, 1, lidos, stdout );

        printf( "\n[n/p/q]: " );
        scanf( " %c", &comando );

        if ( comando == 'n' ) {
            if ( pos + BUFFER_SIZE < doc.offset + doc.size )
                bloco++;
        }
        else if ( comando == 'p' ) {
            if ( bloco > 0 )
                bloco--;
        }
        else if ( comando == 'q' ) {
            break;
        }

        printf( "\n" );
    }

    fclose( gbv );

    return 1;
}

int comparaNome( const void *a, const void *b ) {
    return strcmp( ( ( Document * ) a ) -> name, ( ( Document * ) b ) -> name );
}

int comparaTamanho( const void *a, const void *b ) {
    return ( ( Document * ) a ) -> size - ( ( Document * ) b ) -> size;
}

int comparaData( const void *a, const void *b ) {
    return ( ( Document * ) a ) -> date - ( ( Document * ) b ) -> date;
}

int gbv_order( Library *lib, const char *archive, const char *criteria ) {
    FILE *gbv = NULL, *temp = NULL, *final = NULL;
    superBloco sb;
    char buffer[BUFFER_SIZE];
    size_t lidos;

    /* Abre o arquivo da biblioteca. */
    if ( !( gbv = fopen( archive, "rb" ) ) )
        return 0;

    /* Abre o arquivo temporário. */
    if ( !( temp = fopen( "tmp.gbv", "wb" ) ) ) {
        fclose( gbv );
    
        return 0;
    }

    /* Ordena os documentos de acordo com o critério especificado. */
    if ( strcmp( criteria, "nome" ) == 0 )
        qsort( lib -> docs, lib -> count, sizeof( Document ), comparaNome );
    else if ( strcmp( criteria, "tamanho" ) == 0 )
        qsort( lib -> docs, lib -> count, sizeof( Document ), comparaTamanho );
    else if ( strcmp( criteria, "data" ) == 0 )
        qsort( lib -> docs, lib -> count, sizeof( Document ), comparaData );

    /* Escreve o super bloco no arquivo temporário. */
    sb.numDocs = lib -> count;
    fwrite( &sb, sizeof( superBloco ), 1, temp );


    for ( int i = 0; i < lib -> count; i++ ) {
        Document *d = &lib -> docs[i];
        long restante = d -> size;
        long novoOffset = ftell( temp );

        fseek( gbv, d -> offset, SEEK_SET );

        /* Vai lendo o documento em blocos.
         * Adiciona-os no arquivo temporário. */
        while ( restante > 0 ) {
            int bloco;
            if ( restante > BUFFER_SIZE )
                bloco = BUFFER_SIZE;
            else
                bloco = restante;
            fread( buffer, 1, bloco, gbv );
            fwrite( buffer, 1, bloco, temp );
            restante -= bloco;
        }

        d -> offset = novoOffset;
    }

    sb.offsetDir = ftell( temp );
    fwrite( lib -> docs, sizeof( Document ), lib -> count, temp );

    /* Escreve o super bloco no início do arquivo temporário. */
    fseek( temp, 0, SEEK_SET );
    fwrite( &sb, sizeof( superBloco ), 1, temp );

    fclose( gbv );
    fclose( temp );

    if ( !( gbv = fopen( "tmp.gbv", "rb" ) ) )
        return 0;

    if ( !( final = fopen( archive, "wb" ) ) ) {
        fclose( gbv );
    
        return 0;
    }

    /* Copia o conteúdo do arquivo temporário para o arquivo final. */
    while ( ( lidos = fread( buffer, 1, BUFFER_SIZE, gbv ) ) > 0 )
        fwrite( buffer, 1, lidos, final );

    fclose( gbv );
    fclose( final );

    remove( "tmp.gbv" ); /* Deleta o arquivo temporário. */
    
    return 1;
}