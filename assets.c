#include "zhost.h"
#include "placeholder_wav.h"

Mix_Music *ast_musRain;

Mix_Chunk *ast_wavThunder;
Mix_Chunk *ast_wavGunshot;
Mix_Chunk *ast_wavCasingFallThick;

static Mix_Chunk *ast_wavPlaceholder;
static Mix_Music *ast_musPlaceholder;

static const char* AST_GetAssetPath( const char *name ) {
    return va( "%sdata/%s", SYS_BaseDir(), name );
}

static Mix_Music* AST_LoadMusic( const char *name ) {
    Mix_Music *music = Mix_LoadMUS( AST_GetAssetPath( name ) );
    if ( ! music ) {
        CON_Printf( "LoadMusic: failed to load %s\n", name );
        music = ast_musPlaceholder;
    } else {
        CON_Printf( "Loaded music %s\n", name );
    }
    return music;
}

static Mix_Chunk* AST_LoadSample( const char *name ) {
    Mix_Chunk *chunk = Mix_LoadWAV( AST_GetAssetPath( name ) );
    if ( ! chunk ) {
        CON_Printf( "LoadSample: failed to load %s\n", name );
        chunk = ast_wavPlaceholder;
    } else {
        CON_Printf( "Loaded sample %s\n", name );
    }
    return chunk;
}

void AST_Init( void ) {
    ast_musPlaceholder = Mix_LoadMUS_RW( SDL_RWFromMem( placeholder_wav, placeholder_wav_len ), false );
    ast_musRain = AST_LoadMusic( "rain.ogg" );
    ast_wavPlaceholder = Mix_LoadWAV_RW( SDL_RWFromMem( placeholder_wav, placeholder_wav_len ), false );
    ast_wavThunder = AST_LoadSample( "thunder.ogg" );
    ast_wavGunshot = AST_LoadSample( "gunshot.ogg" );
    ast_wavCasingFallThick = AST_LoadSample( "casing_falling_thick.ogg" );
}

void AST_Done( void ) {
}
