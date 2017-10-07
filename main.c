#include "bgg.h"
#include "assets.h"

static var_t *x_skipRain;

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

static void PlayThunder_f( void ) {
    Mix_PlayChannel( -1, ast_wavThunder, 0 );
    int delay = COM_RandInRange( 15000, 30000 );
    CON_Printf( "Thunder going off. Next in %d seconds\n", delay / 1000 );
    ExecuteAfterDelay( delay, PlayThunder_f );
}

static void PlayCasingFalling_f( void ) {
    Mix_PlayChannel( -1, ast_wavCasingFallThick, 0 );
}

static void X_Shoot_f( void ) {
    Mix_PlayChannel( -1, ast_wavGunshot, 0 );
    if ( ( COM_Rand() & 1023 ) < 100 ) {
        ExecuteAfterDelay( COM_RandInRange( 300, 500 ), PlayCasingFalling_f );
    }
}

static void X_RegisterVars_f( void ) {
    AST_RegisterVars();
    x_skipRain = VAR_Register( "x_skipRain", "0" );
    CMD_Register( "x_shoot", X_Shoot_f );
    I_Bind( "mouse left button", "+x_shoot" );
}

static void X_Init_f( void ) {
    AST_Init();
    Mix_PlayMusic( ast_musRain, -1 );
    if ( VAR_Num( x_skipRain ) ) {
        Mix_PauseMusic();
    }
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
    AST_Frame();
}

static void X_Done_f( void ) {
    AST_Done();
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
