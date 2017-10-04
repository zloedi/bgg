#include "zhost.h"
#include "placeholder_wav.h"

static var_t *x_skipRain;

static Mix_Music *x_musicPlaceholder;
static Mix_Music *x_musicRain;

static Mix_Chunk *x_samplePlaceholder;
static Mix_Chunk *x_sampleThunder;
static Mix_Chunk *x_sampleGunshot;
static Mix_Chunk *x_sampleCasingFallThick;
static Mix_Chunk *x_sampleCasingFallThin;

typedef void (*futureAction_t) ( void );

typedef struct {
    int timeInTheFuture;
    futureAction_t action;
} future_t;

#define MAX_FUTURES 256

static int x_numFutures;
static future_t x_futures[MAX_FUTURES];

static void ExecuteAfterDelay( int delayMs, futureAction_t action ) {
    if ( x_numFutures < MAX_FUTURES ) {
        future_t new = {
            .timeInTheFuture = SYS_RealTime() + delayMs,
            .action = action,
        };
        x_futures[x_numFutures] = new;
        x_numFutures++;
    }
}

static void UpdateFutures( void ) {
    for ( int i = x_numFutures - 1; i >= 0; i-- ) {
        future_t *f = &x_futures[i];
        if ( f->timeInTheFuture <= SYS_RealTime() ) {
            f->action();
            *f = x_futures[x_numFutures - 1];
            x_numFutures--;
        }
    }
}

static Mix_Music* LoadMusic( const char *name ) {
    Mix_Music *music = Mix_LoadMUS( va( "%sdata/%s", SYS_BaseDir(), name ) );
    if ( ! music ) {
        CON_Printf( "LoadMusic: failed to load %s\n", name );
        music = x_musicPlaceholder;
    }
    return music;
}

static Mix_Chunk* LoadSample( const char *name ) {
    Mix_Chunk *chunk = Mix_LoadWAV( va( "%sdata/%s", SYS_BaseDir(), name ) );
    if ( ! chunk ) {
        CON_Printf( "LoadSample: failed to load %s\n", name );
        chunk = x_samplePlaceholder;
    }
    return chunk;
}

static void PlayThunder_f( void ) {
    Mix_PlayChannel( -1, x_sampleThunder, 0 );
    int delay = COM_RandInRange( 15000, 30000 );
    CON_Printf( "Thunder going off. Next in %d seconds\n", delay / 1000 );
    ExecuteAfterDelay( delay, PlayThunder_f );
}

static void PlayCasingFalling_f( void ) {
    Mix_PlayChannel( -1, x_sampleCasingFallThin, 0 );
    //Mix_PlayChannel( -1, x_sampleCasingFallThick, 0 );
}

static void X_Shoot_f( void ) {
    Mix_PlayChannel( -1, x_sampleGunshot, 0 );
    if ( ( COM_Rand() & 1023 ) < 100 ) {
        ExecuteAfterDelay( COM_RandInRange( 300, 500 ), PlayCasingFalling_f );
    }
}

static void X_RegisterVars_f( void ) {
    x_skipRain = VAR_Register( "x_skipRain", "0" );
    CMD_Register( "x_shoot", X_Shoot_f );
    I_Bind( "mouse left button", "+x_shoot" );
}

static void X_Init_f( void ) {
    SDL_RWops *buffer = SDL_RWFromMem( placeholder_wav, placeholder_wav_len );
    x_musicPlaceholder = Mix_LoadMUS_RW( buffer, false );
    x_musicRain = LoadMusic( "rain.wav" );
    Mix_PlayMusic( x_musicRain, -1 );
    if ( VAR_Num( x_skipRain ) ) {
        Mix_PauseMusic();
    }
    x_samplePlaceholder = Mix_LoadWAV_RW( buffer, false );
    x_sampleThunder = LoadSample( "thunder.wav" );
    x_sampleGunshot = LoadSample( "gunshot.wav" );
    x_sampleCasingFallThick = LoadSample( "casing_falling_thick.wav" );
    x_sampleCasingFallThin = LoadSample( "casing_falling_thin.wav" );
    ExecuteAfterDelay( 2000, PlayThunder_f );
}

static void X_Frame_f( void ) {
    UpdateFutures();
    if ( VAR_Changed( x_skipRain ) ) {
        if ( VAR_Num( x_skipRain ) ) { 
            Mix_PauseMusic();
        } else {
            Mix_ResumeMusic();
        }
    }
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
