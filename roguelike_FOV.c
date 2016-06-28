/*
   What this function does:
      Rasterizes a single Field Of View octant on a grid, similar to the way 
      FOV / shadowcasting is implemented in some roguelikes.
      Clips to bitmap

   What it DOES NOT do:
      Subpixel accuracy
      Antialiasing

   To rasterize the entire FOV, call this in a loop with octant in range 0-7
   Inspired by http://blogs.msdn.com/b/ericlippert/archive/2011/12/12/shadowcasting-in-c-part-one.aspx

   See the result here: 
      http://youtu.be/O8xxVQ1fcTE
      http://imgur.com/l2c2Mi4&Wyo46ps&DhkCQXD
      http://imgur.com/l2c2Mi4&Wyo46ps&DhkCQXD#1
      http://imgur.com/l2c2Mi4&Wyo46ps&DhkCQXD#2
*/

typedef struct {
    int x, y;
} c2_t;

static inline c2_t c2xy( int x, int y ) {
    c2_t c = { x, y };
    return c;
}

static inline c2_t c2Add( c2_t a, c2_t b ) {
    return c2xy( a.x + b.x, a.y + b.y );
}

static inline c2_t c2Sub( c2_t a, c2_t b ) {
    return c2xy( a.x - b.x, a.y - b.y );
}

static inline int c2Dot( c2_t a, c2_t b ) {
    return a.x * b.x + a.y * b.y;
}

static inline int c2CrossC( c2_t a, c2_t b ) {
    return a.x * b.y - a.y * b.x;
}

static inline int Maxi( int a, int b ) {
    return a > b ? a : b;
}

static inline int Mini( int a, int b ) {
    return a < b ? a : b;
}

static inline int Clampi( int v, int min, int max ) {
	return Maxi( min, Mini( v, max ) );
}

