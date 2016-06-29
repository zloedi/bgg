#include "zhost.h"
 
typedef struct {
    byte *bits;
    c2_t size;
    rImage_t *image;
} xTex_t;

static xTex_t x_maze;
static xTex_t x_view;
static var_t *x_mazeWidth;
static var_t *x_mazeHeight;
static var_t *x_losRadius;
static rImage_t *x_cp437Texture;
static v2_t x_cp437TextureSize;

void RasterizeFOVOctant( int originX, int originY,
                         int radius, 
                         int bitmapWidth, int bitmapHeight,
                         int octant,
                         const unsigned char *inTerrain, 
                         unsigned char *outBitmap );

void GenerateTestMaze( c2_t mazeSize, byte *maze ) {
    int sz = mazeSize.x * mazeSize.y;
    memset( maze, 0, sz );

    int numPixels = sz / 100;
    int numRects = sz / 50;
    int minRectSide = Mini( mazeSize.x, mazeSize.y ) / 64;
    int maxRectSide = minRectSide * 8;

    for ( int i = 0; i < numPixels; i++ ) {
        c2_t rndPt = c2xy( COM_Rand() % mazeSize.x, COM_Rand() % mazeSize.y );
        maze[rndPt.x + mazeSize.x * rndPt.y] = 255;
    }

    for ( int i = 0; i < numRects; i++ ) {
        c2_t rndPt = c2xy( COM_Rand() % mazeSize.x, COM_Rand() % mazeSize.y );
        c2_t rndSz = c2xy( COM_RandInRange( minRectSide, maxRectSide ), COM_RandInRange( minRectSide, maxRectSide ) );
        COM_RasterizeRectangle8( rndPt, rndSz, 255, mazeSize, maze );
    }

    for ( int i = 0; i < numRects / 4; i++ ) {
        c2_t rndPt = c2xy( COM_Rand() % mazeSize.x, COM_Rand() % mazeSize.y );
        c2_t rndSz = c2xy( COM_RandInRange( minRectSide, maxRectSide ), COM_RandInRange( minRectSide, maxRectSide ) );
        COM_RasterizeRectangle8( rndPt, rndSz, 0, mazeSize, maze );
    }
}

static void X_DrawCursor( rImage_t *texture, v2_t texSize, v2_t position ) {
    v2_t size = v2Scale( texSize, 1 / 16. );
    v2_t st0 = v2xy( 10 / 16., 13 / 16. );
    v2_t st1 = v2xy( 11 / 16., 14 / 16. );
    v2_t origin = v2Sub( position, v2Scale( size, 0.5 ) );
    R_DrawPicV2( origin, size, st0, st1, x_cp437Texture );
    R_DrawPicV2( v2Add( origin, v2Scale( size, 0.25 ) ), size, st0, st1, x_cp437Texture );
    R_DrawPicV2( v2Add( origin, v2Scale( size, 0.5 ) ), size, st0, st1, x_cp437Texture );
}

static void X_RegisterVars_f( void ) {
    x_mazeWidth = VAR_Register( "x_mazeWidth", "256" );
    x_mazeHeight = VAR_Register( "x_mazeHeight", "256" );
    x_losRadius = VAR_Register( "x_losRadius", "32" );
}

static void X_Init_f( void ) {
    x_cp437Texture = R_LoadTextureEx( "cp437_12x12.png", &x_cp437TextureSize );
    x_maze.image = R_BlankTexture();
    x_view.image = R_BlankTexture();
    R_ShowCursor( false );
}

static void X_Frame_f( void ) {
    if ( ! x_maze.bits || VAR_Changed( x_mazeWidth ) || VAR_Changed( x_mazeHeight ) ) {
        x_maze.size = x_view.size = c2xy( Clampf( VAR_Num( x_mazeWidth ), 64, 1024 ), 
                            Clampf( VAR_Num( x_mazeHeight ), 64, 1024 ) );
        int sz = x_maze.size.x * x_maze.size.y;
        x_maze.bits = A_Realloc( x_maze.bits, sz * sizeof( *x_maze.bits ) );
        x_view.bits = A_Realloc( x_view.bits, sz * sizeof( *x_view.bits ) );
        GenerateTestMaze( x_maze.size, x_maze.bits );
        R_BlitToTexture( x_maze.image, x_maze.bits, x_maze.size.x, x_maze.size.y, 1 );
        CON_Printf( "changed size of the maze to %d,%d", x_maze.size.x, x_maze.size.y );
    }
    v2_t windowSize = R_GetWindowSize();
    float scale = windowSize.y / ( float )x_maze.size.y;
    v2_t mouse = I_GetMousePosition();
    v2_t origin = v2xy( mouse.x / scale, mouse.y / scale );
    memset( x_view.bits, 0, sizeof( *x_view.bits ) * x_view.size.x * x_view.size.y );
    // draw the textures before rasterizing
    // so we can draw debug stuff in the raster routine
    R_ColorC( colGreen );
    R_DrawPic( 0, 0, x_maze.size.x * scale, windowSize.y, 0, 0, 1, 1, x_maze.image );
    R_ColorC( colWhite );
    R_DrawPic( 0, 0, x_view.size.x * scale, windowSize.y, 0, 0, 1, 1, x_view.image );
    for ( int i = 0; i < 8; i++ ) {
        RasterizeFOVOctant( origin.x, origin.y,
                            Clampf( VAR_Num( x_losRadius ), 0, 1024 ), 
                            x_maze.size.x, x_maze.size.y,
                            i,
                            x_maze.bits, x_view.bits );
    }
    R_BlitToTexture( x_view.image, x_view.bits, x_view.size.x, x_view.size.y, 1 );
    R_ColorC( colCyan );
    X_DrawCursor( x_cp437Texture, x_cp437TextureSize, mouse );
}

static void X_Done_f( void ) {
    A_Free( x_maze.bits );
    A_Free( x_view.bits );
}

int main( int argc, char *argv[] ) {
    UT_RunApp( "bgg",
               X_RegisterVars_f,
               X_Init_f,
               X_Frame_f,
               X_Done_f );
    return 0;
}
