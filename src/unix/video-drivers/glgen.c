/***************************************************************** 

  Generic OpenGL routines

  Copyright 1998 by Mike Oliphant - oliphant@ling.ed.ac.uk

    http://www.ling.ed.ac.uk/~oliphant/glmame

  Improved by Sven Goethel, http://www.jausoft.com, sgoethel@jausoft.com

  This code may be used and distributed under the terms of the
  Mame license

*****************************************************************/

#include <math.h>
#include "glmame.h"
#include <GL/glext.h>
#include "sysdep/sysdep_display_priv.h"
#include "x11.h"

/* private functions */
static GLdouble CompareVec (GLdouble i, GLdouble j, GLdouble k,
	    GLdouble x, GLdouble y, GLdouble z);
static void TranslatePointInPlane   (
	      GLdouble vx_p1, GLdouble vy_p1, GLdouble vz_p1,
	      GLdouble vx_nw, GLdouble vy_nw, GLdouble vz_nw,
	      GLdouble vx_nh, GLdouble vy_nh, GLdouble vz_nh,
	      GLdouble x_off, GLdouble y_off,
	      GLdouble *vx_p, GLdouble *vy_p, GLdouble *vz_p );
static void ScaleThisVec (GLdouble i, GLdouble j, GLdouble k,
	      GLdouble * x, GLdouble * y, GLdouble * z);
static void AddToThisVec (GLdouble i, GLdouble j, GLdouble k,
	      GLdouble * x, GLdouble * y, GLdouble * z);
static GLdouble LengthOfVec (GLdouble x, GLdouble y, GLdouble z);
static void NormThisVec (GLdouble * x, GLdouble * y, GLdouble * z);
static void DeltaVec (GLdouble x1, GLdouble y1, GLdouble z1,
	       GLdouble x2, GLdouble y2, GLdouble z2,
	       GLdouble * dx, GLdouble * dy, GLdouble * dz);
static void CrossVec (GLdouble a1, GLdouble a2, GLdouble a3,
	  GLdouble b1, GLdouble b2, GLdouble b3,
	  GLdouble * c1, GLdouble * c2, GLdouble * c3);
static void CopyVec(GLdouble *ax,GLdouble *ay,GLdouble *az,            /* dest   */
	     const GLdouble bx,const GLdouble by,const GLdouble bz     /* source */
                  );
static void CalcFlatTexPoint( int x, int y, GLdouble texwpervw, GLdouble texhpervh, 
		       GLdouble * px, GLdouble * py);
static int SetupFrustum (void);
static int SetupOrtho (void);

static void WAvg (GLdouble perc, GLdouble x1, GLdouble y1, GLdouble z1,
	   GLdouble x2, GLdouble y2, GLdouble z2,
	   GLdouble * ax, GLdouble * ay, GLdouble * az);

static GLint   gl_internal_format;
static GLenum  gl_bitmap_format;
static GLenum  gl_bitmap_type;
static GLsizei text_width;
static GLsizei text_height;
static int texnumx;
static int texnumy;

/* The squares that are tiled to make up the game screen polygon */
struct TexSquare
{
  GLubyte *texture;
  GLuint texobj;
  GLdouble x1, y1, z1, x2, y2, z2, x3, y3, z3, x4, y4, z4;
  GLdouble fx1, fy1, fx2, fy2, fx3, fy3, fx4, fy4;
  GLdouble xcov, ycov;
};

static struct TexSquare *texgrid = NULL;

/**
 * cscr..: cabinet screen points:
 * 
 * are defined within <model>.cab in the following order:
 *	1.) left  - top
 *	2.) right - top
 *	3.) right - bottom
 *	4.) left  - bottom
 *
 * are read in reversed:
 *	1.) left  - bottom
 *	2.) right - bottom
 *	3.) right - top
 *	4.) left  - top
 *
 * so we do have a positiv (Q I) coord system ;-), of course:
 *	left   < right
 *	bottom < top
 */
GLdouble vx_cscr_p1, vy_cscr_p1, vz_cscr_p1, 
        vx_cscr_p2, vy_cscr_p2, vz_cscr_p2,
        vx_cscr_p3, vy_cscr_p3, vz_cscr_p3, 
	vx_cscr_p4, vy_cscr_p4, vz_cscr_p4;

/**
 * cscr..: cabinet screen dimension vectors
 *	
 * 	these are the cabinet-screen's width/height in the cabinet's world-coord !!
 */
static GLdouble  vx_cscr_dw, vy_cscr_dw, vz_cscr_dw; /* the width (world-coord) , p1-p2 */
static GLdouble  vx_cscr_dh, vy_cscr_dh, vz_cscr_dh; /* the height (world-coord), p1-p4 */

static GLdouble  s__cscr_w,  s__cscr_h;              /* scalar width/height (world-coord) */
GLdouble  s__cscr_w_view, s__cscr_h_view;     /* scalar width/height (view-coord) */

/* screen x-axis (world-coord), normalized v__cscr_dw */
static GLdouble vx_scr_nx, vy_scr_nx, vz_scr_nx; 

/* screen y-axis (world-coord), normalized v__cscr_dh */
static GLdouble vx_scr_ny, vy_scr_ny, vz_scr_ny; 

/* screen z-axis (world-coord), the normalized cross product of v__cscr_dw,v__cscr_dh */
static GLdouble vx_scr_nz, vy_scr_nz, vz_scr_nz; 

/* x/y-factor for view/world coord translation */
static GLdouble cab_vpw_fx; /* s__cscr_w_view / s__cscr_w */
static GLdouble cab_vpw_fy; /* s__cscr_h_view / s__cscr_h */

/**
 * gscr..: game screen dimension vectors
 *	
 * 	these are the game portion of the cabinet-screen's width/height 
 *      in the cabinet's world-coord !!
 *
 *	gscr[wh]d[xyz] <= cscr[wh]d[xyz]
 */

static GLdouble vx_gscr_dw, vy_gscr_dw, vz_gscr_dw; /* the width (world-coord) */
static GLdouble vx_gscr_dh, vy_gscr_dh, vz_gscr_dh; /* the height (world-coord) */

static GLdouble  s__gscr_w, s__gscr_h;              /* scalar width/height (world-coord) */
static GLdouble  s__gscr_w_view, s__gscr_h_view;    /* scalar width/height (view-coord) */

static GLdouble  s__gscr_offx, s__gscr_offy;          /* delta game start (world-coord) */

static GLdouble  s__gscr_offx_view, s__gscr_offy_view;/* delta game-start (view-coord) */

/**
*
* ALL GAME SCREEN VECTORS ARE IN FINAL ORIENTATION (e.g. if (sysdep_display_params.orientation & SYSDEP_DISPLAY_SWAPXY))
*
* v__gscr_p1 = v__cscr_p1   + 
*              s__gscr_offx * v__scr_nx + s__gscr_offy * v__scr_ny ;
*
* v__gscr_p2  = v__gscr_p1  + 
*              s__gscr_w    * v__scr_nx + 0.0          * v__scr_ny ;
*
* v__gscr_p3  = v__gscr_p2  + 
*              0.0          * v__scr_nx + s__gscr_h    * v__scr_ny ;
*
* v__gscr_p4  = v__gscr_p3  - 
*              s__gscr_w    * v__scr_nx - 0.0          * v__scr_ny ;
*
* v__gscr_p4b = v__gscr_p1  + 
*              0.0          * v__scr_nx + s__gscr_h    * v__scr_ny ;
*
* v__gscr_p4a == v__gscr_p4b
*/
static GLdouble vx_gscr_p1, vy_gscr_p1, vz_gscr_p1; 
static GLdouble vx_gscr_p2, vy_gscr_p2, vz_gscr_p2; 
static GLdouble vx_gscr_p3, vy_gscr_p3, vz_gscr_p3; 
static GLdouble vx_gscr_p4, vy_gscr_p4, vz_gscr_p4; 

static GLdouble mxModel[16];
static GLdouble mxProjection[16];

/* Camera panning variables */
static int currentpan;
static int lastpan;
static int panframe;

/* Misc variables */
static unsigned short *colorBlittedMemory = NULL;
static int cab_loaded = 0;
static int texture_init = 0;
static unsigned char *empty_text = NULL;

/* Vector variables */
GLuint veclist=0;
GLdouble vecx, vecy, vecscalex, vecscaley;

