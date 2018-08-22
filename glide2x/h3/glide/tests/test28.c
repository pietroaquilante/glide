/*
** THIS SOFTWARE IS SUBJECT TO COPYRIGHT PROTECTION AND IS OFFERED ONLY
** PURSUANT TO THE 3DFX GLIDE GENERAL PUBLIC LICENSE. THERE IS NO RIGHT
** TO USE THE GLIDE TRADEMARK WITHOUT PRIOR WRITTEN PERMISSION OF 3DFX
** INTERACTIVE, INC. A COPY OF THIS LICENSE MAY BE OBTAINED FROM THE 
** DISTRIBUTOR OR BY CONTACTING 3DFX INTERACTIVE INC(info@3dfx.com). 
** THIS PROGRAM IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER 
** EXPRESSED OR IMPLIED. SEE THE 3DFX GLIDE GENERAL PUBLIC LICENSE FOR A
** FULL TEXT OF THE NON-WARRANTY PROVISIONS.  
** 
** USE, DUPLICATION OR DISCLOSURE BY THE GOVERNMENT IS SUBJECT TO
** RESTRICTIONS AS SET FORTH IN SUBDIVISION (C)(1)(II) OF THE RIGHTS IN
** TECHNICAL DATA AND COMPUTER SOFTWARE CLAUSE AT DFARS 252.227-7013,
** AND/OR IN SIMILAR OR SUCCESSOR CLAUSES IN THE FAR, DOD OR NASA FAR
** SUPPLEMENT. UNPUBLISHED RIGHTS RESERVED UNDER THE COPYRIGHT LAWS OF
** THE UNITED STATES.  
** 
** COPYRIGHT 3DFX INTERACTIVE, INC. 1999, ALL RIGHTS RESERVED
**
*/

#include <stdlib.h>
#include <stdio.h>
#ifndef __linux__
#include <conio.h>
#endif
#include <assert.h>

#include <glide.h>
#include "tlib.h"

#include <float.h>
#include <math.h>

#define PI 3.141592653589793
#define RAD_TO_DEG(t) ((t) * 180.0 / PI)
#define DEG_TO_RAD(t) ((t) * PI / 180.0)

typedef struct _RGB {
    float r, g, b;
} RGB, *RGBPtr;


#define FUDGE(x) ((x) * 255.0f)
#define PHASE(x, y, m) (((x) + (y)) % (m))

static float
value(float n1, float n2, float hue)
{
    float retval;

    if (hue > 360.0f) hue -= 360.0f;
    if (hue < 0.0f) hue += 360.0f;

    if (hue < 60.0f) {
        retval = n1 + (n2 - n1) * hue / 60.0f;
    } else if (hue < 180.0f) {
        retval = n2;
    } else if (hue < 240.0f) {
        retval = n1 + (n2 - 1.0f) * (240.0f - hue) / 60.0f;
    } else {
        retval = n1;
    }
//    assert((retval >= 0.0f) && (retval <= 1.0f));
    if (retval < 0.0f) retval = 0.0f;
    if (retval > 1.0f) retval = 1.0f;
    return FUDGE(retval);
}

static void
hlsToRGB(float h, float l, float s, RGBPtr color)
{
    float p1, p2;

    p2 = (l <= 0.5f) ? l * (1.0f + s) : l + s - l * s;
    p1 = 2 * l - p2;

    if (s == 0.0f) {
        color->r = color->g = color->b = l;
    } else {
        color->r = value(p1, p2, h+120.0f);
        color->g = value(p1, p2, h);
        color->b = value(p1, p2, h-120.0f);
    }

} /* end hlsToRGB() */


GrHwConfiguration hwconfig;
static char version[80];

static const char name[]    = "test28";
static const char purpose[] = "planar polygon test";
static const char usage[]   = "-n <frames> -r <res>";

