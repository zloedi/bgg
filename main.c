#include "bgg.h"
#include "assets.h"

static var_t *x_skipRain;

typedef void (*futureAction_t) ( void );

typedef struct {
    int timeInTheFuture;
    futureAction_t action;
} future_t;

typedef enum {
    CT_RENDERER,
    CT_NUM_TYPES,
} compType_t;

typedef struct {
    int parent;
    v2_t down;
    v2_t right;
    v2_t position;
    v2_t localDown;
    v2_t localRight;
    v2_t localPosition;
    void *components[CT_NUM_TYPES];
} compEntity_t;

typedef struct {
    int entity;
    astSprite_t sprite;
    color_t color;
} compRenderer_t;

#define HUNK_CHANGE_SEQUENCE SYS_RealTime
#define HUNK_SIZE (128*1024)

typedef struct {
    int numElements;
    int changeStamp;
    byte buffer[];
} hunk_t;

const size_t hunkHeadSize = ( size_t )&((hunk_t*)0)->buffer;

#define HunkPushBack(a,v) ((a)[_HunkHead(a)->numElements++]=(v),_HunkChange(a))
#define HunkLast(a) ((a)[_HunkHead(a)->numElements-1])

static inline hunk_t* _HunkHead( void *a ) {
    return ( hunk_t* )( ( byte* )a - hunkHeadSize );
}

static inline void _HunkChange( void *a ) {
    _HunkHead( a )->changeStamp = HUNK_CHANGE_SEQUENCE();
}

static inline void* HunkAlloc( size_t size ) {
    return ( ( hunk_t* )A_Static( hunkHeadSize + size ) )->buffer;
}

static inline bool_t HunkChanged( void *a ) {
    return _HunkHead( a )->changeStamp == HUNK_CHANGE_SEQUENCE();
}

static inline void HunkPopBack( void *a ) {
    _HunkHead( a )->numElements--;
    _HunkChange( a );
}

static inline int HunkNumElems( void* a ) {
    return _HunkHead( a )->numElements;
}

static void *x_compBins[CT_NUM_TYPES];

//static int x_transformsSequence;
//static compEntity_t *x_transforms[2];

//static compEntity_t* GetTransforms( int sequence ) {
//    return x_transforms[sequence & 1];
//}

static compRenderer_t* GetRenderers( void ) {
    return x_compBins[CT_RENDERER];
}

static void CreateRenderer( int entity, astSprite_t sprite, compRenderer_t *renderers ) {
    //int numElements = *( ( int* )renderers - 1 );
    //if ( entity == 0 ) {
    //    CON_Printf( CreateRenderer
    //}
    compRenderer_t rend = {
        .color = colWhite,
        .sprite = sprite,
    };
    HunkPushBack( renderers, rend );
}

static void UpdateRenderers( compRenderer_t *renderers ) {
}

static void SetParentTransform( int this, int parent ) {
}

static void DeleteTransform( int entity ) {
    //for ( int i = entity; ; i++ ) {
    //    compEntity_t *t = &x_transforms[i];
    //    if ( t->parent == INVALID_TRANSFORM_PARENT ) {
    //        break;
    //    }
    //    if ( t->parent < entity ) {
    //        break;
    //    }
    //    t->parent = INVALID_TRANSFORM_PARENT;
    //}
    //x_transformsNeedSort = true;
}

static void* GetComponent( compEntity_t *ent, compType_t type ) {
    return ent->components[type];
}

#define MAX_FUTURES 256

static int x_numFutures;
static future_t x_futures[MAX_FUTURES];

static void DrawCenteredTile( int index, v2_t screenPos, float scale, float angle ) {
    astSprite_t *tileset = &ast_sprites[AST_SPR_TILESET];
    SDL_SetTextureColorMod( tileset->atlas->texture, 255, 255, 255 );
    SDL_SetTextureAlphaMod( tileset->atlas->texture, 255 );
    SDL_SetTextureBlendMode( tileset->atlas->texture, SDL_BLENDMODE_BLEND );
    c2_t st = c2xy( ( index & 15 ) * ast_tileSizeC.x, ( index / 16 ) * ast_tileSizeC.y );
    st = c2Add( st, tileset->originInAtlas );
    SDL_Rect src = {
        .x = st.x,
        .y = st.y,
        .w = ast_tileSizeC.x,
        .h = ast_tileSizeC.y,
    };
    SDL_Rect dst = {
        .x = screenPos.x - scale * ast_tileSizeC.x / 2,
        .y = screenPos.y - scale * ast_tileSizeC.y / 2,
        .w = ast_tileSizeC.x * scale,
        .h = ast_tileSizeC.y * scale,
    };
    SDL_RenderCopyEx(r_renderer,
                     tileset->atlas->texture,
                     &src,
                     &dst,
                     angle,
                     NULL,
                     SDL_FLIP_NONE);
}

