#include <fcntl.h>
#include "bgg.h"
#include "assets.h"

#define WLD_MAX_ENTS             (32*1024)
#define WLD_SIDE_IN_TILES        (2*1024)
#define WLD_TILES_PER_CHUNK_SIDE 32
#define WLD_SIDE_IN_CHUNKS       (WLD_SIDE_IN_TILES / WLD_TILES_PER_CHUNK_SIDE)
#define WLD_MAX_ENTS_IN_CHUNK    (WLD_TILES_PER_CHUNK_SIDE * WLD_TILES_PER_CHUNK_SIDE)
#define WLD_MAX_ENTS_IN_TILE     4

// these will break the world file
#define WLD_TILE_MEM_SIZE        16
#define WLD_CHUNK_MEM_SIZE       (8*1024)
#define WLD_ENT_MEM_SIZE         1024

typedef union {
    struct {
        short ents[WLD_MAX_ENTS_IN_TILE];
        short image;
    };
    byte mem[WLD_TILE_MEM_SIZE];
} wldTile_t;

typedef union {
    struct {
        short ents[WLD_MAX_ENTS_IN_CHUNK];
        short terrain[WLD_TILES_PER_CHUNK_SIDE * WLD_TILES_PER_CHUNK_SIDE];
    };
    byte mem[WLD_CHUNK_MEM_SIZE];
} wldChunk_t;

typedef enum {
    CT_TRANSFORM,
    CT_RENDERER,
    CT_NUM_TYPES,
} cType_t;

typedef struct {
    int parent;
    v2_t down;
    v2_t right;
    v2_t position;
    v2_t localDown;
    v2_t localRight;
    v2_t localPosition;
} cTransform_t;

typedef union {
    struct {
        int components[CT_NUM_TYPES];
    };
    byte mem[WLD_ENT_MEM_SIZE];
} entity_t;

typedef union {
    struct {
        int version;
        bool_t bigEndian;
    };
    byte mem[1024];
} wldHeader_t;

typedef struct {
    wldHeader_t header;
    entity_t ents[WLD_MAX_ENTS];
    wldTile_t map[WLD_SIDE_IN_TILES * WLD_SIDE_IN_TILES];
    //wldChunk_t map[WLD_SIDE_IN_CHUNKS * WLD_SIDE_IN_CHUNKS];
} store_t;

void WLD_Init( void ) {
    STATIC_ASSERT( sizeof( wldTile_t ) <= WLD_TILE_MEM_SIZE, world_tile_size_should_be_less_or_equal_to_WLD_TILE_MEM_SIZE );
    //STATIC_ASSERT( sizeof( wldChunk_t ) <= WLD_CHUNK_MEM_SIZE, world_chunk_size_should_be_less_than_WLD_CHUNK_MEM_SIZE );
    //CON_Printf( "World chunk size: %.2fKb\n", sizeof( wldChunk_t ) / 1024.f );
    CON_Printf( "Total storage: %.1fMb\n", sizeof( store_t ) / ( float )( 1024 * 1024 ) );
    //int o = open(map_file_name, O_TRUNC | O_BINARY | O_RDWR | O_CREAT, mode);
    //int fd = open("kor", O_BINARY | O_RDWR | O_CREAT | O_TRUNC, (mode_t)0600);
}

//=============================================================================

/* mmap() replacement for Windows
 *
 * Author: Mike Frysinger <vapier@gentoo.org>
 * Placed into the public domain
 */

/* References:
 * CreateFileMapping: http://msdn.microsoft.com/en-us/library/aa366537(VS.85).aspx
 * CloseHandle:       http://msdn.microsoft.com/en-us/library/ms724211(VS.85).aspx
 * MapViewOfFile:     http://msdn.microsoft.com/en-us/library/aa366761(VS.85).aspx
 * UnmapViewOfFile:   http://msdn.microsoft.com/en-us/library/aa366882(VS.85).aspx
 */

//#include <io.h>
//#include <windows.h>
//#include <sys/types.h>
//
//#define PROT_READ     0x1
//#define PROT_WRITE    0x2
///* This flag is only available in WinXP+ */
//#ifdef FILE_MAP_EXECUTE
//#define PROT_EXEC     0x4
//#else
//#define PROT_EXEC        0x0
//#define FILE_MAP_EXECUTE 0
//#endif
//
//#define MAP_SHARED    0x01
//#define MAP_PRIVATE   0x02
//#define MAP_ANONYMOUS 0x20
//#define MAP_ANON      MAP_ANONYMOUS
//#define MAP_FAILED    ((void *) -1)
//
//#ifdef __USE_FILE_OFFSET64
//# define DWORD_HI(x) (x >> 32)
//# define DWORD_LO(x) ((x) & 0xffffffff)
//#else
//# define DWORD_HI(x) (0)
//# define DWORD_LO(x) (x)
//#endif
//
//static void *mmap(void *start, size_t length, int prot, int flags, int fd, off_t offset)
//{
//	if (prot & ~(PROT_READ | PROT_WRITE | PROT_EXEC))
//		return MAP_FAILED;
//	if (fd == -1) {
//		if (!(flags & MAP_ANON) || offset)
//			return MAP_FAILED;
//	} else if (flags & MAP_ANON)
//		return MAP_FAILED;
//
//	DWORD flProtect;
//	if (prot & PROT_WRITE) {
//		if (prot & PROT_EXEC)
//			flProtect = PAGE_EXECUTE_READWRITE;
//		else
//			flProtect = PAGE_READWRITE;
//	} else if (prot & PROT_EXEC) {
//		if (prot & PROT_READ)
//			flProtect = PAGE_EXECUTE_READ;
//		else if (prot & PROT_EXEC)
//			flProtect = PAGE_EXECUTE;
//	} else
//		flProtect = PAGE_READONLY;
//
//	off_t end = length + offset;
//	HANDLE mmap_fd, h;
//	if (fd == -1)
//		mmap_fd = INVALID_HANDLE_VALUE;
//	else
//		mmap_fd = (HANDLE)_get_osfhandle(fd);
//	h = CreateFileMapping(mmap_fd, NULL, flProtect, DWORD_HI(end), DWORD_LO(end), NULL);
//	if (h == NULL)
//		return MAP_FAILED;
//
//	DWORD dwDesiredAccess;
//	if (prot & PROT_WRITE)
//		dwDesiredAccess = FILE_MAP_WRITE;
//	else
//		dwDesiredAccess = FILE_MAP_READ;
//	if (prot & PROT_EXEC)
//		dwDesiredAccess |= FILE_MAP_EXECUTE;
//	if (flags & MAP_PRIVATE)
//		dwDesiredAccess |= FILE_MAP_COPY;
//	void *ret = MapViewOfFile(h, dwDesiredAccess, DWORD_HI(offset), DWORD_LO(offset), length);
//	if (ret == NULL) {
//		CloseHandle(h);
//		ret = MAP_FAILED;
//	}
//	return ret;
//}
//
//static void munmap(void *addr, size_t length)
//{
//	UnmapViewOfFile(addr);
//	/* ruh-ro, we leaked handle from CreateFileMapping() ... */
//}
//
//#undef DWORD_HI
//#undef DWORD_LO