#define RETURN_IF_GL_ERROR() \
      if((err = disp__glGetError ())) \
      { \
        print_gl_error("GLEROR", __FILE__, __LINE__, err); \
        return 1; \
      }

/* ---------------------------------------------------------------------- */
/* ------------ New OpenGL Specials ------------------------------------- */
/* ---------------------------------------------------------------------- */
static int gl_set_bilinear (int new_value)
{
  int x, y;
  GLenum err;
  bilinear = new_value;
  if (bilinear)
  {
    if (cabtex)
    {
	    disp__glBindTexture (GL_TEXTURE_2D, cabtex[0]);
	    RETURN_IF_GL_ERROR ();
	    disp__glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	    RETURN_IF_GL_ERROR ();
	    disp__glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	    RETURN_IF_GL_ERROR ();
    }

    for (y = 0; y < texnumy; y++)
    {
      for (x = 0; x < texnumx; x++)
      {
        disp__glBindTexture (GL_TEXTURE_2D, texgrid[y * texnumx + x].texobj);
	RETURN_IF_GL_ERROR ();
        disp__glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        RETURN_IF_GL_ERROR ();
        disp__glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        RETURN_IF_GL_ERROR ();
      }
    }
  }
  else
  {
    if (cabtex)
    {
	    disp__glBindTexture (GL_TEXTURE_2D, cabtex[0]);
	    RETURN_IF_GL_ERROR ();
	    disp__glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	    RETURN_IF_GL_ERROR ();
	    disp__glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	    RETURN_IF_GL_ERROR ();
    }

    for (y = 0; y < texnumy; y++)
    {
      for (x = 0; x < texnumx; x++)
      {
        disp__glBindTexture (GL_TEXTURE_2D, texgrid[y * texnumx + x].texobj);
        RETURN_IF_GL_ERROR ();
        disp__glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        RETURN_IF_GL_ERROR ();
        disp__glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        RETURN_IF_GL_ERROR ();
      }
    }
  }
  return 0;
}

static int gl_set_cabview (int new_value)
{
  cabview = new_value;
  if (cabview)
  {
        currentpan = 1;
	lastpan    = 0;
	panframe   = 0;
	return SetupFrustum ();
  } else {
	return SetupOrtho ();
  }
}

static int gl_set_antialias(int new_value)
{
  GLenum err;
  antialias = new_value;
  if (antialias)
  {
    disp__glShadeModel (GL_SMOOTH);
    RETURN_IF_GL_ERROR ();
    disp__glEnable (GL_POLYGON_SMOOTH);
    RETURN_IF_GL_ERROR ();
    disp__glEnable (GL_LINE_SMOOTH);
    RETURN_IF_GL_ERROR ();
    disp__glEnable (GL_POINT_SMOOTH);
    RETURN_IF_GL_ERROR ();
  }
  else
  {
    disp__glShadeModel (GL_FLAT);
    RETURN_IF_GL_ERROR ();
    disp__glDisable (GL_POLYGON_SMOOTH);
    RETURN_IF_GL_ERROR ();
    disp__glDisable (GL_LINE_SMOOTH);
    RETURN_IF_GL_ERROR ();
    disp__glDisable (GL_POINT_SMOOTH);
    RETURN_IF_GL_ERROR ();
  }
  return 0;
}

static int gl_set_beam(float new_value)
{
	GLenum err;
	gl_beam = new_value;
	disp__glLineWidth(gl_beam);
        RETURN_IF_GL_ERROR ();
	disp__glPointSize(gl_beam);
        RETURN_IF_GL_ERROR ();
	printf("GLINFO (vec): beamer size %f\n", gl_beam);
	return 0;
}

