#include "zhost.h"
 
static rImage_t *x_cp437Texture;

static void X_RegisterVars_f( void ) {
}

static void X_Init_f( void ) {
    x_cp437Texture = R_LoadTexture( "cp437_12x12.png" );
}

static void X_Frame_f( const utFrameParams_t *params ) {
    R_ColorC( colWhite );
    R_DrawPicV2( params->cursorPosition, v2xy( 20, 20 ), v2zero, v2one, x_cp437Texture );
    CON_Printf( "time delta: %d\n", params->timeDelta );
}

static void X_Done_f( void ) {
}

int main( int argc, char *argv[] ) {
    UT_RunApp( NULL, 
               "bgg",
               "Bullets, Grit, Gasoline",
               colorrgb( 0.1f, 0.1f, 0.1f ),
               X_RegisterVars_f,
               X_Init_f,
               X_Frame_f,
               X_Done_f );
    return 0;
}
