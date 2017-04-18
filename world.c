#include "zhost.h"

#define W_CHUNK_SIDE 32
#define W_WORLD_HEIGHT 8

bool_t W_GetCachedChunk( c2_t pos, int **outChunk ) {
    return false;
}

int* W_GenerateChunk( c2_t pos ) {
    return NULL;
}

void W_CacheChunk( c2_t pos, int *chunk ) {
}

int* W_GetChunk( c2_t pos ) {
    int *chunk;
    if ( W_GetCachedChunk( pos, &chunk ) ) {
        return chunk;
    }
    chunk = W_GenerateChunk( pos );
    W_CacheChunk( pos, chunk );
    return chunk;
}

void W_DrawChunk( const int *chunk ) {
}
