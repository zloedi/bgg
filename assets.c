#include "bgg.h"
#include "placeholder_wav.h"
#include "assets.h"
#include "stb_rect_pack.h"

astImage_t ast_imgTileset;
astImage_t ast_imgAtlas;
Mix_Music *ast_musRain;

Mix_Chunk *ast_wavThunder;
Mix_Chunk *ast_wavGunshot;
Mix_Chunk *ast_wavCasingFallThick;

static Mix_Chunk *ast_wavFallback;
static Mix_Music *ast_musFallback;
static astImage_t ast_imgFallback;
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

static astImage_t AST_LoadImage( const char *name ) {
    c2_t size;
    int bytesPerPixel;
    byte *bitmap = R_LoadImageRaw( name, &size, &bytesPerPixel, 0 );
    if ( bitmap ) {
        SDL_Texture *tex = R_CreateStaticTexFromBitmap( bitmap, size, bytesPerPixel );
        astImage_t result = {
            .sizeC = size,
            .sizeV = v2c2( size ),
            .bitmap = bitmap,
            .texture = tex,
        };
        return result;
    }
    return ast_imgFallback;
}

static void AST_DestroyImage( astImage_t *img ) {
    A_Free( img->bitmap );
    img->bitmap = NULL;
}

void AST_Init( void ) {
    ast_imgFallback.sizeC = c2one,
    ast_imgFallback.sizeV = v2one,
    ast_imgFallback.bitmap = A_MallocZero( 1 ),
    ast_imgFallback.texture = R_CreateStaticTexFromBitmap( ast_imgFallback.bitmap, c2one, 1 );
    ast_imgTileset = AST_LoadImage( "cp437_12x12.png" );
    ast_musFallback = Mix_LoadMUS_RW( SDL_RWFromMem( placeholder_wav, placeholder_wav_len ), false );
    ast_musRain = AST_LoadMusic( "rain.ogg" );
    ast_wavFallback = Mix_LoadWAV_RW( SDL_RWFromMem( placeholder_wav, placeholder_wav_len ), false );
    ast_wavThunder = AST_LoadSample( "thunder.ogg" );
    ast_wavGunshot = AST_LoadSample( "gunshot.ogg" );
    ast_wavCasingFallThick = AST_LoadSample( "casing_falling_thick.ogg" );
}

static void AST_DrawTileset( v2_t position, color_t color ) {
    SDL_SetTextureColorMod( ast_imgTileset.texture,
            ( Uint8 )( color.r * 255 ), 
            ( Uint8 )( color.g * 255 ),
            ( Uint8 )( color.b * 255 ) );
    SDL_SetTextureAlphaMod( ast_imgTileset.texture, ( Uint8 )( color.alpha * 255 ) );
    SDL_SetTextureBlendMode( ast_imgTileset.texture, SDL_BLENDMODE_BLEND );
    SDL_Rect dest = {
        .x = position.x,
        .y = position.y,
        .w = ast_imgTileset.sizeC.x,
        .h = ast_imgTileset.sizeC.y,
    };
    SDL_RenderCopy( r_renderer, ast_imgTileset.texture, NULL, &dest );
}

void AST_Frame( void ) {
    if ( VAR_Num( ast_showTileset ) ) {
        //PrintInt( ast_imgTileset.size.x );
        AST_DrawTileset( v2xy( R_GetWindowSize().x - ast_imgTileset.sizeV.x, 0 ), colWhite );
    }
}

void AST_Done( void ) {
    AST_DestroyImage( &ast_imgAtlas );
    AST_DestroyImage( &ast_imgTileset );
    AST_DestroyImage( &ast_imgFallback );
}

void AST_RegisterVars( void ) {
    ast_showTileset = VAR_Register( "ast_showTileset", "0" );
}

