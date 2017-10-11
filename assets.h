typedef struct {
    c2_t sizeC;
    v2_t sizeV;
    byte *bitmap;
    SDL_Texture *texture;
} astImage_t;

typedef struct {
    c2_t originInAtlas;
    c2_t sizeC;
    v2_t sizeV;
    SDL_Texture *atlas;
} astSprite_t;

Mix_Music *ast_musRain;
Mix_Chunk *ast_wavThunder;
Mix_Chunk *ast_wavGunshot;
Mix_Chunk *ast_wavCasingFallThick;
astImage_t ast_imgTileset;

void AST_Init( void );
void AST_Done( void );
void AST_Frame( void );
void AST_RegisterVars( void );