int gl_open_display (void)
{
  const unsigned char * glVersion;
  double game_aspect ;
  double cabn_aspect ;
  int x, y;
  struct TexSquare *tsq;
  GLenum err;
  GLdouble vx_gscr_p4b, vy_gscr_p4b, vz_gscr_p4b; 
  GLdouble t1, texwpervw, texhpervh;
  GLint format=0;
  GLint tidxsize=0;
  int format_ok=0;
  int bytes_per_pixel = (sysdep_display_params.depth + 7) / 8;

  glVersion = disp__glGetString(GL_VERSION);
  RETURN_IF_GL_ERROR ();

  printf("\nGLINFO: OpenGL Driver Information:\n");
  printf("\tvendor: %s,\n\trenderer %s,\n\tversion %s\n", 
  	disp__glGetString(GL_VENDOR), 
	disp__glGetString(GL_RENDERER),
	glVersion);

  if(!(glVersion[0]>'1' ||
       (glVersion[0]=='1' && glVersion[2]>='2') ) )
  {
       printf("error: an OpenGL >= 1.2 capable driver is required!\n");
       return 1;
  }

  printf("GLINFO: GLU Driver Information:\n");
  printf("\tversion %s\n",
        disp__gluGetString(GLU_VERSION));
  
  cab_loaded = LoadCabinet (cabname);
  if (cabview && !cab_loaded)
  {
    fprintf(stderr, "GLERROR: Unable to load cabinet %s\n", cabname);
    cabview = 0;
  }

  disp__glClearColor (0, 0, 0, 1);
  RETURN_IF_GL_ERROR ();
  disp__glClear (GL_COLOR_BUFFER_BIT);
  RETURN_IF_GL_ERROR ();
  disp__glFlush ();
  RETURN_IF_GL_ERROR ();

  if (gl_resize())
    return 1;

  disp__glDepthFunc (GL_LEQUAL);
  RETURN_IF_GL_ERROR ();

  if (cab_loaded)
  {
    /* Calulate delta vectors for screen height and width */
    DeltaVec (vx_cscr_p1, vy_cscr_p1, vz_cscr_p1, vx_cscr_p2, vy_cscr_p2, vz_cscr_p2,
              &vx_cscr_dw, &vy_cscr_dw, &vz_cscr_dw);
    DeltaVec (vx_cscr_p1, vy_cscr_p1, vz_cscr_p1, vx_cscr_p4, vy_cscr_p4, vz_cscr_p4,
              &vx_cscr_dh, &vy_cscr_dh, &vz_cscr_dh);

    s__cscr_w = LengthOfVec (vx_cscr_dw, vy_cscr_dw, vz_cscr_dw);
    s__cscr_h = LengthOfVec (vx_cscr_dh, vy_cscr_dh, vz_cscr_dh);


            /*	  
            ScaleThisVec ( -1.0,  1.0,  1.0, &vx_cscr_dh, &vy_cscr_dh, &vz_cscr_dh);
            ScaleThisVec ( -1.0,  1.0,  1.0, &vx_cscr_dw, &vy_cscr_dw, &vz_cscr_dw);
            */

    CopyVec( &vx_scr_nx, &vy_scr_nx, &vz_scr_nx,
              vx_cscr_dw,  vy_cscr_dw,  vz_cscr_dw);
    NormThisVec (&vx_scr_nx, &vy_scr_nx, &vz_scr_nx);

    CopyVec( &vx_scr_ny, &vy_scr_ny, &vz_scr_ny,
              vx_cscr_dh,  vy_cscr_dh,  vz_cscr_dh);
    NormThisVec (&vx_scr_ny, &vy_scr_ny, &vz_scr_ny);

    CrossVec (vx_cscr_dw, vy_cscr_dw, vz_cscr_dw,
              vx_cscr_dh, vy_cscr_dh, vz_cscr_dh,
              &vx_scr_nz, &vy_scr_nz, &vz_scr_nz);
    NormThisVec (&vx_scr_nz, &vy_scr_nz, &vz_scr_nz);

#ifndef NDEBUG
    /**
     * assertions ...
     */
    CopyVec( &t1x, &t1y, &t1z,
             vx_scr_nx, vy_scr_nx, vz_scr_nx);
    ScaleThisVec (s__cscr_w,s__cscr_w,s__cscr_w,
                  &t1x, &t1y, &t1z);
    t1 =  CompareVec (t1x, t1y, t1z, vx_cscr_dw,  vy_cscr_dw,  vz_cscr_dw);

    printf("GLINFO: test v__cscr_dw - ( v__scr_nx * s__cscr_w ) = %f\n", t1);
    printf("\t v__cscr_dw = %f / %f / %f\n", vx_cscr_dw,  vy_cscr_dw,  vz_cscr_dw);
    printf("\t v__scr_nx = %f / %f / %f\n", vx_scr_nx, vy_scr_nx, vz_scr_nx);
    printf("\t s__cscr_w  = %f \n", s__cscr_w);

    CopyVec( &t1x, &t1y, &t1z,
             vx_scr_ny, vy_scr_ny, vz_scr_ny);
    ScaleThisVec (s__cscr_h,s__cscr_h,s__cscr_h,
                  &t1x, &t1y, &t1z);
    t1 =  CompareVec (t1x, t1y, t1z, vx_cscr_dh,  vy_cscr_dh,  vz_cscr_dh);

    printf("GLINFO: test v__cscr_dh - ( v__scr_ny * s__cscr_h ) = %f\n", t1);
    printf("\t v__cscr_dh = %f / %f / %f\n", vx_cscr_dh,  vy_cscr_dh,  vz_cscr_dh);
    printf("\t v__scr_ny  = %f / %f / %f\n", vx_scr_ny, vy_scr_ny, vz_scr_ny);
    printf("\t s__cscr_h   = %f \n", s__cscr_h);
#endif

    /**
     *
     * Cabinet-Screen Ratio:
     */
     game_aspect = (double)sysdep_display_params.width / (double)sysdep_display_params.height;

     cabn_aspect = (double)s__cscr_w        / (double)s__cscr_h;

     if( game_aspect <= cabn_aspect )
     {
          /**
           * cabinet_width  >  game_width
           * cabinet_height == game_height 
           *
           * cabinet_height(view-coord) := sysdep_display_params.height(view-coord) 
           */
          s__cscr_h_view  = (GLdouble) sysdep_display_params.height;
          s__cscr_w_view  = s__cscr_h_view * cabn_aspect;

          s__gscr_h_view  = s__cscr_h_view;
          s__gscr_w_view  = s__gscr_h_view * game_aspect; 

          s__gscr_offy_view = 0.0;
          s__gscr_offx_view = ( s__cscr_w_view - s__gscr_w_view ) / 2.0;

     } else {
          /**
           * cabinet_width  <  game_width
           * cabinet_width  == game_width 
           *
           * cabinet_width(view-coord) := sysdep_display_params.width(view-coord) 
           */
          s__cscr_w_view  = (GLdouble) sysdep_display_params.width;
          s__cscr_h_view  = s__cscr_w_view / cabn_aspect;

          s__gscr_w_view  = s__cscr_w_view;
          s__gscr_h_view  = s__gscr_w_view / game_aspect; 

          s__gscr_offx_view = 0.0;
          s__gscr_offy_view = ( s__cscr_h_view - s__gscr_h_view ) / 2.0;

     }
     cab_vpw_fx = (GLdouble)s__cscr_w_view / (GLdouble)s__cscr_w ;
     cab_vpw_fy = (GLdouble)s__cscr_h_view / (GLdouble)s__cscr_h ;

     s__gscr_w   = (GLdouble)s__gscr_w_view  / cab_vpw_fx ;
     s__gscr_h   = (GLdouble)s__gscr_h_view  / cab_vpw_fy ;
     s__gscr_offx = (GLdouble)s__gscr_offx_view / cab_vpw_fx ;
     s__gscr_offy = (GLdouble)s__gscr_offy_view / cab_vpw_fy ;


     /**
      * ALL GAME SCREEN VECTORS ARE IN FINAL ORIENTATION (e.g. if (sysdep_display_params.orientation & SYSDEP_DISPLAY_SWAPXY))
      *
      * v__gscr_p1 = v__cscr_p1   + 
      *              s__gscr_offx * v__scr_nx + s__gscr_offy * v__scr_ny ;
      *
      * v__gscr_p2  = v__gscr_p1  + 
      *              s__gscr_w    * v__scr_nx + 0.0          * v__scr_ny ;
      *
      * v__gscr_p3  = v__gscr_p2  + 
      *              0.0          * v__scr_nx + s__gscr_h    * v__scr_ny ;
      *
      * v__gscr_p4  = v__gscr_p3  - 
      *              s__gscr_w    * v__scr_nx - 0.0          * v__scr_ny ;
      *
      * v__gscr_p4b = v__gscr_p1  + 
      *              0.0          * v__scr_nx + s__gscr_h    * v__scr_ny ;
      *
      * v__gscr_p4  == v__gscr_p4b
      */
     TranslatePointInPlane ( vx_cscr_p1, vy_cscr_p1, vz_cscr_p1,
                             vx_scr_nx, vy_scr_nx, vz_scr_nx,
                             vx_scr_ny, vy_scr_ny, vz_scr_ny,
                             s__gscr_offx, s__gscr_offy,
                             &vx_gscr_p1, &vy_gscr_p1, &vz_gscr_p1);

     TranslatePointInPlane ( vx_gscr_p1, vy_gscr_p1, vz_gscr_p1,
                             vx_scr_nx, vy_scr_nx, vz_scr_nx,
                             vx_scr_ny, vy_scr_ny, vz_scr_ny,
                             s__gscr_w, 0.0,
                             &vx_gscr_p2, &vy_gscr_p2, &vz_gscr_p2);

     TranslatePointInPlane ( vx_gscr_p2, vy_gscr_p2, vz_gscr_p2,
                             vx_scr_nx, vy_scr_nx, vz_scr_nx,
                             vx_scr_ny, vy_scr_ny, vz_scr_ny,
                             0.0, s__gscr_h,
                             &vx_gscr_p3, &vy_gscr_p3, &vz_gscr_p3);

     TranslatePointInPlane ( vx_gscr_p3, vy_gscr_p3, vz_gscr_p3,
                             vx_scr_nx, vy_scr_nx, vz_scr_nx,
                             vx_scr_ny, vy_scr_ny, vz_scr_ny,
                             -s__gscr_w, 0.0,
                             &vx_gscr_p4, &vy_gscr_p4, &vz_gscr_p4);

     TranslatePointInPlane ( vx_gscr_p1, vy_gscr_p1, vz_gscr_p1,
                             vx_scr_nx, vy_scr_nx, vz_scr_nx,
                             vx_scr_ny, vy_scr_ny, vz_scr_ny,
                             0.0, s__gscr_h,
                             &vx_gscr_p4b, &vy_gscr_p4b, &vz_gscr_p4b);

     t1 =  CompareVec (vx_gscr_p4,  vy_gscr_p4,  vz_gscr_p4,
                       vx_gscr_p4b, vy_gscr_p4b, vz_gscr_p4b);

    DeltaVec (vx_gscr_p1, vy_gscr_p1, vz_gscr_p1, vx_gscr_p2, vy_gscr_p2, vz_gscr_p2,
              &vx_gscr_dw, &vy_gscr_dw, &vz_gscr_dw);
    DeltaVec (vx_gscr_p1, vy_gscr_p1, vz_gscr_p1, vx_gscr_p4, vy_gscr_p4, vz_gscr_p4,
              &vx_gscr_dh, &vy_gscr_dh, &vz_gscr_dh);

#ifndef NDEBUG
    printf("GLINFO: test v__cscr_dh - ( v__scr_ny * s__cscr_h ) = %f\n", t1);
    printf("GLINFO: cabinet vectors\n");
    printf("\t cab p1     : %f / %f / %f \n", vx_cscr_p1, vy_cscr_p1, vz_cscr_p1);
    printf("\t cab p2     : %f / %f / %f \n", vx_cscr_p2, vy_cscr_p2, vz_cscr_p2);
    printf("\t cab p3     : %f / %f / %f \n", vx_cscr_p3, vy_cscr_p3, vz_cscr_p3);
    printf("\t cab p4     : %f / %f / %f \n", vx_cscr_p4, vy_cscr_p4, vz_cscr_p4);
    printf("\n") ;
    printf("\t cab width  : %f / %f / %f \n", vx_cscr_dw, vy_cscr_dw, vz_cscr_dw);
    printf("\t cab height : %f / %f / %f \n", vx_cscr_dh, vy_cscr_dh, vz_cscr_dh);
    printf("\n");
    printf("\t x axis : %f / %f / %f \n", vx_scr_nx, vy_scr_nx, vz_scr_nx);
    printf("\t y axis : %f / %f / %f \n", vx_scr_ny, vy_scr_ny, vz_scr_ny);
    printf("\t z axis : %f / %f / %f \n", vx_scr_nz, vy_scr_nz, vz_scr_nz);
    printf("\n");
    printf("\n");
    printf("\t cab wxh scal wd: %f x %f \n", s__cscr_w, s__cscr_h);
    printf("\t cab wxh scal vw: %f x %f \n", s__cscr_w_view, s__cscr_h_view);
    printf("\n");
    printf("\t gam p1     : %f / %f / %f \n", vx_gscr_p1, vy_gscr_p1, vz_gscr_p1);
    printf("\t gam p2     : %f / %f / %f \n", vx_gscr_p2, vy_gscr_p2, vz_gscr_p2);
    printf("\t gam p3     : %f / %f / %f \n", vx_gscr_p3, vy_gscr_p3, vz_gscr_p3);
    printf("\t gam p4     : %f / %f / %f \n", vx_gscr_p4, vy_gscr_p4, vz_gscr_p4);
    printf("\t gam p4b    : %f / %f / %f \n", vx_gscr_p4b, vy_gscr_p4b, vz_gscr_p4b);
    printf("\t gam p4-p4b : %f\n", t1);
    printf("\n");
    printf("\t gam width  : %f / %f / %f \n", vx_gscr_dw, vy_gscr_dw, vz_gscr_dw);
    printf("\t gam height : %f / %f / %f \n", vx_gscr_dh, vy_gscr_dh, vz_gscr_dh);
    printf("\n");
    printf("\t gam wxh scal wd: %f x %f \n", s__gscr_w, s__gscr_h);
    printf("\t gam wxh scal vw: %f x %f \n", s__gscr_w_view, s__gscr_h_view);
    printf("\n");
    printf("\t gam off  wd: %f / %f\n", s__gscr_offx, s__gscr_offy);
    printf("\t gam off  vw: %f / %f\n", s__gscr_offx_view, s__gscr_offy_view);
    printf("\n");
#endif
  }
  
  if (sysdep_display_params.vec_src_bounds)
  {
    veclist=disp__glGenLists(1);
    RETURN_IF_GL_ERROR ();
    disp__glBlendFunc (GL_SRC_ALPHA, GL_ONE);
    RETURN_IF_GL_ERROR ();
    if (gl_set_beam(gl_beam))
      return 1;
  }

  /* no alpha .. important, because mame has no alpha set ! */
  gl_internal_format=GL_RGB;

  /* fill the sysdep_display_properties struct & determine bitmap format */
  memset (&sysdep_display_properties, 0, sizeof (sysdep_display_properties));
  switch(sysdep_display_params.depth)
  {
	case 15:
		    /* ARGB1555 */
		    sysdep_display_properties.palette_info.red_mask   = 0x00007C00;
		    sysdep_display_properties.palette_info.green_mask = 0x000003E0;
		    sysdep_display_properties.palette_info.blue_mask  = 0x0000001F;
		    gl_bitmap_format = GL_BGRA;
		    /*                                   A R G B */
		    gl_bitmap_type   = GL_UNSIGNED_SHORT_1_5_5_5_REV;
		    break;
	case 16:
		    /* RGB565 */
		    sysdep_display_properties.palette_info.red_mask   = 0x0000F800;
		    sysdep_display_properties.palette_info.green_mask = 0x000007C0;
		    sysdep_display_properties.palette_info.blue_mask  = 0x0000003F;
		    gl_bitmap_format = GL_RGB;
		    /*                                   R G B */
		    gl_bitmap_type   = GL_UNSIGNED_SHORT_5_6_5;
		    break;
	case 32:
		  /* ARGB8888 */
		  sysdep_display_properties.palette_info.red_mask    = 0x00FF0000;
		  sysdep_display_properties.palette_info.green_mask  = 0x0000FF00;
		  sysdep_display_properties.palette_info.blue_mask   = 0x000000FF;
		  gl_bitmap_format = GL_BGRA;
		  /*                                 A R G B */
		  gl_bitmap_type   = GL_UNSIGNED_INT_8_8_8_8_REV;
		  break;
  }
  sysdep_display_properties.vector_renderer = glvec_renderer;

  /* determine the texture size to use */
  if(force_text_width_height>0)
  {
    text_height=force_text_width_height;
    text_width=force_text_width_height;
    fprintf (stderr, "GLINFO: force_text_width_height := %d x %d\n",
             text_height, text_width);
  }
  else
  {
    text_width  = sysdep_display_params.orig_width;
    text_height = sysdep_display_params.orig_height;
  }
  
  /* achieve the 2**e_x:=text_width, 2**e_y:=text_height */
  x=1;
  while (x<text_width)
    x*=2;
  text_width=x;

  y=1;
  while (y<text_height)
    y*=2;
  text_height=y;

  /* Test the max texture size */
  while(!format_ok && text_width>=64 && text_height>=64)
  {
    disp__glTexImage2D (GL_PROXY_TEXTURE_2D, 0,
		  gl_internal_format,
		  text_width, text_height,
		  0, gl_bitmap_format, gl_bitmap_type, 0);
    CHECK_GL_ERROR ();

    disp__glGetTexLevelParameteriv
      (GL_PROXY_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, &format);
    CHECK_GL_ERROR ();

#ifndef NOTEXIDXSIZE
    disp__glGetTexLevelParameteriv
      (GL_PROXY_TEXTURE_2D, 0, GL_TEXTURE_INDEX_SIZE_EXT, &tidxsize);
    CHECK_GL_ERROR ();
#else
    tidxsize = -1;
#endif
    
    switch(sysdep_display_params.depth)
    {
      case 15:
        if(format == GL_RGB || format == GL_RGB5)
          format_ok = 1;
        break;
      case 16:
        if(format == GL_RGB)
          format_ok = 1;
        break;
      case 32:
        if(format == GL_RGB || format == GL_RGB8)
          format_ok = 1;
        break;
    }

    if (!format_ok)
    {
      fprintf (stderr, "GLINFO: Needed texture [%dx%d] too big (format=0x%X,idxsize=%d), ",
		text_height, text_width, format, tidxsize);
      if (text_width > text_height)
	text_width /= 2;
      else
	text_height /= 2;
      fprintf (stderr, "trying [%dx%d] !\n", text_height, text_width);
    }
  }

  if(!format_ok)
  {
    fprintf (stderr, "GLERROR: Give up .. usable texture size not available, or texture config error !\n");
    return 1;
  }
  
  texnumx = sysdep_display_params.orig_width / text_width;
  if ((sysdep_display_params.orig_width % text_width) > 0)
    texnumx++;

  texnumy = sysdep_display_params.orig_height / text_height;
  if ((sysdep_display_params.orig_height % text_height) > 0)
    texnumy++;

  fprintf (stderr, "GLINFO: texture-usage %d*width=%d, %d*height=%d\n",
		 (int) texnumx, (int) text_width, (int) texnumy,
		 (int) text_height);

  /* allocate some buffers */
  texgrid    = calloc(texnumx * texnumy, sizeof (struct TexSquare));
  empty_text = calloc(text_width*text_height, bytes_per_pixel);
  if (!texgrid || !empty_text)
  {
    fprintf(stderr, "GLERROR: couldn't allocate memory\n");
    return 1;
  }

  /**
   * texwpervw, texhpervh:
   * 	how much the texture covers the visual,
   * 	for both components (width/height) (percent).
   */
  texwpervw = (GLdouble) text_width / (GLdouble) sysdep_display_params.orig_width;
  if (texwpervw > 1.0)
    texwpervw = 1.0;

  texhpervh = (GLdouble) text_height / (GLdouble) sysdep_display_params.orig_height;
  if (texhpervh > 1.0)
    texhpervh = 1.0;

  /* create (but don't "fill") the textures */
  for (y = 0; y < texnumy; y++)
  {
    for (x = 0; x < texnumx; x++)
    {
      tsq = texgrid + y * texnumx + x;
  
      if (x == texnumx - 1 && sysdep_display_params.orig_width % text_width)
	tsq->xcov =
	  (GLdouble) (sysdep_display_params.orig_width % text_width) / (GLdouble) text_width;
      else
	tsq->xcov = 1.0;

      if (y == texnumy - 1 && sysdep_display_params.orig_height % text_height)
	tsq->ycov =
	  (GLdouble) (sysdep_display_params.orig_height % text_height) / (GLdouble) text_height;
      else
	tsq->ycov = 1.0;

      CalcFlatTexPoint (x, y, texwpervw, texhpervh, &(tsq->fx1), &(tsq->fy1));
      CalcFlatTexPoint (x + 1, y, texwpervw, texhpervh, &(tsq->fx2), &(tsq->fy2));
      CalcFlatTexPoint (x + 1, y + 1, texwpervw, texhpervh, &(tsq->fx3), &(tsq->fy3));
      CalcFlatTexPoint (x, y + 1, texwpervw, texhpervh, &(tsq->fx4), &(tsq->fy4));

      CalcCabPointbyViewpoint( x*(GLdouble)text_width,
      		    y*(GLdouble)text_height,
                   &(tsq->x1), &(tsq->y1), &(tsq->z1));

      CalcCabPointbyViewpoint( x*(GLdouble)text_width  + tsq->xcov*(GLdouble)text_width,
      		    y*(GLdouble)text_height,
                   &(tsq->x2), &(tsq->y2), &(tsq->z2));

      CalcCabPointbyViewpoint( x*(GLdouble)text_width  + tsq->xcov*(GLdouble)text_width,
      		    y*(GLdouble)text_height + tsq->ycov*(GLdouble)text_height,
                   &(tsq->x3), &(tsq->y3), &(tsq->z3));

      CalcCabPointbyViewpoint( x*(GLdouble)text_width,
      		    y*(GLdouble)text_height + tsq->ycov*(GLdouble)text_height,
                   &(tsq->x4), &(tsq->y4), &(tsq->z4));

      tsq->texobj=0;
      disp__glGenTextures (1, &(tsq->texobj));
      RETURN_IF_GL_ERROR ();
      disp__glBindTexture (GL_TEXTURE_2D, tsq->texobj);
      RETURN_IF_GL_ERROR ();

      if(disp__glIsTexture(tsq->texobj) == GL_FALSE)
      {
	fprintf (stderr, "GLERROR ain't a texture (glGenText): texnum x=%d, y=%d, texture=%d\n",
		x, y, tsq->texobj);
      }
      RETURN_IF_GL_ERROR ();

      disp__glTexImage2D (GL_TEXTURE_2D, 0,
		    gl_internal_format,
		    text_width, text_height,
		    0, gl_bitmap_format, gl_bitmap_type, empty_text);
      RETURN_IF_GL_ERROR ();
      disp__glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_PRIORITY, 1.0);
      RETURN_IF_GL_ERROR ();
      disp__glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
      RETURN_IF_GL_ERROR ();
      disp__glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
      RETURN_IF_GL_ERROR ();
      disp__glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
      RETURN_IF_GL_ERROR ();
    }	/* for all texnumx */
  }  /* for all texnumy */
  free (empty_text);
  empty_text   = NULL;

  /* lets init the rest of the customizings ... */
  if (gl_set_bilinear (bilinear))
    return 1;
  if (gl_set_antialias (antialias))
    return 1;
  if (gl_set_cabview (cabview))
    return 1;

  /* done */
  if(sysdep_display_params.depth == 16)
  {
    colorBlittedMemory = malloc( sysdep_display_params.width * sysdep_display_params.height * bytes_per_pixel);
    if (!colorBlittedMemory)
    {
      fprintf(stderr, "GLERROR: couldn't allocate memory\n");
      return 1;
    }
    printf("GLINFO: Using bit blit to map color indices !!\n");
  } else {
    printf("GLINFO: Using true color mode (no color indices, but direct color)!!\n");
  }

  printf("GLINFO: depth=%d, rgb 0x%X, 0x%X, 0x%X (true color mode)\n",
		sysdep_display_params.depth, 
		sysdep_display_properties.palette_info.red_mask, sysdep_display_properties.palette_info.green_mask, 
		sysdep_display_properties.palette_info.blue_mask);
  
  return 0;
}

