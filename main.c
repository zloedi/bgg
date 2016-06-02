#include "zhost.h"

static void X_RegisterVars_f( void ) {
}

static void X_Init_f( void ) {
}

static void X_Start_f( void ) {
}

static void X_Frame_f( void ) {
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
               X_Start_f,
               X_Frame_f,
               X_Done_f );
    return 0;
}
