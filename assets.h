typedef struct {
    c2_t sizeC;
    v2_t sizeV;
    byte *bits;
} astBitmap_t;

typedef struct {
    astBitmap_t bitmap;
    SDL_Texture *texture;
} astImage_t;

typedef struct {
    c2_t originInAtlas;
    c2_t sizeC;
    v2_t sizeV;
    astImage_t *atlas;
} astSprite_t;

typedef enum {
    AST_SPR_TILESET,
    AST_SPR_DISK,
    AST_NUM_SPRITES,
} astSpriteId_t;

astSprite_t ast_sprites[AST_NUM_SPRITES];
c2_t ast_tileSizeC;
v2_t ast_tileSizeV;
Mix_Music *ast_musRain;
Mix_Chunk *ast_wavThunder;
Mix_Chunk *ast_wavGunshot;
Mix_Chunk *ast_wavCasingFallThick;

void AST_Init( void );
void AST_Done( void );
void AST_Frame( void );
void AST_RegisterVars( void );