/* Close down the virtual screen */
void gl_close_display (void)
{
  CHECK_GL_BEGINEND();
  CHECK_GL_ERROR();

  if(colorBlittedMemory!=NULL)
  {
    free(colorBlittedMemory);
    colorBlittedMemory = NULL;
  }
  if (empty_text)
  {
    free (empty_text);
    empty_text = NULL;
  }
  if (texgrid)
  {
    free(texgrid);
    texgrid = NULL;
  }
  if (cpan)
  {
    free(cpan);
    cpan = NULL;
  }

  if (veclist)
  {
    disp__glDeleteLists(veclist, 1);
    CHECK_GL_ERROR();
    veclist = 0;
  }

  texture_init = 0;
}

/**
 * the given bitmap MUST be the original mame core bitmap !!!
 *    - no swapxy, flipx or flipy and no resize !
 *    - shall be Machine->scrbitmap
 */
static void InitTextures (struct mame_bitmap *bitmap, struct rectangle *vis_area)
{
  int x=0, y=0;
  unsigned char *line_1=0;
  struct TexSquare *tsq=0;
  int line_len;
  int bytes_per_pixel = (sysdep_display_params.depth + 7) / 8;

  if(sysdep_display_params.depth == 16) 
  {
    line_1   = (unsigned char *)colorBlittedMemory;
    line_len = sysdep_display_params.orig_width;
  }
  else
  {
    unsigned char *line_2;
    line_1   = (unsigned char *) bitmap->line[vis_area->min_y];
    line_2   = (unsigned char *) bitmap->line[vis_area->min_y + 1];
    line_len = (line_2 - line_1) / bytes_per_pixel;
    line_1  += vis_area->min_x * bytes_per_pixel;
  }

  disp__glPixelStorei (GL_UNPACK_ROW_LENGTH, line_len);
  CHECK_GL_ERROR ();

  for (y = 0; y < texnumy; y++)
  {
    for (x = 0; x < texnumx; x++)
    {
      tsq = texgrid + y * texnumx + x;

      /* calculate the pixel store data, to use the machine-bitmap for our texture */
      tsq->texture = line_1 +
        ( (y * text_height * line_len) + (x * text_width)) * bytes_per_pixel;
    }	/* for all texnumx */
  }  /* for all texnumy */

  if (sysdep_display_params.vec_src_bounds)
  {
    if(sysdep_display_params.vec_dest_bounds)
    {
      vecx = (GLdouble)sysdep_display_params.vec_dest_bounds->min_x/
        sysdep_display_params.orig_width;
      vecy = (GLdouble)sysdep_display_params.vec_dest_bounds->min_y/
        sysdep_display_params.orig_height;
      vecscalex = (65536.0 * sysdep_display_params.orig_width *
        (sysdep_display_params.vec_src_bounds->max_x-
         sysdep_display_params.vec_src_bounds->min_x)) /
        ((sysdep_display_params.vec_dest_bounds->max_x + 1) -
         sysdep_display_params.vec_dest_bounds->min_x);
      vecscaley = (65536.0 * sysdep_display_params.orig_height *
        (sysdep_display_params.vec_src_bounds->max_y-
         sysdep_display_params.vec_src_bounds->min_y)) /
        ((sysdep_display_params.vec_dest_bounds->max_y + 1) -
         sysdep_display_params.vec_dest_bounds->min_y);
    }
    else
    {
      vecx = 0.0;
      vecy = 0.0;
      vecscalex = (sysdep_display_params.vec_src_bounds->max_x-
        sysdep_display_params.vec_src_bounds->min_x) * 65536.0;
      vecscaley = (sysdep_display_params.vec_src_bounds->max_y-
        sysdep_display_params.vec_src_bounds->min_y) * 65536.0;
    }
  }

  texture_init = 1;
}

