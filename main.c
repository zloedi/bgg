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

static void DrawCenteredTile( int index, v2_t screenPos, float scale, float angle ) {
    v2_t tileSize = v2Scale( ast_visTileset.sizeV, 1 / 16.0f );
    c2_t st = c2xy( ( index & 15 ) * tileSize.x, ( index / 16 ) * tileSize.y );
    SDL_Rect src = {
        .x = st.x,
        .y = st.y,
        .w = tileSize.x,
        .h = tileSize.y,
    };
    SDL_Rect dst = {
        .x = screenPos.x - scale * tileSize.x / 2,
        .y = screenPos.y - scale * tileSize.y / 2,
        .w = tileSize.x * scale,
        .h = tileSize.y * scale,
    };
    SDL_RenderCopyEx(r_renderer,
                     ast_visTileset.texture,
                     &src,
                     &dst,
                     angle,
                     NULL,
                     SDL_FLIP_NONE);
}

static void DrawCharacter( v2_t screenPos, v2_t direction, float scale ) {
    float angle = atan2( direction.y, direction.x ) / ( M_PI * 2 ) * 360;
    DrawCenteredTile( '@', screenPos, scale, 0 );//3 + sin( SYS_RealTime() * 0.01 ), SYS_RealTime() * 0.1 );
    DrawCenteredTile( '(', screenPos, scale, angle );//3 + sin( SYS_RealTime() * 0.01 ), SYS_RealTime() * 0.1 );
}

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
    SDL_SetTextureColorMod( ast_visTileset.texture, 255, 255, 255 );
    SDL_SetTextureAlphaMod( ast_visTileset.texture, 255 );
    SDL_SetTextureBlendMode( ast_visTileset.texture, SDL_BLENDMODE_BLEND );
    v2_t origin = v2Scale( R_GetWindowSize(), 0.5 );
    v2_t toMouse = v2Sub( I_GetMousePositionV(), origin );
    v2_t direction = v2Norm( toMouse );
    DrawCharacter( origin, direction, 8 );
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
