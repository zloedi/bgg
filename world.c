#include "zhost.h"

#define W_CHUNK_SIDE 8
#define W_WORLD_HEIGHT 8

bool_t W_GetCachedChunk( c2_t pos, int **outChunk ) {
    return false;
}

int* W_GenerateChunk( c2_t pos ) {
    static byte *bits;
    static rImage_t *img;  
    if ( ! img ) {
        int n;
        c2_t sz;
        bits = R_LoadImageRaw( "noise_256.png", &sz, &n );
        img = R_CreateStaticTexture( bits, sz, n );
    }
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

void W_DrawChunk( const int *chunk, int slice ) {
}