/**
 * returns the length of the |(x,y,z)-(i,j,k)|
 */
static GLdouble
CompareVec (GLdouble i, GLdouble j, GLdouble k,
	    GLdouble x, GLdouble y, GLdouble z)
{
  GLdouble dx = x-i;
  GLdouble dy = y-j;
  GLdouble dz = z-k;

  return LengthOfVec(dx, dy, dz);
}

static void
AddToThisVec (GLdouble i, GLdouble j, GLdouble k,
	      GLdouble * x, GLdouble * y, GLdouble * z)
{
  *x += i ;
  *y += j ;
  *z += k ;
}

/**
 * TranslatePointInPlane:
 *
 * v__p = v__p1 + 
 *        x_off * v__nw +
 *        y_off * v__nh;
 */
static void TranslatePointInPlane   (
	      GLdouble vx_p1, GLdouble vy_p1, GLdouble vz_p1,
	      GLdouble vx_nw, GLdouble vy_nw, GLdouble vz_nw,
	      GLdouble vx_nh, GLdouble vy_nh, GLdouble vz_nh,
	      GLdouble x_off, GLdouble y_off,
	      GLdouble *vx_p, GLdouble *vy_p, GLdouble *vz_p )
{
   GLdouble tx, ty, tz;

   CopyVec( vx_p, vy_p, vz_p,
            vx_p1, vy_p1, vz_p1); 

   CopyVec( &tx, &ty, &tz,
	    vx_nw, vy_nw, vz_nw);

   ScaleThisVec (x_off, x_off, x_off,
                 &tx, &ty, &tz);

   AddToThisVec (tx, ty, tz,
                 vx_p, vy_p, vz_p);

   CopyVec( &tx, &ty, &tz,
	    vx_nh, vy_nh, vz_nh);

   ScaleThisVec (y_off, y_off, y_off,
                 &tx, &ty, &tz);

   AddToThisVec (tx, ty, tz,
                 vx_p, vy_p, vz_p);
}

