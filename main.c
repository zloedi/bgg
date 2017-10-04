#include "zhost.h"

static void X_RegisterVars_f( void ) {
}

static void X_Init_f( void ) {
}

static void X_Frame_f( void ) {
}

static void X_Done_f( void ) {
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
