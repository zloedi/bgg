#include "bgg.h"
#include "placeholder_wav.h"
#include "assets.h"
#define STB_RECT_PACK_IMPLEMENTATION
#include "stb_rect_pack.h"

astImage_t ast_imgAtlas;
astSprite_t ast_sprites[AST_NUM_SPRITES];
Mix_Music *ast_musRain;

Mix_Chunk *ast_wavThunder;
Mix_Chunk *ast_wavGunshot;
Mix_Chunk *ast_wavCasingFallThick;

c2_t tileSizeC;
v2_t tileSizeV;

static Mix_Chunk *ast_wavFallback;
static Mix_Music *ast_musFallback;
static astBitmap_t ast_bmpFallback;
static astImage_t ast_imgFallback;
static var_t *ast_showAtlas;

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

static astBitmap_t AST_BitmapFromMem( byte *mem, c2_t size ) {
    astBitmap_t result = {
        .sizeC = size,
        .sizeV = v2c2( size ),
        .bits = mem,
    };
    return result;
}

static bool_t AST_LoadBitmap( const char *name, astBitmap_t *outBitmap ) {
    c2_t size;
    int bytesPerPixel;
    byte *bits = R_LoadImageRaw( name, &size, &bytesPerPixel, 0 );
    if ( bits ) {
        astBitmap_t result = {
            .sizeC = size,
            .sizeV = v2c2( size ),
            .bits = bits,
        };
        *outBitmap = result;
        return true;
    }
    CON_Printf( "LoadBitmap: failed to load %s\n", name );
    *outBitmap = ast_bmpFallback;
    return false;
}

//static astImage_t AST_LoadImage( const char *name ) {
//    astBitmap_t outBitmap;
//    if ( AST_LoadBitmap( name, &outBitmap ) ) {
//        astImage_t result = {
//            .bitmap = outBitmap,
//            .texture = R_CreateStaticTexFromBitmap( outBitmap.bits, outBitmap.sizeC, 1 ),
//        };
//        return result;
//    }
//    return ast_imgFallback;
//}

static void AST_DestroyImage( astImage_t *img ) {
    A_Free( img->bitmap.bits );
    img->bitmap.bits = NULL;
}

static void AST_PushToPacker( astBitmap_t *bitmaps, int id, stbrp_rect **ioRects ) {
    astBitmap_t *bitmap = &bitmaps[id];
    stbrp_rect r = {
        .id = id,
        .w = bitmap->sizeC.x,
        .h = bitmap->sizeC.y,
    };
    PrintC2( bitmap->sizeC );
    stb_sb_push( *ioRects, r );
}

static byte* AST_CreateDiskBitmap( byte *buffer, c2_t size ) {
    c2_t c;
    int r = Mini( size.x, size.y ) / 2;
    int rsq = r * r;
    for ( c.y = 0; c.y <= r; c.y++ ) {
        for ( c.x = 0; c.x <= r; c.x++ ) {
            c2_t d = c2Sub( c, c2xy( r, r ) );
            int val = c2Dot( d, d ) < rsq ? 255 : 0;
            buffer[c.x              + c.y * size.y] = val;
            buffer[size.x - c.x - 1 + c.y * size.y] = val;
            buffer[c.x              + ( size.y - c.y - 1 ) * size.y] = val;
            buffer[size.x - c.x - 1 + ( size.y - c.y - 1 ) * size.y] = val;
        }
    }
    return buffer;
}

static bool_t AST_PackRects( stbrp_rect *rects,c2_t *ioAtlasSize ) {
    c2_t atlasSize = *ioAtlasSize;
    for ( int i = 0; i < 10; i++ ) {
        stbrp_context context = {0};
        int numNodes = atlasSize.x;
        stbrp_node nodes[numNodes];
        stbrp_init_target( &context, atlasSize.x, atlasSize.y, nodes, numNodes );
        if ( stbrp_pack_rects( &context, rects, stb_sb_count( rects ) ) ) {
            CON_Printf( "Packed all atlas rects. Atlas size: %d,%d\n", atlasSize.x, atlasSize.y );
            *ioAtlasSize = atlasSize;
            return true;
        }
        atlasSize = c2Scale( atlasSize, 2 );
    }
    CON_Printf( "PackRects: Failed to pack atlas rects\n" );
    *ioAtlasSize = c2zero;
    return false;
} 

static void AST_CopyBitmap( byte *src, c2_t srcSize, byte *dst, c2_t dstOrigin, int dstPitch ) {
    for ( int i = 0, y = 0; y < srcSize.y; y++ ) {
        int start = dstOrigin.x + y * dstPitch;
        for ( int x = 0; x < srcSize.x; i++, x++ ) {
            dst[start + x] = src[i];
        }
    }
}