static void
ScaleThisVec (GLdouble i, GLdouble j, GLdouble k,
	      GLdouble * x, GLdouble * y, GLdouble * z)
{
  *x *= i ;
  *y *= j ;
  *z *= k ;
}

static GLdouble
LengthOfVec (GLdouble x, GLdouble y, GLdouble z)
{
  return sqrt(x*x+y*y+z*z);
}

static void
NormThisVec (GLdouble * x, GLdouble * y, GLdouble * z)
{
  double len = LengthOfVec (*x, *y, *z);

  *x /= len ;
  *y /= len ;
  *z /= len ;
}

/* Compute a delta vector between two points */
static void
DeltaVec (GLdouble x1, GLdouble y1, GLdouble z1,
	  GLdouble x2, GLdouble y2, GLdouble z2,
	  GLdouble * dx, GLdouble * dy, GLdouble * dz)
{
  *dx = x2 - x1;
  *dy = y2 - y1;
  *dz = z2 - z1;
}

/* Compute a crossproduct vector of two vectors ( plane ) */
static void
CrossVec (GLdouble a1, GLdouble a2, GLdouble a3,
	  GLdouble b1, GLdouble b2, GLdouble b3,
	  GLdouble * c1, GLdouble * c2, GLdouble * c3)
{
  *c1 = a2*b3 - a3*b2; 
  *c2 = a3*b1 - a1*b3;
  *c3 = a1*b2 - a1*b1;
}

static void CopyVec(GLdouble *ax,GLdouble *ay,GLdouble *az,  /* dest   */
  const GLdouble bx,const GLdouble by,const GLdouble bz)     /* source */
{
	*ax=bx;
	*ay=by;
	*az=bz;
}

/**
 * Calculate texture points (world) for flat screen 
 *
 * x,y: 
 * 	texture index (scalar)
 *	
 * texwpervw, texhpervh:
 * 	how much the texture covers the visual,
 * 	for both components (width/height) (percent).
 *
 * px,py,pz:
 * 	the resulting cabinet point
 *
 */
static void CalcFlatTexPoint( int x, int y, GLdouble texwpervw,
  GLdouble texhpervh, GLdouble *px,GLdouble *py)
{
  *px=(double)x*texwpervw;
  if(*px>1.0) *px=1.0;
  *py=(double)y*texhpervh;
  if(*py>1.0) *py=1.0;
}

/**
 * vx_gscr_view,vy_gscr_view:
 * 	view-corrd within game-screen
 *
 * vx_p,vy_p,vz_p: 
 * 	world-coord within cab-screen
 */
void CalcCabPointbyViewpoint( 
		   GLdouble vx_gscr_view, GLdouble vy_gscr_view, 
                   GLdouble *vx_p, GLdouble *vy_p, GLdouble *vz_p
		 )
{
  GLdouble vx_gscr = (GLdouble)vx_gscr_view/cab_vpw_fx;
  GLdouble vy_gscr = (GLdouble)vy_gscr_view/cab_vpw_fy;

   /**
    * v__p  = v__gscr_p1  + vx_gscr * v__scr_nx + vy_gscr * v__scr_ny ;
    */

   TranslatePointInPlane ( vx_gscr_p1, vy_gscr_p1, vz_gscr_p1,
                           vx_scr_nx, vy_scr_nx, vz_scr_nx,
			   vx_scr_ny, vy_scr_ny, vz_scr_ny,
			   vx_gscr, vy_gscr,
			   vx_p, vy_p, vz_p);
}

/* Set up a frustum projection */
static int SetupFrustum (void)
{
  GLenum err;
  double vscrnaspect = (double) window_width / (double) window_height;

  disp__glMatrixMode (GL_PROJECTION);
  RETURN_IF_GL_ERROR ();
  disp__glLoadIdentity ();
  RETURN_IF_GL_ERROR ();
  disp__glFrustum (-vscrnaspect, vscrnaspect, -1.0, 1.0, 5.0, 100.0);
  RETURN_IF_GL_ERROR ();
  disp__glGetDoublev(GL_PROJECTION_MATRIX, mxProjection);
  RETURN_IF_GL_ERROR ();
  disp__glMatrixMode (GL_MODELVIEW);
  RETURN_IF_GL_ERROR ();
  disp__glLoadIdentity ();
  RETURN_IF_GL_ERROR ();
  disp__glTranslatef (0.0, 0.0, -20.0);
  RETURN_IF_GL_ERROR ();
  disp__glGetDoublev(GL_MODELVIEW_MATRIX, mxModel);
  RETURN_IF_GL_ERROR ();
  
  return 0;
}

/* Set up an orthographic projection */
static int SetupOrtho (void)
{
  GLenum err;

  disp__glMatrixMode (GL_PROJECTION);
  RETURN_IF_GL_ERROR ();
  disp__glLoadIdentity ();
  RETURN_IF_GL_ERROR ();
  disp__glOrtho (-0.5,  0.5, -0.5,  0.5,  1.0,  -1.0); /* normal display ! */
  RETURN_IF_GL_ERROR ();
  disp__glMatrixMode (GL_MODELVIEW);
  RETURN_IF_GL_ERROR ();
  disp__glLoadIdentity ();
  RETURN_IF_GL_ERROR ();
  disp__glRotated ( 180.0 , 1.0, 0.0, 0.0);
  RETURN_IF_GL_ERROR ();

  if ( (sysdep_display_params.orientation & SYSDEP_DISPLAY_FLIPX) )
  {
	disp__glRotated (  180.0 , 0.0, 1.0, 0.0);
        RETURN_IF_GL_ERROR ();
  }

  if ( (sysdep_display_params.orientation & SYSDEP_DISPLAY_FLIPY) )
  {
	disp__glRotated ( -180.0 , 1.0, 0.0, 0.0);
        RETURN_IF_GL_ERROR ();
  }

  if( (sysdep_display_params.orientation & SYSDEP_DISPLAY_SWAPXY) ) {
	disp__glRotated ( 180.0 , 0.0, 1.0, 0.0);
        RETURN_IF_GL_ERROR ();
  	disp__glRotated (  90.0 , 0.0, 0.0, 1.0 );
        RETURN_IF_GL_ERROR ();
  }

  disp__glTranslated ( -0.5 , -0.5, 0.0 );
  RETURN_IF_GL_ERROR ();

  return 0;  	
}

int gl_resize(void)
{
  GLenum err;
  CHECK_GL_BEGINEND();

  if (cabview)
  {
          disp__glViewport (0, 0, window_width, window_height);
          RETURN_IF_GL_ERROR ();
          return SetupFrustum ();
  }
  else
  {
          int vw, vh;
          int vscrndx;
          int vscrndy;

          mode_clip_aspect(window_width, window_height, &vw, &vh);
          
          vscrndx = (window_width  - vw) / 2;
          vscrndy = (window_height - vh) / 2;

          disp__glViewport (vscrndx, vscrndy, vw, vh);
          RETURN_IF_GL_ERROR ();
          return SetupOrtho ();
  }
}

