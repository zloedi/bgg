#include "bgg.h"
#include "placeholder_wav.h"
#include "assets.h"

astVisual_t ast_visTileset;
Mix_Music *ast_musRain;

Mix_Chunk *ast_wavThunder;
Mix_Chunk *ast_wavGunshot;
Mix_Chunk *ast_wavCasingFallThick;

static Mix_Chunk *ast_wavFallback;
static Mix_Music *ast_musFallback;
static astVisual_t ast_visFallback;
static var_t *ast_showTileset;

static const char* AST_GetAssetPath( const char *name ) {
    return va( "%sdata/%s", SYS_BaseDir(), name );
}

static Mix_Music* AST_LoadMusic( const char *name ) {
    Mix_Music *music = Mix_LoadMUS( AST_GetAssetPath( name ) );
    if ( ! music ) {
        CON_Printf( "LoadMusic: failed to load %s\n", name );
        music = ast_musFallback;
    } else {
        CON_Printf( "Loaded music %s\n", name );
    }
    return music;
}

static Mix_Chunk* AST_LoadSample( const char *name ) {
    Mix_Chunk *chunk = Mix_LoadWAV( AST_GetAssetPath( name ) );
    if ( ! chunk ) {
        CON_Printf( "LoadSample: failed to load %s\n", name );
        chunk = ast_wavFallback;
    } else {
        CON_Printf( "Loaded sample %s\n", name );
    }
    return chunk;
}

static astVisual_t AST_LoadVisual( const char *name ) {
    c2_t size;
    int bytesPerPixel;
    // force one byte per pixel, all assets are alpha only
    // but the renderer SDL textures are only RGBA
    byte *bitmap = R_LoadImageRaw( name, &size, &bytesPerPixel, 1 );
    if ( bitmap && bytesPerPixel == 1 ) {
        SDL_Texture *tex = R_CreateStaticTexFromBitmap( bitmap, size, bytesPerPixel );
        astVisual_t result = {
            .sizeC = size,
            .sizeV = v2c2( size ),
            .bitmap = bitmap,
            .texture = tex,
        };
        return result;
    }
    return ast_visFallback;
}

static void AST_DestroyVisual( astVisual_t *vis ) {
    A_Free( vis->bitmap );
    vis->bitmap = NULL;
}

void AST_Init( void ) {
    ast_visFallback.sizeC = c2one,
    ast_visFallback.sizeV = v2one,
    ast_visFallback.bitmap = A_MallocZero( 1 ),
    ast_visFallback.texture = R_CreateStaticTexFromBitmap( ast_visFallback.bitmap, c2one, 1 );
    ast_visTileset = AST_LoadVisual( "cp437_12x12.png" );
    ast_musFallback = Mix_LoadMUS_RW( SDL_RWFromMem( placeholder_wav, placeholder_wav_len ), false );
    ast_musRain = AST_LoadMusic( "rain.ogg" );
    ast_wavFallback = Mix_LoadWAV_RW( SDL_RWFromMem( placeholder_wav, placeholder_wav_len ), false );
    ast_wavThunder = AST_LoadSample( "thunder.ogg" );
    ast_wavGunshot = AST_LoadSample( "gunshot.ogg" );
    ast_wavCasingFallThick = AST_LoadSample( "casing_falling_thick.ogg" );
}

static void AST_DrawTileset( v2_t position, color_t color ) {
    SDL_SetTextureColorMod( ast_visTileset.texture,
            ( Uint8 )( color.r * 255 ), 
            ( Uint8 )( color.g * 255 ),
            ( Uint8 )( color.b * 255 ) );
    SDL_SetTextureAlphaMod( ast_visTileset.texture, ( Uint8 )( color.alpha * 255 ) );
    SDL_SetTextureBlendMode( ast_visTileset.texture, SDL_BLENDMODE_BLEND );
    SDL_Rect dest = {
        .x = position.x,
        .y = position.y,
        .w = ast_visTileset.sizeC.x,
        .h = ast_visTileset.sizeC.y,
    };
    SDL_RenderCopy( r_renderer, ast_visTileset.texture, NULL, &dest );
}

void AST_Frame( void ) {
    if ( VAR_Num( ast_showTileset ) ) {
        //PrintInt( ast_visTileset.size.x );
        AST_DrawTileset( v2xy( R_GetWindowSize().x - ast_visTileset.sizeV.x, 0 ), colWhite );
    }
}

void AST_Done( void ) {
    AST_DestroyVisual( &ast_visTileset );
    AST_DestroyVisual( &ast_visFallback );
}

void AST_RegisterVars( void ) {
    ast_showTileset = VAR_Register( "ast_showTileset", "0" );
}

