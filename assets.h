typedef struct {
    c2_t sizeC;
    v2_t sizeV;
    byte *bitmap;
    SDL_Texture *texture;
} astVisual_t;

Mix_Music *ast_musRain;
Mix_Chunk *ast_wavThunder;
Mix_Chunk *ast_wavGunshot;
Mix_Chunk *ast_wavCasingFallThick;
astVisual_t ast_visTileset;

void AST_Init( void );
void AST_Done( void );
void AST_Frame( void );
void AST_RegisterVars( void );
