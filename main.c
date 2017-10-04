#include "zhost.h"
#include "placeholder_wav.h"

static Mix_Chunk *x_sampleShoot;

static void X_Shoot_f( void ) {
    Mix_PlayChannel( -1, x_sampleShoot, 0 );
}

static void X_RegisterVars_f( void ) {
    CMD_Register( "x_shoot", X_Shoot_f );
    I_Bind( "mouse left button", "+x_shoot" );
}

static void X_Init_f( void ) {
    SDL_RWops *buffer = SDL_RWFromMem( placeholder_wav, placeholder_wav_len );
    x_sampleShoot = Mix_LoadWAV_RW( buffer, false );
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