static void DrawCharacter( v2_t screenPos, v2_t direction, float scale ) {
    float angle = atan2( direction.y, direction.x ) / ( M_PI * 2 ) * 360;
    DrawCenteredTile( '@', screenPos, scale, 0 );//3 + sin( SYS_RealTime() * 0.01 ), SYS_RealTime() * 0.1 );
    DrawCenteredTile( '(', screenPos, scale, angle );//3 + sin( SYS_RealTime() * 0.01 ), SYS_RealTime() * 0.1 );
}

static void ExecuteAfterDelay( int delayMs, futureAction_t action ) {
    if ( x_numFutures < MAX_FUTURES ) {
        future_t new = {
            .timeInTheFuture = SYS_RealTime() + delayMs,
            .action = action,
        };
        x_futures[x_numFutures] = new;
        x_numFutures++;
    }
}

static void UpdateFutures( void ) {
    for ( int i = x_numFutures - 1; i >= 0; i-- ) {
        future_t *f = &x_futures[i];
        if ( f->timeInTheFuture <= SYS_RealTime() ) {
            f->action();
            *f = x_futures[x_numFutures - 1];
            x_numFutures--;
        }
    }
}

static void PlayThunder_f( void ) {
    Mix_PlayChannel( -1, ast_wavThunder, 0 );
    int delay = COM_RandInRange( 15000, 30000 );
    CON_Printf( "Thunder going off. Next in %d seconds\n", delay / 1000 );
    ExecuteAfterDelay( delay, PlayThunder_f );
}

static void PlayCasingFalling_f( void ) {
    Mix_PlayChannel( -1, ast_wavCasingFallThick, 0 );
}

static void X_Shoot_f( void ) {
    Mix_PlayChannel( -1, ast_wavGunshot, 0 );
    if ( ( COM_Rand() & 1023 ) < 100 ) {
        ExecuteAfterDelay( COM_RandInRange( 300, 500 ), PlayCasingFalling_f );
    }
}

static void X_RegisterVars_f( void ) {
    VAR_SetCFGVersion( 1 );
    AST_RegisterVars();
    x_skipRain = VAR_Register( "x_skipRain", "0" );
    CMD_Register( "x_shoot", X_Shoot_f );
    I_Bind( "mouse left button", "!x_shoot" );
}

static void X_Init_f( void ) {
    AST_Init();
    WLD_Init();
    for ( int i = 0; i < CT_NUM_TYPES; i++ ) {
        x_compBins[i] = HunkAlloc( HUNK_SIZE );
    }
    //x_transforms[0] = x_compBins[CT_TRANSFORM];
    //x_transforms[1] = HunkAlloc( HUNK_SIZE );

    Mix_PlayMusic( ast_musRain, -1 );
    if ( VAR_Num( x_skipRain ) ) {
        Mix_PauseMusic();
    }
    ExecuteAfterDelay( 2000, PlayThunder_f );
}

static void X_Frame_f( void ) {
    UpdateFutures();
    if ( VAR_Changed( x_skipRain ) ) {
        if ( VAR_Num( x_skipRain ) ) { 
            Mix_PauseMusic();
        } else {
            Mix_ResumeMusic();
        }
    }
    v2_t origin = v2Scale( R_GetWindowSize(), 0.5 );
    v2_t toMouse = v2Sub( I_GetMousePositionV(), origin );
    v2_t direction = v2Norm( toMouse );
    DrawCharacter( origin, direction, 8 );
    AST_Frame();
}

static void X_Done_f( void ) {
    AST_Done();
}

int main( int argc, char *argv[] ) {
    UT_RunApp( "bgg",
               X_RegisterVars_f,
               X_Init_f,
               X_Frame_f,
               X_Done_f,
               0 );
    return 0;
}
