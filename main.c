#include "zhost.h"
 
static rImage_t *x_cp437Texture;
static float    x_textureSize;

static float X_TileSize() {
    return x_textureSize / 16;
}

static float X_TileSTSize() {
    float stScale = 1 / x_textureSize;
    return X_TileSize() * stScale;
}

static void X_DrawCursor( v2_t position ) {
    float stSize = X_TileSTSize();
    v2_t st0 = v2Scale( v2xy( 10, 13 ), stSize );
    v2_t st1 = v2Scale( v2xy( 11, 14 ), stSize );
    float cursorSize = X_TileSize() * 2;
    v2_t origin = v2Add( position, v2xy( -cursorSize * 0.5, -cursorSize * 0.5 ) );
    R_DrawPicV2( origin, v2xy( cursorSize, cursorSize ), st0, st1, x_cp437Texture );
}

//static void X_DrawSprite( v2_t position, float scale, v2_t st, color_t color ) {
//    float stScale = 1 / x_textureSize;
//    float stSize = x_textureSize / 16 * stScale;
//    v2_t st0 = v2Scale( st, stSize );
//    v2_t st1 = v2Add( st0, v2xy( stSize, stSize ) );
//    R_DrawPicV2( position, v2xy( 12.f * scale, 12.f * scale ), st0, st1, x_cp437Texture );
//}

static void X_RegisterVars_f( void ) {
}

static void X_Init_f( void ) {
    int w, h;
    x_cp437Texture = R_LoadTextureEx( "cp437_12x12.png", &w, &h );
    //x_cp437Texture = R_LoadTextureEx( "rexpaint_cp437_10x10.png", &w, &h );
    if ( w != h ) {
        CON_Printf( "X_Init_f: non-square code page 437 texture. Using fallback texture instead.\n" );
        x_cp437Texture = r_fallbackTexture;
        x_textureSize = 1;
    } else {
        x_textureSize = w;
    }
}

static void X_Frame_f( const utFrameParams_t *params ) {
    R_ColorC( colCyan );
    X_DrawCursor( params->cursorPosition );
}

static void X_Done_f( void ) {
}

int main( int argc, char *argv[] ) {
    UT_RunApp( NULL, 
               "bgg",
               "Bullets, Grit, Gasoline",
               false,
               colorrgb( 0.1f, 0.1f, 0.1f ),
               X_RegisterVars_f,
               X_Init_f,
               X_Frame_f,
               X_Done_f );
    return 0;
}
