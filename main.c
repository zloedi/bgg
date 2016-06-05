#include "zhost.h"
 
static rImage_t *x_cp437Texture;
static float    x_cp437TextureWidth;

static void X_DrawCursor( v2_t position ) {
    float curDim = x_cp437TextureWidth / 16. * Maxi( R_GetWindowSize().y / 512, 1 );
    v2_t size = v2xy( curDim, curDim );
    v2_t st0 = v2xy( 10 / 16., 13 / 16. );
    v2_t st1 = v2xy( 11 / 16., 14 / 16. );
    v2_t origin = v2Sub( position, v2Scale( size, 0.5 ) );
    R_DrawPicV2( origin, size, st0, st1, x_cp437Texture );
    R_DrawPicV2( 
            v2Add( origin, v2Scale( size, 0.25 ) ), 
            size, 
            st0, 
            st1, 
            x_cp437Texture );
}

static void X_RegisterVars_f( void ) {
}

static void X_Init_f( void ) {
    int w, h;
    x_cp437Texture = R_LoadTextureEx( "cp437_12x12.png", &w, &h );
    if ( w != h ) {
        CON_Printf( "X_Init_f: non-square code page 437 texture. Using fallback texture instead.\n" );
        x_cp437Texture = r_fallbackTexture;
        x_cp437TextureWidth = 1;
    } else {
        x_cp437TextureWidth = w;
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