void RasterizeFOVOctant( int originX, int originY,
                         int radius, 
                         int bitmapWidth, int bitmapHeight,
                         int octant,
                         const unsigned char *inTerrain, 
                         unsigned char *outBitmap ) {

#define READ_PIXEL(c) inTerrain[(c).x+(c).y*bitmapWidth]
#define WRITE_PIXEL(c,color) outBitmap[(c).x+(c).y*bitmapWidth]=(color)
#define FALSE 0
#define TRUE 1
#define MAX_FRUST_RAYS 32

    c2_t origin = c2xy( originX, originY );
    c2_t bitmapSize = c2xy( bitmapWidth, bitmapHeight );

    // quit if on blocking tile
    if ( READ_PIXEL( origin ) ) {
        return;
    }

    origin.x = Clampi( origin.x, 0, bitmapSize.x - 1 );
    origin.y = Clampi( origin.y, 0, bitmapSize.y - 1 );

    // generate octant transform matrix
    int lx = octant & 1;
    int ly = ( octant + 1 ) & 1;
    int sx = 1 - ( ( octant & 4 ) >> 1 );
    int sy = 1 - ( ( ( octant + 2 ) & 4 ) >> 1 );
    c2_t e0 = c2xy( lx * sx, ly * sx );
    c2_t e1 = c2xy( ly * sy, lx * sy );

    // get clipping distances in local space by projecting minmax on the transform axes
    // apply bias when rasterizing to the "left"
    c2_t v = c2Sub( origin, bitmapSize );
    int dx0 = c2Dot( origin, e0 ); 
    int dx1 = c2Dot( v, e0 ); 
    int biasX = dx0 < 0;
    int dx = Maxi( dx0, dx1 ) - biasX;
    int dy0 = c2Dot( origin, e1 ); 
    int dy1 = c2Dot( v, e1 ); 
    int biasY = dy0 < 0;
    int dy = Maxi( dy0, dy1 ) - biasY;

    int maxX = Mini( radius, bitmapSize.x - dx );

    typedef struct {
        c2_t v;
        int w;
    } frustRay_t;

    // rays are ordered in pairs forming frustums
    // the initial frustum contains the entire octant
    int numFrust = 2;
    frustRay_t frust[MAX_FRUST_RAYS] = { 
        { c2xy( 1, 0 ), 0, },
        { c2xy( 1, 1 ), 0, },
    };

    int r2 = radius * radius;

    c2_t local;
    for ( local.x = 0; local.x < maxX; ) {

        // y moves down diagonally
        int maxY = Mini( local.x + 1, bitmapSize.y - dy );

        // keep the Y of first blocking pixel and use it to correct the lower frustum vector
        int entryY = maxY;

        for ( local.y = 0; local.y < maxY; ) {
            int d2 = c2Dot( local, local );

            if ( d2 <= r2 ) {

                // world space transform tile
                c2_t p = c2xy( e0.x * local.x + e1.x * local.y + origin.x,
                               e0.y * local.x + e1.y * local.y + origin.y );

                int isBlockingLight = READ_PIXEL( p );

                if ( isBlockingLight ) {

                    // obstructing tiles hit by frustum rays cause angle between rays to shrink by pushing them accordingly
                    // new frustum is created around tile if it is not penetrated by any rays and is contained in an existing frustum

                    const c2_t verts[2] = {
                        c2xy( local.x, local.y + 1 ),
                        c2xy( local.x + 1, local.y ),
                    };

                    if ( local.y < entryY ) {
                        entryY = local.y;
                    }

                    int hitByRay = FALSE;

                    for ( int i = 0; i < numFrust; i++ ) {
                        frustRay_t *fr = &frust[i];

                        // handle the case where ray (1,0) hits blocking tile (x,0) explicitly
                        if ( fr->v.y == 0 && local.y == 0 ) {
                            fr->v = c2xy( local.x, 1 );
                            fr->w = c2CrossC( fr->v, local );
                            continue;
                        }

                        // ignore x + 1, y + 1 because in this octant ray always comes from up left
                        int w0 = c2CrossC( fr->v, verts[0] );
                        int w1 = c2CrossC( fr->v, verts[1] );
                        int w2 = fr->w;

                        // if signs differ, the ray intersects the tile
                        if ( ( ( w0 ^ w1 ) | ( w0 ^ w2 ) ) < 0 ) {
                            // correct the ray

                            // top rays sink, bottom rays bubble up
                            if ( i & 1 ) {
                                fr->v = c2xy( local.x + 1, entryY );
                            } else {
                                fr->v = c2xy( local.x, local.y + 1 );
                            }

                            fr->w = c2CrossC( fr->v, local );

                            hitByRay = TRUE;
                        }
                    }

                    if ( ! hitByRay && numFrust < MAX_FRUST_RAYS ) {

                        // create new frustum by splitting the one that contains the tile

                        for ( int i = 0; i < numFrust; i += 2 ) {
                            frustRay_t *upper = &frust[i + 0];
                            frustRay_t *lower = &frust[i + 1];

                            int w0 =  upper->w;
                            int w1 = -lower->w;

                            // check if tile is on or "inside" both edges
                            if ( ( w0 | w1 ) >= 0 ) {

                                // insert frustum rays around light blocking tile(s)

                                frust[numFrust + 1] = *lower;

                                frustRay_t *fr1 = lower;
                                frustRay_t *fr2 = &frust[numFrust];

                                fr1->v = c2xy( local.x + 1, local.y );
                                fr2->v = c2xy( local.x, entryY + 1 );

                                numFrust += 2;

                                // ignore other frustums
                                break;
                            }
                        }
                    }
                } 

                else {

                    // non blocking tile

                    for ( int i = 0; i < numFrust; i += 2 ) {
                        frustRay_t *upper = &frust[i + 0];
                        frustRay_t *lower = &frust[i + 1];

                        int w0 =  upper->w;
                        int w1 = -lower->w;

                        // check if tile is on or "inside" both edges
                        if ( ( w0 | w1 ) >= 0 ) {

                            // light up the tile

                            // uncomment this for attenuation
                            //WRITE_PIXEL( p, 255 - d2 * 255 / r2 );

                            WRITE_PIXEL( p, 255 );

                            // ignore other frustums
                            break;
                        }
                    }

                    // reset blocking span of tiles 
                    entryY = maxY;
                }
            }

            local.y++;

            // no need to do cross product here, just interpolate
            for ( int i = 0; i < numFrust; i++ ) {
                frustRay_t *fr = &frust[i];
                fr->w += fr->v.x;
            }
        }

        local.x++;

        // this too can be replaced by an addition and kept in a wColumn member
        // but it leads to more bookkeeping inside frustum correction
        for ( int i = 0; i < numFrust; i++ ) {
            frustRay_t *fr = &frust[i];
            fr->w = c2CrossC( fr->v, c2xy( local.x, 0 ) );
        }
    }
}
