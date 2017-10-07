/*
   What this function does:
      Rasterizes a single Field Of View octant on a grid, similar to the way 
      FOV / shadowcasting is implemented in some roguelikes.
      Clips to bitmap
      Steps on pixel centers
      Optional attenuation
      Optional circle clip
      Optional lit blocking tiles

   To rasterize the entire FOV, call this in a loop with octant in range 0-7
   Inspired by http://blogs.msdn.com/b/ericlippert/archive/2011/12/12/shadowcasting-in-c-part-one.aspx
*/

#include "zhost.h"

void RasterizeFOVOctant( int originX, int originY,
                         int radius, 
                         int bitmapWidth, int bitmapHeight,
                         int octant,
                         int skipAttenuation,
                         int skipClampToRadius,
                         int darkWalls,
                         const unsigned char *inBitmap, 
                         unsigned char *outBitmap ) {
#define READ_PIXEL(c) inBitmap[(c).x+(c).y*bitmapWidth]
#define WRITE_PIXEL(c,color) outBitmap[(c).x+(c).y*bitmapWidth]=(color)
#define MAX_RAYS 64
#define ADD_RAY(c) {nextRays->rays[Mini(nextRays->numRays,MAX_RAYS-1)] = (c);nextRays->numRays++;}
#define IS_ON_MAP(c) ((c).x >= 0 && (c).x < bitmapWidth && (c).y >= 0 && (c).y < bitmapHeight)
    typedef struct {
        int numRays;
        c2_t rays[MAX_RAYS];
    } raysList_t;
    // keep these coupled like this
    static const c2_t bases[] = {
        { { 1, 0  } }, { { 0, 1  } },
        { { 1, 0  } }, { { 0, -1 } },
        { { -1, 0 } }, { { 0, -1 } },
        { { -1, 0 } }, { { 0, 1  } },
        { { 0, 1  } }, { { -1, 0 } },
        { { 0, 1  } }, { { 1, 0  } },
        { { 0, -1 } }, { { 1, 0  } },
        { { 0, -1 } }, { { -1, 0 } },
    }; 
    c2_t e0 = bases[( octant * 2 + 0 ) & 15];
    c2_t e1 = bases[( octant * 2 + 1 ) & 15];
    raysList_t rayLists[2] = { {
        .numRays = 2,
        .rays = {
            c2xy( 1, 0 ),
            c2xy( 1, 1 ),
        }, 
    } };
    c2_t bitmapSize = c2xy( bitmapWidth, bitmapHeight );
    c2_t bitmapMax = c2Sub( bitmapSize, c2one );
    c2_t origin = c2Clamp( c2xy( originX, originY ), c2zero, bitmapMax );
    if ( READ_PIXEL( origin ) ) {
        WRITE_PIXEL( origin, 255 );
        return;
    }
    c2_t dmin = c2Neg( origin );
    c2_t dmax = c2Sub( bitmapMax, origin );
    int dmin0 = c2Dot( dmin, e0 );
    int dmax0 = c2Dot( dmax, e0 );
    int limit0 = Mini( radius, dmin0 > 0 ? dmin0 : dmax0 );
    int dmin1 = c2Dot( dmin, e1 );
    int dmax1 = c2Dot( dmax, e1 );
    int limit1 = Mini( radius, dmin1 > 0 ? dmin1 : dmax1 );
    c2_t ci = origin;
    for ( int i = 0; i <= limit0; i++ ) {
        int i2 = i * 2;
        raysList_t *currRays = &rayLists[( i + 0 ) & 1];
        raysList_t *nextRays = &rayLists[( i + 1 ) & 1];
        nextRays->numRays = 0;
        for ( int r = 0; r < currRays->numRays - 1; r += 2 ) {
            c2_t r0 = currRays->rays[r + 0];
            c2_t r1 = currRays->rays[r + 1];
            int inyr0 = ( i2 - 1 ) * r0.y / r0.x;
            int outyr0 = ( i2 + 1 ) * r0.y / r0.x;
            int inyr1 = ( i2 - 1 ) * r1.y / r1.x;
            int outyr1 = ( i2 + 1 ) * r1.y / r1.x;

            // every pixel with a center INSIDE the frustum is lit

            int starty = outyr0 + 1;
            if ( c2CrossC( r0, c2xy( i2, outyr0 ) ) < 0 ) {
                starty++;
            }
            starty /= 2;
            c2_t start = c2Add( ci, c2Scale( e1, starty ) );
            int endy = inyr1 + 1;
            if ( c2CrossC( r1, c2xy( i2, inyr1 + 1 ) ) > 0 ) {
                endy--;
            }
            endy /= 2;
            //c2_t end = c2Add( ci, c2Scale( e1, endy ) );
            {
                int y;
                c2_t p;
                int miny = starty;
                int maxy = Mini( endy, limit1 ); 
                for ( y = miny, p = start; y <= maxy; y++, p = c2Add( p, e1 ) ) {
                    WRITE_PIXEL( p, 255 );
                }
            }

            // push rays for the next column

            // correct the bounds first

            c2_t bounds0;
            c2_t bounds1;
            c2_t firstin = c2Add( ci, c2Scale( e1, ( inyr0 + 1 ) / 2 ) );
            c2_t firstout = c2Add( ci, c2Scale( e1, ( outyr0 + 1 ) / 2 ) );
            if ( ( IS_ON_MAP( firstin ) && ! READ_PIXEL( firstin ) )
                && ( IS_ON_MAP( firstout ) && ! READ_PIXEL( firstout ) ) ) {
                  bounds0 = r0;
            } else {
                int top = ( outyr0 + 1 ) / 2;
                int bottom = Mini( ( inyr1 + 1 ) / 2, limit1 );
                int y;
                c2_t p = c2Add( ci, c2Scale( e1, top ) );
                for ( y = top * 2; y <= bottom * 2; y += 2, p = c2Add( p, e1 ) ) {
                    if ( ! READ_PIXEL( p ) ) {
                        break;
                    }
                    // pixels that force ray corrections are lit too
                    WRITE_PIXEL( p, 255 );
                }
                bounds0 = c2xy( i2 - 1, y - 1 );
                inyr0 = ( i2 - 1 ) * bounds0.y / bounds0.x;
                outyr0 = ( i2 + 1 ) * bounds0.y / bounds0.x;
            }
            c2_t lastin = c2Add( ci, c2Scale( e1, ( inyr1 + 1 ) / 2 ) );
            c2_t lastout = c2Add( ci, c2Scale( e1, ( outyr1 + 1 ) / 2 ) );
            if ( ( IS_ON_MAP( lastin ) && ! READ_PIXEL( lastin ) )
                && ( IS_ON_MAP( lastout ) && ! READ_PIXEL( lastout ) ) ) {
                bounds1 = r1;
            } else {
                int top = ( outyr0 + 1 ) / 2;
                int bottom = Mini( ( inyr1 + 1 ) / 2, limit1 );
                int y;
                c2_t p = c2Add( ci, c2Scale( e1, bottom ) );
                for ( y = bottom * 2; y >= top * 2; y -= 2, p = c2Sub( p, e1 ) ) {
                    if ( ! READ_PIXEL( p ) ) {
                        break;
                    }
                    // pixels that force ray corrections are lit too
                    WRITE_PIXEL( p, 255 );
                }
                bounds1 = c2xy( i2 + 1, y + 1 );
                inyr1 = ( i2 - 1 ) * bounds1.y / bounds1.x;
                outyr1 = ( i2 + 1 ) * bounds1.y / bounds1.x;
            }

            // closed frustum - quit
            if ( c2CrossC( bounds0, bounds1 ) <= 0 ) {
                continue;
            }

            // push actual rays
            {
                ADD_RAY( bounds0 );
                int top = ( outyr0 + 1 ) / 2;
                int bottom = Mini( ( inyr1 + 1 ) / 2, limit1 );
                c2_t p = c2Add( ci, c2Scale( e1, top ) );
                int prevPixel = READ_PIXEL( p );
                for ( int y = top * 2; y <= bottom * 2; y += 2, p = c2Add( p, e1 ) ) {
                    int pixel = READ_PIXEL( p );
                    if ( prevPixel != pixel ) {
                        c2_t ray;
                        if ( pixel ) {
                            ray = c2xy( i2 + 1, y - 1 );
                        } else {
                            ray = c2xy( i2 - 1, y - 1 );
                        }
                        ADD_RAY( ray );
                    }
                    prevPixel = pixel;
                }
                ADD_RAY( bounds1 );
            }
        }
        ci = c2Add( ci, e0 );
    }

    if ( ! skipAttenuation ) {
        c2_t ci = origin;
        int rsq = radius * radius;
        for ( int i = 0; i <= limit0; i++ ) {
            c2_t p = ci;
            for ( int j = 0; j <= limit1; j++ ) {
                c2_t d = c2Sub( p, origin );
                int dsq = c2Dot( d, d );
                int mod = 255 - Mini( dsq * 255 / rsq, 255 );
                int lit = !! outBitmap[p.x + p.y * bitmapWidth];
                WRITE_PIXEL( p, mod * lit );
                p = c2Add( p, e1 );
            }
            ci = c2Add( ci, e0 );
        }
    } else if ( ! skipClampToRadius ) {
        c2_t ci = origin;
        int rsq = radius * radius;
        for ( int i = 0; i <= limit0; i++ ) {
            c2_t p = ci;
            for ( int j = 0; j <= limit1; j++ ) {
                c2_t d = c2Sub( p, origin );
                if ( c2Dot( d, d ) > rsq ) { 
                    WRITE_PIXEL( p, 0 );
                }
                p = c2Add( p, e1 );
            }
            ci = c2Add( ci, e0 );
        }
    }

    if ( darkWalls ) {
        c2_t ci = origin;
        for ( int i = 0; i <= limit0; i++ ) {
            c2_t p = ci;
            for ( int j = 0; j <= limit1; j++ ) {
                if ( READ_PIXEL( p ) ) { 
                    WRITE_PIXEL( p, 0 );
                }
                p = c2Add( p, e1 );
            }
            ci = c2Add( ci, e0 );
        }
    } 
}