/* Compute an average between two sets of 3D coordinates */
static void
WAvg (GLdouble perc, GLdouble x1, GLdouble y1, GLdouble z1,
      GLdouble x2, GLdouble y2, GLdouble z2,
      GLdouble * ax, GLdouble * ay, GLdouble * az)
{
  *ax = (1.0 - perc) * x1 + perc * x2;
  *ay = (1.0 - perc) * y1 + perc * y2;
  *az = (1.0 - perc) * z1 + perc * z2;
}

#if 0 /* not used */
static void drawGameAxis ()
{
  GLdouble tx, ty, tz;

	disp__glPushMatrix ();

	disp__glLineWidth(1.5f);
	disp__glBegin(GL_LINES);

	/** x-axis **/
	disp__glColor3d (1.0,1.0,1.0);
	disp__glVertex3d(0,0,0);

	disp__glColor3d (1.0,0.0,0.0);
        CopyVec( &tx, &ty, &tz,
	         vx_scr_nx, vy_scr_nx, vz_scr_nx);
        ScaleThisVec ( 5.0, 5.0, 5.0, &tx, &ty, &tz);
	disp__glVertex3d( tx, ty, tz );


	/** y-axis **/
	disp__glColor3d (1.0,1.0,1.0);
	disp__glVertex3d(0,0,0);

	disp__glColor3d (0.0,1.0,0.0);
        CopyVec( &tx, &ty, &tz,
	         vx_scr_ny, vy_scr_ny, vz_scr_ny);
        ScaleThisVec ( 5.0, 5.0, 5.0, &tx, &ty, &tz);
	disp__glVertex3d( tx, ty, tz );


	/** z-axis **/
	disp__glColor3d (1.0,1.0,1.0);
	disp__glVertex3d(0,0,0);

	disp__glColor3d (0.0,0.0,1.0);
        CopyVec( &tx, &ty, &tz,
	         vx_scr_nz, vy_scr_nz, vz_scr_nz);
        ScaleThisVec ( 5.0, 5.0, 5.0, &tx, &ty, &tz);
	disp__glVertex3d( tx, ty, tz );

	disp__glEnd();

	disp__glPopMatrix ();
	disp__glColor3d (1.0,1.0,1.0);
        CHECK_GL_ERROR ();
}
#endif

static void cabinetTextureRotationTranslation ()
{
	/**
	 * Be aware, this matrix is written in reverse logical
	 * order of GL commands !
	 *
	 * This is the way OpenGL stacks does work !
	 *
	 * So if you interprete this code,
	 * you have to start from the bottom from this block !!
	 *
	 * !!!!!!!!!!!!!!!!!!!!!!
	 */

	/** END  READING ... TRANSLATION / ROTATION **/

	/* go back on screen */
	disp__glTranslated ( vx_gscr_p1, vy_gscr_p1, vz_gscr_p1); 
	CHECK_GL_ERROR ();

	/* x-border -> I. Q */
	disp__glTranslated ( vx_gscr_dw/2.0, vy_gscr_dw/2.0, vz_gscr_dw/2.0);
	CHECK_GL_ERROR ();

	/* y-border -> I. Q */
	disp__glTranslated ( vx_gscr_dh/2.0, vy_gscr_dh/2.0, vz_gscr_dh/2.0);
	CHECK_GL_ERROR ();

	/********* CENTERED AT ORIGIN END  ****************/

	if ( (sysdep_display_params.orientation & SYSDEP_DISPLAY_FLIPX) )
	{
		disp__glRotated ( -180.0 , vx_scr_ny, vy_scr_ny, vz_scr_ny);
		CHECK_GL_ERROR ();
	}

	if ( (sysdep_display_params.orientation & SYSDEP_DISPLAY_FLIPY) )
	{
		disp__glRotated ( -180.0 , vx_scr_nx, vy_scr_nx, vz_scr_nx);
		CHECK_GL_ERROR ();
	}

	/********* CENTERED AT ORIGIN BEGIN ****************/
	if( (sysdep_display_params.orientation & SYSDEP_DISPLAY_SWAPXY) )
	{
		disp__glRotated ( -180.0 , vx_scr_ny, vy_scr_ny, vz_scr_ny);
		CHECK_GL_ERROR ();

		/* x-center */
		disp__glTranslated ( vx_gscr_dw/2.0, vy_gscr_dw/2.0, vz_gscr_dw/2.0);
		CHECK_GL_ERROR ();

		/* y-center */
		disp__glTranslated ( vx_gscr_dh/2.0, vy_gscr_dh/2.0, vz_gscr_dh/2.0);
		CHECK_GL_ERROR ();

		/* swap -> III. Q */
		disp__glRotated ( -90.0 , vx_scr_nz, vy_scr_nz, vz_scr_nz);
		CHECK_GL_ERROR ();
	} else {
		/* x-center */
		disp__glTranslated ( -vx_gscr_dw/2.0, -vy_gscr_dw/2.0, -vz_gscr_dw/2.0);
		CHECK_GL_ERROR ();

		/* y-center */
		disp__glTranslated ( vx_gscr_dh/2.0, vy_gscr_dh/2.0, vz_gscr_dh/2.0);
		CHECK_GL_ERROR ();
	}

	/* re-flip -> IV. Q     (normal) */
	disp__glRotated ( 180.0 , vx_scr_nx, vy_scr_nx, vz_scr_nx);
	CHECK_GL_ERROR ();

	/* go to origin -> I. Q (flipx) */
	disp__glTranslated ( -vx_gscr_p1, -vy_gscr_p1, -vz_gscr_p1); 
	CHECK_GL_ERROR ();

	/** START READING ... TRANSLATION / ROTATION **/
}