int main( int argc, char **argv) {
    char match; 
    char **remArgs;
    int  rv;

    GrScreenResolution_t resolution = GR_RESOLUTION_640x480;
    float                scrWidth   = 640.0f;
    float                scrHeight  = 480.0f;
#define NVERT 5
    GrVertex vtx[NVERT];
    int index[NVERT];
    int frames                      = -1;
    int i, idx;
#define NHUE 360
    RGB hues[NHUE];
    
    /* Process Command Line Arguments */
    while ((rv = tlGetOpt(argc, argv, "nr", &match, &remArgs)) != 0) {
        if ( rv == -1 ) {
            printf( "Unrecognized command line argument\n" );
            printf( "%s %s\n", name, usage );
            printf( "Available resolutions:\n%s\n",
                    tlGetResolutionList() );
            return -1;
        }
        switch( match ) {
        case 'n':
            frames = atoi( remArgs[0] );
            break;
        case 'r':
            resolution = tlGetResolutionConstant( remArgs[0], 
                                                  &scrWidth, 
                                                  &scrHeight );
            break;
        }
    }

    tlSetScreen( scrWidth, scrHeight );

    grGlideGetVersion( version );

    printf( "%s:\n%s\n", name, purpose );
    printf( "%s\n", version );
    printf( "Resolution: %s\n", tlGetResolutionString( resolution ) );
    if ( frames == -1 ) {
        printf( "Press A Key To Begin Test.\n" );
        tlGetCH();
    }
    
    /* Initialize Glide */
    grGlideInit();
    assert( grSstQueryHardware( &hwconfig ) );
    grSstSelect( 0 );
    assert( grSstWinOpen( 0,
                      resolution,
                      GR_REFRESH_60Hz,
                      GR_COLORFORMAT_ABGR,
                      GR_ORIGIN_UPPER_LEFT,
                      2, 1 ) );
    
    tlConSet( 0.0f, 0.0f, 1.0f, 1.0f, 
              60, 30, 0xffffff );

    /* Set up Render State - gouraud shading */
    grColorCombine( GR_COMBINE_FUNCTION_LOCAL,
                    GR_COMBINE_FACTOR_NONE,
                    GR_COMBINE_LOCAL_ITERATED,
                    GR_COMBINE_OTHER_NONE,
                    FXFALSE );

//    grRenderBuffer(GR_BUFFER_FRONTBUFFER);    /* debug only */
    
    tlConOutput( "Press a key to quit\n" );

    /* init a table of hues */
    for (i=0; i<NHUE; i++) {
      const float theta = i * 360.0f / NHUE;
      hlsToRGB( theta, 0.4f, 0.5f, &hues[i]);
    }

    /* assign hues to vertices */
    for (i=0; i<NVERT; i++) {
        vtx[i].r = hues[PHASE(0, i*(NHUE / NVERT), NHUE)].r;
        vtx[i].g = hues[PHASE(0, i*(NHUE / NVERT), NHUE)].g;
        vtx[i].b = hues[PHASE(0, i*(NHUE / NVERT), NHUE)].b;
    }
#if 1
    /*
     * Force polygon RGB values to be planar... note overflow!
     * this is deliberate as a sanity check
     */
    vtx[3].r = 235.519f; vtx[3].g = 51.001f; vtx[3].b = 115.721f;
    vtx[4].r = 298.559f; vtx[4].g = -12.039f; vtx[4].b = 91.0f;
#endif

    while( frames-- && tlOkToRender()) {

        tlGetDimsByConst(resolution,
                         &scrWidth, 
                         &scrHeight );
        
        grClipWindow(0, 0, (FxU32) scrWidth, (FxU32) scrHeight);

        grBufferClear( 0x00, 0, GR_WDEPTHVALUE_FARTHEST );

	/* generate a equilateral polygon */
	for (i=0; i<NVERT; i++) {
	  double theta = 2.0 * PI * i / (double) NVERT;
	  
	  vtx[i].x = tlScaleX((float)((cos(theta) / 4.0) + 0.5));
	  vtx[i].y = tlScaleY((float)((sin(theta) / 4.0) + 0.5));
	  
	  index[i] = i;
	}

        idx = 30 /* (-frames) % NHUE */;

#if 1
        /* cyclical permutation: turn off to see just one set of triangles */
        for (i=0; i<NVERT; i++) {
            index[i] = (index[i] + 1) % NVERT;
        }
#endif

        grDrawPlanarPolygon(NVERT, index, vtx);

        tlConRender();
        grBufferSwap( 1 );
        if ( tlKbHit() ) frames = 0;
    }
    
    grGlideShutdown();
    return 0;
}