void AST_Init( void ) {
    ast_bmpFallback.sizeC = c2one;
    ast_bmpFallback.sizeV = v2one;
    ast_bmpFallback.bits = A_MallocZero( 1 );
    ast_imgFallback.bitmap = ast_bmpFallback;
    ast_imgFallback.texture = R_CreateStaticTexFromBitmap( ast_imgFallback.bitmap.bits, c2one, 1 );
    stbrp_rect *rects = NULL;
    astBitmap_t bitmaps[AST_NUM_SPRITES];
    astBitmap_t *tileset = &bitmaps[AST_SPR_TILESET];
    AST_LoadBitmap( "cp437_12x12.png", tileset );
    ast_tileSizeC = c2Divs( tileset->sizeC, 16 );
    ast_tileSizeV = v2c2( ast_tileSizeC );
    AST_PushToPacker( bitmaps, AST_SPR_TILESET, &rects );
    c2_t diskSize = c2Scale( ast_tileSizeC, 2 );
    byte disk[c2MulComps( diskSize )];
    bitmaps[AST_SPR_DISK] = AST_BitmapFromMem( AST_CreateDiskBitmap( disk, diskSize ), diskSize );
    AST_PushToPacker( bitmaps, AST_SPR_DISK, &rects );
    c2_t atlasSize = c2one;
    if ( AST_PackRects( rects, &atlasSize ) ) {
        int numBytes = c2MulComps( atlasSize );
        byte *atlasBits = A_MallocZero( numBytes );
        int n = stb_sb_count( rects );
        for ( int i = 0; i < n; i++ ) {
            stbrp_rect *r = &rects[i];
            c2_t sz = c2xy( r->w, r->h );
            astSprite_t s = {
                .atlas = &ast_imgAtlas,
                .originInAtlas = c2xy( r->x, r->y ),
                .sizeC = sz,
                .sizeV = v2c2( sz ),
            };
            astBitmap_t *bmp = &bitmaps[r->id];
            AST_CopyBitmap( bmp->bits, bmp->sizeC, atlasBits, s.originInAtlas, atlasSize.x );
            ast_sprites[r->id] = s;
        }
        astImage_t atlas = {
            .bitmap = AST_BitmapFromMem( atlasBits, atlasSize ),
            .texture = R_CreateStaticTexFromBitmap( atlasBits, atlasSize, 1 ),
        };
        ast_imgAtlas = atlas;
    } else {
        for ( int i = 0; i < AST_NUM_SPRITES; i++ ) {
            astSprite_t s = {
                .atlas = &ast_imgFallback,
                .sizeC = c2one,
                .sizeV = v2one,
            };
            ast_sprites[i] = s;
        }
    }
    A_Free( tileset->bits );
    stb_sb_free( rects );
    ast_musFallback = Mix_LoadMUS_RW( SDL_RWFromMem( placeholder_wav, placeholder_wav_len ), false );
    ast_musRain = AST_LoadMusic( "rain.ogg" );
    ast_wavFallback = Mix_LoadWAV_RW( SDL_RWFromMem( placeholder_wav, placeholder_wav_len ), false );
    ast_wavThunder = AST_LoadSample( "thunder.ogg" );
    ast_wavGunshot = AST_LoadSample( "gunshot.ogg" );
    ast_wavCasingFallThick = AST_LoadSample( "casing_falling_thick.ogg" );
}

void AST_Frame( void ) {
    if ( VAR_Num( ast_showAtlas ) ) {
        v2_t size = v2Scale( ast_imgAtlas.bitmap.sizeV, 2 );
        v2_t origin = v2xy( R_GetWindowSize().x - size.x, 0 );
        SDL_SetTextureColorMod( ast_imgAtlas.texture,
                ( Uint8 )( 255 ), 
                ( Uint8 )( 255 ),
                ( Uint8 )( 255 ) );
        SDL_SetTextureAlphaMod( ast_imgAtlas.texture, ( Uint8 )( 255 ) );
        SDL_SetTextureBlendMode( ast_imgAtlas.texture, SDL_BLENDMODE_BLEND );
        SDL_Rect dest = {
            .x = origin.x,
            .y = origin.y,
            .w = size.x,
            .h = size.y,
        };
        SDL_RenderCopy( r_renderer, ast_imgAtlas.texture, NULL, &dest );
    }
}

void AST_Done( void ) {
    AST_DestroyImage( &ast_imgAtlas );
    AST_DestroyImage( &ast_imgFallback );
}

void AST_RegisterVars( void ) {
    ast_showAtlas = VAR_Register( "ast_showAtlas", "0" );
}