static void drawTextureDisplay (struct mame_bitmap *bitmap,
	  struct rectangle *vis_area,  struct rectangle *dirty_area,
	  struct sysdep_palette_struct *palette,
	  unsigned int flags)
{
  struct TexSquare *square;
  int x = 0, y = 0;
  static const double z_pos = 0.9f;
  int updateTexture = 1;
  static int ui_was_dirty=0;

  if(sysdep_display_params.vec_src_bounds && !(flags & SYSDEP_DISPLAY_UI_DIRTY)
     && !ui_was_dirty)
    updateTexture = 0;

  if (updateTexture && (sysdep_display_params.depth == 16))
  {
    	unsigned short *dest=colorBlittedMemory;
    	int y,x;
    	for (y=vis_area->min_y;y<=vis_area->max_y;y++)
    	{
    	   for (x=vis_area->min_x; x<=vis_area->max_x; x++, dest++)
    	   {
    	      *dest=palette->lookup[((UINT16*)(bitmap->line[y]))[x]];
    	   }
    	}
  }

  disp__glColor4d (1.0, 1.0, 1.0, 1.0);
  CHECK_GL_ERROR ();
  disp__glEnable (GL_TEXTURE_2D);
  CHECK_GL_ERROR ();

  for (y = 0; y < texnumy; y++)
  {
    for (x = 0; x < texnumx; x++)
    {
      int width,height;
      
      square = texgrid + y * texnumx + x;
      
      if(x<(texnumx-1) || !(sysdep_display_params.orig_width%text_width))
		width=text_width;
      else width=sysdep_display_params.orig_width%text_width;

      if(y<(texnumy-1) || !(sysdep_display_params.orig_height%text_height))
		height=text_height;
      else height=sysdep_display_params.orig_height%text_height;

      disp__glBindTexture (GL_TEXTURE_2D, square->texobj);
      CHECK_GL_ERROR ();

      /* This is the quickest way I know of to update the texture */
      if (updateTexture)
      {
	disp__glTexSubImage2D (GL_TEXTURE_2D, 0, 0, 0,
		width, height,
		gl_bitmap_format, gl_bitmap_type, square->texture);
        CHECK_GL_ERROR ();
      }

      if (cabview)
      {
  	GL_BEGIN(GL_QUADS);
        CHECK_GL_ERROR ();
	disp__glTexCoord2d (0, 0);
        CHECK_GL_ERROR ();
	disp__glVertex3d (square->x1, square->y1, square->z1);
        CHECK_GL_ERROR ();
	disp__glTexCoord2d (square->xcov, 0);
        CHECK_GL_ERROR ();
	disp__glVertex3d (square->x2, square->y2, square->z2);
        CHECK_GL_ERROR ();
	disp__glTexCoord2d (square->xcov, square->ycov);
        CHECK_GL_ERROR ();
	disp__glVertex3d (square->x3, square->y3, square->z3);
        CHECK_GL_ERROR ();
	disp__glTexCoord2d (0, square->ycov);
        CHECK_GL_ERROR ();
	disp__glVertex3d (square->x4, square->y4, square->z4);
        CHECK_GL_ERROR ();
	GL_END();
      }
      else
      {
	GL_BEGIN(GL_QUADS);
        CHECK_GL_ERROR ();
	disp__glTexCoord2d (0, 0);
        CHECK_GL_ERROR ();
	disp__glVertex3d (square->fx1, square->fy1, z_pos);
        CHECK_GL_ERROR ();
	disp__glTexCoord2d (square->xcov, 0);
        CHECK_GL_ERROR ();
	disp__glVertex3d (square->fx2, square->fy2, z_pos);
        CHECK_GL_ERROR ();
	disp__glTexCoord2d (square->xcov, square->ycov);
        CHECK_GL_ERROR ();
	disp__glVertex3d (square->fx3, square->fy3, z_pos);
        CHECK_GL_ERROR ();
	disp__glTexCoord2d (0, square->ycov);
        CHECK_GL_ERROR ();
	disp__glVertex3d (square->fx4, square->fy4, z_pos);
        CHECK_GL_ERROR ();
	GL_END();
      }
    } /* for all texnumx */
  } /* for all texnumy */
  CHECK_GL_BEGINEND();

  disp__glDisable (GL_TEXTURE_2D);
  CHECK_GL_ERROR ();
  
  /* Draw the vectors if in vector mode */
  if (sysdep_display_params.vec_src_bounds)
  {
    if (antialiasvec)
    {
      disp__glEnable (GL_LINE_SMOOTH);
      CHECK_GL_ERROR ();
      disp__glEnable (GL_POINT_SMOOTH);
      CHECK_GL_ERROR ();
    }
    else
    {
      disp__glDisable (GL_LINE_SMOOTH);
      CHECK_GL_ERROR ();
      disp__glDisable (GL_POINT_SMOOTH);
      CHECK_GL_ERROR ();
    }

    disp__glShadeModel (GL_FLAT);
    CHECK_GL_ERROR ();
    disp__glEnable (GL_BLEND);
    CHECK_GL_ERROR ();
    disp__glCallList (veclist);
    CHECK_GL_BEGINEND();
    CHECK_GL_ERROR ();
    disp__glDisable (GL_BLEND);
    CHECK_GL_ERROR ();

    /* restore normal antialias settings */
    gl_set_antialias (antialias);
  }

  ui_was_dirty = flags & SYSDEP_DISPLAY_UI_DIRTY;
}

/* Draw a frame in Cabinet mode */
static void UpdateCabDisplay (struct mame_bitmap *bitmap,
	  struct rectangle *vis_area,  struct rectangle *dirty_area,
	  struct sysdep_palette_struct *palette,
	  unsigned int flags)
{
  GLdouble camx, camy, camz;
  GLdouble dirx, diry, dirz;
  GLdouble normx, normy, normz;
  GLdouble perc;
  struct CameraPan *pan, *lpan;

  disp__glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  CHECK_GL_ERROR ();
  disp__glPushMatrix ();
  CHECK_GL_ERROR ();

  /* Do the camera panning */
  if (cpan)
  {
    pan = cpan + currentpan;

/*
    printf("GLINFO (glcab): pan %d/%d panframe %d/%d\n", 
    	currentpan, numpans, panframe, pan->frames);
*/

    if (0 >= panframe || panframe >= pan->frames)
    {
      lastpan = currentpan;
      currentpan += 1;
      if(currentpan>=numpans) currentpan=1;
      panframe = 0;
/*
      printf("GLINFO (glcab): finished pan %d/%d\n", currentpan, numpans);
*/
    }

    switch (pan->type)
    {
    case pan_goto:
      camx = pan->lx;
      camy = pan->ly;
      camz = pan->lz;
      dirx = pan->px;
      diry = pan->px;
      dirz = pan->pz;
      normx = pan->nx;
      normy = pan->ny;
      normz = pan->nz;
      break;
    case pan_moveto:
      lpan = cpan + lastpan;
      perc = (GLdouble) panframe / (GLdouble) pan->frames;
      WAvg (perc, lpan->lx, lpan->ly, lpan->lz,
	    pan->lx, pan->ly, pan->lz, &camx, &camy, &camz);
      WAvg (perc, lpan->px, lpan->py, lpan->pz,
	    pan->px, pan->py, pan->pz, &dirx, &diry, &dirz);
      WAvg (perc, lpan->nx, lpan->ny, lpan->nz,
	    pan->nx, pan->ny, pan->nz, &normx, &normy, &normz);
      break;
    default:
      break;
    }

    disp__gluLookAt (camx, camy, camz, dirx, diry, dirz, normx, normy, normz);

    panframe++;
  }
  else
    disp__gluLookAt (-5.0, 0.0, 5.0, 0.0, 0.0, -5.0, 0.0, 1.0, 0.0);

  CHECK_GL_ERROR ();
  disp__glEnable (GL_DEPTH_TEST);
  CHECK_GL_ERROR ();

  /* Draw the cabinet */
  disp__glCallList (cablist);
  CHECK_GL_BEGINEND();
  CHECK_GL_ERROR ();

  /* Draw the game screen */
  cabinetTextureRotationTranslation ();
  drawTextureDisplay (bitmap, vis_area, dirty_area, palette, flags);

  disp__glDisable (GL_DEPTH_TEST);
  CHECK_GL_ERROR ();
}

/**
 * the given bitmap MUST be the original mame core bitmap !!!
 *    - no swapxy, flipx or flipy and no resize !
 *    - shall be Machine->scrbitmap
 */
static void UpdateGLDisplay (struct mame_bitmap *bitmap,
	  struct rectangle *vis_area,  struct rectangle *dirty_area,
	  struct sysdep_palette_struct *palette,
	  unsigned int flags)
{
  if (!texture_init || (flags & SYSDEP_DISPLAY_VIS_AREA_CHANGED))
    InitTextures (bitmap, vis_area);

  if (cabview)
  {
    UpdateCabDisplay (bitmap, vis_area, dirty_area, palette, flags);
  }
  else
  {
    disp__glClear (GL_COLOR_BUFFER_BIT);
    CHECK_GL_ERROR ();
    disp__glPushMatrix ();
    CHECK_GL_ERROR ();
    drawTextureDisplay (bitmap, vis_area, dirty_area, palette, flags);
  }

  disp__glPopMatrix ();
  CHECK_GL_ERROR ();
}

/**
 * the given bitmap MUST be the original mame core bitmap !!!
 *    - no swapxy, flipx or flipy and no resize !
 *    - shall be Machine->scrbitmap
 */
void gl_update_display(struct mame_bitmap *bitmap,
	  struct rectangle *vis_area,  struct rectangle *dirty_area,
	  struct sysdep_palette_struct *palette,
	  unsigned int flags)
{
  UpdateGLDisplay (bitmap, vis_area, dirty_area, palette, flags);
/* FIXME
  if (code_pressed (KEYCODE_RALT))
  {
    if (code_pressed_memory (KEYCODE_A))
    {
	  gl_set_antialias (1-antialias);
	  printf("GLINFO: switched antialias := %d\n", antialias);
    }
    else if (code_pressed_memory (KEYCODE_B))
    {
      gl_set_bilinear (1 - bilinear);
      printf("GLINFO: switched bilinear := %d\n", bilinear);
    }
    else if (code_pressed_memory (KEYCODE_C))
    {
      gl_set_cabview (1-cabview);
      printf("GLINFO: switched cabinet := %d\n", cabview);
    }
    else if (code_pressed_memory (KEYCODE_PLUS_PAD))
    {
	gl_set_beam(gl_beam+0.5);
    }
    else if (code_pressed_memory (KEYCODE_MINUS_PAD))
    {
	gl_set_beam(gl_beam-0.5);
    }
  }
  */
}

#if 0 /* disabled for now */
struct mame_bitmap *osd_override_snapshot(struct mame_bitmap *bitmap,
		struct rectangle *bounds)
{
	do_snapshot = 1;
	return NULL;
}
#endif
