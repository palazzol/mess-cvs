/* Blit & Video Effect Handling
 *
 * Original version:		Ben Saylor - bsaylor@macalester.edu 
 * Clean up / shuffle:		Hans de Goede
 */
#include <stdlib.h>
#include <string.h>
#include "blit/blit.h"
#include "sysdep/sysdep_display_priv.h"
#include "effect.h"

/* defines/ enums */
#define RMASK16_INV(P) ((P) & 0x07ff)
#define GMASK16_INV(P) ((P) & 0xf81f)
#define BMASK16_INV(P) ((P) & 0xffe0)
#define RMASK32_INV(P) ((P) & 0x0000ffff)
#define GMASK32_INV(P) ((P) & 0x00ff00ff)
#define BMASK32_INV(P) ((P) & 0x00ffff00)
#define SYSDEP_DISPLAY_EFFECT_MODES (EFFECT_COLOR_FORMATS*3) /* 15,16,32 src */
/* We differentiate between 6 different destination types */
enum { EFFECT_UNKNOWN = -1, EFFECT_15, EFFECT_16, EFFECT_24, EFFECT_32,
  EFFECT_YUY2, EFFECT_YV12, EFFECT_COLOR_FORMATS };

/* public variables */
char *effect_dbbuf  = NULL;
char *rotate_dbbuf0 = NULL;
char *rotate_dbbuf1 = NULL;
char *rotate_dbbuf2 = NULL;
/* for the 6tap filter */
char *_6tap2x_buf0 = NULL;
char *_6tap2x_buf1 = NULL;
char *_6tap2x_buf2 = NULL;
char *_6tap2x_buf3 = NULL;
char *_6tap2x_buf4 = NULL;
char *_6tap2x_buf5 = NULL;
void (*rotate_func)(void *dst, struct mame_bitmap *bitamp, int y, struct rectangle *bounds);

const struct sysdep_display_effect_properties_struct sysdep_display_effect_properties[] = {
  { 1, 8, 1, 8, 0,                                  "no effect" },
  { 2, 3, 2, 6, 0,                                  "smooth scaling" },
  { 1, 4, 2, 2, SYSDEP_DISPLAY_Y_SCALE_LOCKED,      "light scanlines (h)" },
  { 1, 6, 3, 3, SYSDEP_DISPLAY_Y_SCALE_LOCKED,      "rgb scanlines (h)" }, 
  { 2, 6, 3, 3, SYSDEP_DISPLAY_Y_SCALE_LOCKED,      "deluxe scanlines (h)" },
  { 2, 2, 1, 8, SYSDEP_DISPLAY_X_SCALE_LOCKED,      "light scanlines (v)" },
  { 3, 3, 1, 8, SYSDEP_DISPLAY_X_SCALE_LOCKED,      "rgb scanlines (v)" }, 
  { 3, 3, 1, 8, SYSDEP_DISPLAY_X_SCALE_LOCKED,      "deluxe scanlines (v)" }
};

#if 0  
  { 2, 2, 2, 2, 0, "low quality filter" },   /* lq2x */
  { 2, 2, 2, 2, 0, "high quality filter" },  /* hq2x */
  { 2, 2, 2, 2, 0, "6-tap filter & scanlines" }, /* 6tap2x */
  { 1, 8, 2, 8, 0, "black scanlines" }       /* fakescan */
#endif
 
/* Private variables
   
   We save the original palette info to restore it on close
   as we modify it for some special cases */
static struct sysdep_palette_info orig_palette_info;
static int orig_palette_info_saved = 0;
/* array with all the effect functions:
   6x 15 to ... + 6x 16 to ... + 6x 32 to ...
   15
   16
   24
   32
   YUY2
   YV12
   For each effect ! */
static blit_func_p effect_funcs[] = {
   /* normal */
   blit_normal_15_15_direct,
   blit_normal_16_16, /* Just use the 16 bpp src versions, since we need */
   blit_normal_16_24, /* to go through the lookup anyways. */
   blit_normal_16_32,
   blit_normal_16_YUY2,
   NULL, /* reserved for 16_YV12 */
   blit_normal_16_16, /* We use the lookup and don't do any calculations */
   blit_normal_16_16, /* with the result so these are the same. */
   blit_normal_16_24,
   blit_normal_16_32,
   blit_normal_16_YUY2,
   NULL, /* reserved for 16_YV12 */
   blit_normal_32_15_direct,
   blit_normal_32_16_direct,
   blit_normal_32_24_direct,
   blit_normal_32_32_direct,
   blit_normal_32_YUY2_direct,
   NULL, /* reserved for 32_YV12_direct */
   /* scale 2x */
   blit_scale2x_15_15_direct,
   blit_scale2x_16_16, /* Just use the 16 bpp src versions, since we need */
   blit_scale2x_16_24, /* to go through the lookup anyways. */
   blit_scale2x_16_32,
   blit_scale2x_16_YUY2,
   NULL, /* reserved for 16_YV12 */
   blit_scale2x_16_16, /* We use the lookup and don't do any calculations */
   blit_scale2x_16_16, /* with the result so these are the same. */
   blit_scale2x_16_24,
   blit_scale2x_16_32,
   blit_scale2x_16_YUY2,
   NULL, /* reserved for 16_YV12 */
   blit_scale2x_32_15_direct,
   blit_scale2x_32_16_direct,
   blit_scale2x_32_24_direct,
   blit_scale2x_32_32_direct,
   blit_scale2x_32_YUY2_direct,
   NULL, /* reserved for 32_YV12_direct */
   /* scan2 */
   blit_scan2_h_15_15_direct,
   blit_scan2_h_16_16, /* just use the 16 bpp src versions, since we need */
   blit_scan2_h_16_24, /* to go through the lookup anyways */
   blit_scan2_h_16_32,
   blit_scan2_h_16_YUY2,
   NULL, /* reserved for 16_YV12 */
   blit_scan2_h_16_15,
   blit_scan2_h_16_16,
   blit_scan2_h_16_24,
   blit_scan2_h_16_32,
   blit_scan2_h_16_YUY2,
   NULL, /* reserved for 16_YV12 */
   blit_scan2_h_32_15_direct,
   blit_scan2_h_32_16_direct,
   blit_scan2_h_32_24_direct,
   blit_scan2_h_32_32_direct,
   blit_scan2_h_32_YUY2_direct,
   NULL, /* reserved for 32_YV12_direct */
   /* rgbscan */
   blit_rgbscan_h_15_15_direct,
   blit_rgbscan_h_16_16, /* just use the 16 bpp src versions, since we need */
   blit_rgbscan_h_16_24, /* to go through the lookup anyways */
   blit_rgbscan_h_16_32,
   blit_rgbscan_h_16_YUY2,
   NULL, /* reserved for 16_YV12 */
   blit_rgbscan_h_16_15,
   blit_rgbscan_h_16_16,
   blit_rgbscan_h_16_24,
   blit_rgbscan_h_16_32,
   blit_rgbscan_h_16_YUY2,
   NULL, /* reserved for 16_YV12 */
   blit_rgbscan_h_32_15_direct,
   blit_rgbscan_h_32_16_direct,
   blit_rgbscan_h_32_24_direct,
   blit_rgbscan_h_32_32_direct,
   blit_rgbscan_h_32_YUY2_direct,
   NULL, /* reserved for 32_YV12_direct */
   /* scan3 */
   blit_scan3_h_15_15_direct,
   blit_scan3_h_16_16, /* just use the 16 bpp src versions, since we need */
   blit_scan3_h_16_24, /* to go through the lookup anyways */
   blit_scan3_h_16_32,
   blit_scan3_h_16_YUY2,
   NULL, /* reserved for 16_YV12 */
   blit_scan3_h_16_15,
   blit_scan3_h_16_16,
   blit_scan3_h_16_24,
   blit_scan3_h_16_32,
   blit_scan3_h_16_YUY2,
   NULL, /* reserved for 16_YV12 */
   blit_scan3_h_32_15_direct,
   blit_scan3_h_32_16_direct,
   blit_scan3_h_32_24_direct,
   blit_scan3_h_32_32_direct,
   blit_scan3_h_32_YUY2_direct,
   NULL, /* reserved for 32_YV12_direct */
   /* scan2 */
   blit_scan2_v_15_15_direct,
   blit_scan2_v_16_16, /* just use the 16 bpp src versions, since we need */
   blit_scan2_v_16_24, /* to go through the lookup anyways */
   blit_scan2_v_16_32,
   blit_scan2_v_16_YUY2,
   NULL, /* reserved for 16_YV12 */
   blit_scan2_v_16_15,
   blit_scan2_v_16_16,
   blit_scan2_v_16_24,
   blit_scan2_v_16_32,
   blit_scan2_v_16_YUY2,
   NULL, /* reserved for 16_YV12 */
   blit_scan2_v_32_15_direct,
   blit_scan2_v_32_16_direct,
   blit_scan2_v_32_24_direct,
   blit_scan2_v_32_32_direct,
   blit_scan2_v_32_YUY2_direct,
   NULL, /* reserved for 32_YV12_direct */
   /* rgbscan */
   blit_rgbscan_v_15_15_direct,
   blit_rgbscan_v_16_16, /* just use the 16 bpp src versions, since we need */
   blit_rgbscan_v_16_24, /* to go through the lookup anyways */
   blit_rgbscan_v_16_32,
   blit_rgbscan_v_16_YUY2,
   NULL, /* reserved for 16_YV12 */
   blit_rgbscan_v_16_15,
   blit_rgbscan_v_16_16,
   blit_rgbscan_v_16_24,
   blit_rgbscan_v_16_32,
   blit_rgbscan_v_16_YUY2,
   NULL, /* reserved for 16_YV12 */
   blit_rgbscan_v_32_15_direct,
   blit_rgbscan_v_32_16_direct,
   blit_rgbscan_v_32_24_direct,
   blit_rgbscan_v_32_32_direct,
   blit_rgbscan_v_32_YUY2_direct,
   NULL, /* reserved for 32_YV12_direct */
   /* scan3 */
   blit_scan3_v_15_15_direct,
   blit_scan3_v_16_16, /* just use the 16 bpp src versions, since we need */
   blit_scan3_v_16_24, /* to go through the lookup anyways */
   blit_scan3_v_16_32,
   blit_scan3_v_16_YUY2,
   NULL, /* reserved for 16_YV12 */
   blit_scan3_v_16_15,
   blit_scan3_v_16_16,
   blit_scan3_v_16_24,
   blit_scan3_v_16_32,
   blit_scan3_v_16_YUY2,
   NULL, /* reserved for 16_YV12 */
   blit_scan3_v_32_15_direct,
   blit_scan3_v_32_16_direct,
   blit_scan3_v_32_24_direct,
   blit_scan3_v_32_32_direct,
   blit_scan3_v_32_YUY2_direct,
   NULL, /* reserved for 32_YV12_direct */
};

#if 0
   /* lq2x */
   effect_lq2x_15_15_direct,
   effect_lq2x_16_16, /* just use the 16 bpp src versions, since we need */
   effect_lq2x_16_32, /* to go through the lookup anyways */
   effect_lq2x_16_YUY2,
   NULL, /* reserved for 16_YV12 */
   effect_lq2x_16_15,
   effect_lq2x_16_16,
   effect_lq2x_16_32,
   effect_lq2x_16_YUY2,
   NULL, /* reserved for 16_YV12 */
   effect_lq2x_32_15_direct,
   effect_lq2x_32_16_direct,
   effect_lq2x_32_32_direct,
   effect_lq2x_32_YUY2_direct,
   NULL, /* reserved for 32_YV12_direct */
   /* hq2x */
   effect_hq2x_15_15_direct,
   effect_hq2x_16_16, /* just use the 16 bpp src versions, since we need */
   effect_hq2x_16_32, /* to go through the lookup anyways */
   effect_hq2x_16_YUY2,
   NULL, /* reserved for 16_YV12 */
   effect_hq2x_16_15,
   effect_hq2x_16_16,
   effect_hq2x_16_32,
   effect_hq2x_16_YUY2,
   NULL, /* reserved for 16_YV12 */
   effect_hq2x_32_15_direct,
   effect_hq2x_32_16_direct,
   effect_hq2x_32_32_direct,
   effect_hq2x_32_YUY2_direct,
   NULL /* reserved for 32_YV12_direct */
   /* 6tap */
   effect_6tap_15_15_direct,
   effect_6tap_16_16, /* just use the 16 bpp src versions, since we need */
   effect_6tap_16_32, /* to go through the lookup anyways */
   effect_6tap_16_YUY2,
   NULL, /* reserved for 16_YV12 */
   effect_6tap_16_15,
   effect_6tap_16_16,
   effect_6tap_16_32,
   effect_6tap_16_YUY2,
   NULL, /* reserved for 16_YV12 */
   effect_6tap_32_15_direct,
   effect_6tap_32_16_direct,
   effect_6tap_32_32_direct,
   effect_6tap_32_YUY2_direct,
   NULL, /* reserved for 32_YV12_direct */
   /* fakescan */
   blit_fakescan_15_15_direct,
   blit_fakescan_16_16, /* just use the 16 bpp src versions, since we need */
   blit_fakescan_16_24, /* to go through the lookup anyways */
   blit_fakescan_16_32,
   blit_fakescan_16_YUY2,
   NULL, /* reserved for 16_YV12 */
   blit_fakescan_16_16, /* We use the lookup and don't do any calculations */
   blit_fakescan_16_16, /* with the result so these are the same. */
   blit_fakescan_16_24,
   blit_fakescan_16_32,
   blit_fakescan_16_YUY2,
   NULL, /* reserved for 16_YV12 */
   blit_fakescan_32_15_direct,
   blit_fakescan_32_16_direct,
   blit_fakescan_32_24_direct,
   blit_fakescan_32_32_direct,
   blit_fakescan_32_YUY2_direct,
   NULL /* reserved for 32_YV12_direct */
#endif

static void rotate_16_16(void *dst, struct mame_bitmap *bitmap, int y, struct rectangle *bounds);
static void rotate_32_32(void *dst, struct mame_bitmap *bitmap, int y, struct rectangle *bounds);

/* Functions

   Check if widthscale, heightscale and yarbsize are compatible with
   the choisen effect, if not update them so that they are. */
int sysdep_display_check_effect_params(
  struct sysdep_display_open_params *params)
{
  if ((params->effect < 0) || (params->effect > SYSDEP_DISPLAY_EFFECT_LAST))
    return 1;

  /* Can we do effects? */  
  if (!(sysdep_display_properties.mode_info[params->video_mode] &
        SYSDEP_DISPLAY_EFFECTS))
    params->effect = 0;

  /* Adjust widthscale */ 
  if (params->widthscale <
      sysdep_display_effect_properties[params->effect].min_widthscale)
  {
    params->widthscale =
      sysdep_display_effect_properties[params->effect].
        min_widthscale;
  }
  else if (params->widthscale >
           sysdep_display_effect_properties[params->effect].max_widthscale)
  {
    params->widthscale =
      sysdep_display_effect_properties[params->effect].max_widthscale;
  }
  
  /* Adjust heightscale */ 
  if (params->heightscale <
      sysdep_display_effect_properties[params->effect].min_heightscale)
  {
    params->heightscale =
      sysdep_display_effect_properties[params->effect].min_heightscale;
  }
  else if(params->heightscale >
          sysdep_display_effect_properties[params->effect].max_heightscale)
  {
    params->heightscale =
      sysdep_display_effect_properties[params->effect].max_heightscale;
  }
  
  if (params->effect < SYSDEP_DISPLAY_EFFECT_SCAN_V)
    params->yarbsize = 0;
  
  return 0;
}

/* called from sysdep_display_open;
 * returns a suitable blitfunctions and allocates the nescesarry buffers.
 *
 * The caller should call sysdep_display_effect_close() on failure and when
 * done, to free (partly) allocated buffers */
blit_func_p sysdep_display_effect_open(void)
{
  const char *display_name[EFFECT_COLOR_FORMATS] = {
    "RGB 555",
    "RGB 565",
    "RGB 888 (24bpp)",
    "RGB 888 (32bpp)",
    "YUY2",
    "YV12"
  };
  int effect_index = EFFECT_UNKNOWN;
  int need_rgb888_lookup = 0;

  /* FIXME only allocate if needed and of the right size */
  if (!(effect_dbbuf = malloc(sysdep_display_params.max_width*sysdep_display_params.widthscale*sysdep_display_params.heightscale*4)))
  {
    fprintf(stderr, "Error: couldnot allocate memory\n");
    return NULL;
  }
  memset(effect_dbbuf, sysdep_display_params.max_width*sysdep_display_params.widthscale*sysdep_display_params.heightscale*4, 0);

  switch(sysdep_display_properties.palette_info.fourcc_format)
  {
    case FOURCC_YUY2:
      effect_index = EFFECT_YUY2;
      break;
    case FOURCC_YV12:
      effect_index = EFFECT_YV12;
      break;
    case 0:
      if ( (sysdep_display_properties.palette_info.bpp == 16) &&
           (sysdep_display_properties.palette_info.red_mask   == (0x1F << 10)) &&
           (sysdep_display_properties.palette_info.green_mask == (0x1F <<  5)) &&
           (sysdep_display_properties.palette_info.blue_mask  == (0x1F      )))
        effect_index = EFFECT_15;
      if ( (sysdep_display_properties.palette_info.bpp == 16) &&
           (sysdep_display_properties.palette_info.red_mask   == (0x1F << 11)) &&
           (sysdep_display_properties.palette_info.green_mask == (0x3F <<  5)) &&
           (sysdep_display_properties.palette_info.blue_mask  == (0x1F      )))
        effect_index = EFFECT_16;
      if ( (sysdep_display_properties.palette_info.bpp == 24) &&
           (sysdep_display_properties.palette_info.red_mask   == (0xFF << 16)) &&
           (sysdep_display_properties.palette_info.green_mask == (0xFF <<  8)) &&
           (sysdep_display_properties.palette_info.blue_mask  == (0xFF      )))
        effect_index = EFFECT_24;
      if ( (sysdep_display_properties.palette_info.bpp == 32) &&
           (sysdep_display_properties.palette_info.red_mask   == (0xFF << 16)) &&
           (sysdep_display_properties.palette_info.green_mask == (0xFF <<  8)) &&
           (sysdep_display_properties.palette_info.blue_mask  == (0xFF      )))
        effect_index = EFFECT_32;
  }

  if (effect_index == EFFECT_UNKNOWN)
  {
    if ((sysdep_display_properties.palette_info.fourcc_format == 0) &&
        (sysdep_display_properties.palette_info.bpp == 16) &&
        (sysdep_display_params.depth <= 16))
    {
      if (sysdep_display_params.effect)
      {
        fprintf(stderr, "Warning: Your current color format is not supported by the effect code, disabling effects\n");
        sysdep_display_params.effect = 0;
      }
      effect_index = EFFECT_16;
    }
    else
    {
      fprintf(stderr, "Error: the required colordepth is not supported with your display settings\n");
      return NULL;
    }
  }
  effect_index += (sysdep_display_params.depth / 16) * EFFECT_COLOR_FORMATS;
  effect_index += sysdep_display_params.effect * 3 * EFFECT_COLOR_FORMATS;

  /* report our results to the user */
  if (effect_funcs[effect_index])
  {
    fprintf(stderr,
      "Initialized %s: bitmap depth = %d, color format = %s\n",
      sysdep_display_effect_properties[sysdep_display_params.effect].name,
      sysdep_display_params.depth, display_name[effect_index%EFFECT_COLOR_FORMATS]);
  }
  else
  {
    fprintf(stderr,
      "Warning effect %s is not supported with color format %s, disabling effects\n",
      sysdep_display_effect_properties[sysdep_display_params.effect].name,
      display_name[effect_index%EFFECT_COLOR_FORMATS]);
    sysdep_display_params.effect = 0;
    effect_index %= EFFECT_COLOR_FORMATS*3;
  }

  if (sysdep_display_params.orientation)
  {
    switch (sysdep_display_params.depth) {
    case 15:
    case 16:
      rotate_func = rotate_16_16;
      break;
    case 32:
      rotate_func = rotate_32_32;
      break;
    }

    /* add safety of +- 16 bytes, since some effects assume that this
       is present and otherwise segfault */
    if (!(rotate_dbbuf0 = calloc(sysdep_display_params.max_width*((sysdep_display_params.depth+1)/8) + 32, sizeof(char))))
    {
      fprintf(stderr, "Error: couldnot allocate memory\n");
      return NULL;
    }
    rotate_dbbuf0 += 16;

    if ((sysdep_display_params.effect == SYSDEP_DISPLAY_EFFECT_SCALE2X) ||
        (sysdep_display_params.effect == SYSDEP_DISPLAY_EFFECT_HQ2X)    ||
        (sysdep_display_params.effect == SYSDEP_DISPLAY_EFFECT_LQ2X)) {
      if (!(rotate_dbbuf1 = calloc(sysdep_display_params.max_width*((sysdep_display_params.depth+1)/8) + 32, sizeof(char))))
      {
        fprintf(stderr, "Error: couldnot allocate memory\n");
        return NULL;
      }
      rotate_dbbuf1 += 16;
      if (!(rotate_dbbuf2 = calloc(sysdep_display_params.max_width*((sysdep_display_params.depth+1)/8) + 32, sizeof(char))))
      {
        fprintf(stderr, "Error: couldnot allocate memory\n");
        return NULL;
      }
      rotate_dbbuf2 += 16;
    }
  }

  /* Effect specific initialiasations */
  switch (sysdep_display_params.effect)
  {
    case SYSDEP_DISPLAY_EFFECT_6TAP2X:
      _6tap2x_buf0 = calloc(sysdep_display_params.max_width*8, sizeof(char));
      _6tap2x_buf1 = calloc(sysdep_display_params.max_width*8, sizeof(char));
      _6tap2x_buf2 = calloc(sysdep_display_params.max_width*8, sizeof(char));
      _6tap2x_buf3 = calloc(sysdep_display_params.max_width*8, sizeof(char));
      _6tap2x_buf4 = calloc(sysdep_display_params.max_width*8, sizeof(char));
      _6tap2x_buf5 = calloc(sysdep_display_params.max_width*8, sizeof(char));
      if (!_6tap2x_buf0 || !_6tap2x_buf1 || !_6tap2x_buf2 || !_6tap2x_buf3 ||
          !_6tap2x_buf4 || !_6tap2x_buf5 )
      {
        fprintf(stderr, "Error: couldnot allocate memory\n");
        return NULL;
      }
      if(sysdep_display_params.depth == 16)
        need_rgb888_lookup = 1;
      break;
    case SYSDEP_DISPLAY_EFFECT_RGBSCAN_H:
    case SYSDEP_DISPLAY_EFFECT_RGBSCAN_V:
      if((effect_index%EFFECT_COLOR_FORMATS) == EFFECT_YUY2)
        need_rgb888_lookup = 1;
      break;
/*  case SYSDEP_DISPLAY_EFFECT_FAKESCAN:
      sysdep_display_driver_clear_buffer(); */
      break;
  }
  
  if (need_rgb888_lookup)
  {
    /* HACK: we need the palette lookup table to be 888 rgb, this means
       that the lookup table won't be usable for normal blitting anymore
       but that is not a problem, since we're not doing normal blitting,
       we do need to restore it on close though! */
    orig_palette_info = sysdep_display_properties.palette_info;
    orig_palette_info_saved = 1;
    sysdep_display_properties.palette_info.fourcc_format = 0;
    sysdep_display_properties.palette_info.red_mask   = 0x00FF0000;
    sysdep_display_properties.palette_info.green_mask = 0x0000FF00;
    sysdep_display_properties.palette_info.blue_mask  = 0x000000FF;
  }
  return effect_funcs[effect_index];
}

void sysdep_display_effect_close(void)
{
  /* if we modifified it then restore palette_info */
  if (orig_palette_info_saved)
  {
    sysdep_display_properties.palette_info = orig_palette_info;
    orig_palette_info_saved = 0;
  }
  
  if (effect_dbbuf)
  {
    free(effect_dbbuf);
    effect_dbbuf = NULL;
  }

  /* there is a safety of +- 16 bytes, since some effects assume that this
     is present and otherwise segfault */
  if (rotate_dbbuf0)
  {
     rotate_dbbuf0 -= 16;
     free(rotate_dbbuf0);
     rotate_dbbuf0 = NULL;
  }
  if (rotate_dbbuf1)
  {
     rotate_dbbuf1 -= 16;
     free(rotate_dbbuf1);
     rotate_dbbuf1 = NULL;
  }
  if (rotate_dbbuf2)
  {
     rotate_dbbuf2 -= 16;
     free(rotate_dbbuf2);
     rotate_dbbuf2 = NULL;
  }

  if (_6tap2x_buf0)
  {
    free(_6tap2x_buf0);
    _6tap2x_buf0 = NULL;
  }
  if (_6tap2x_buf1)
  {
    free(_6tap2x_buf1);
    _6tap2x_buf1 = NULL;
  }
  if (_6tap2x_buf2)
  {
    free(_6tap2x_buf2);
    _6tap2x_buf2 = NULL;
  }
  if (_6tap2x_buf3)
  {
    free(_6tap2x_buf3);
    _6tap2x_buf3 = NULL;
  }
  if (_6tap2x_buf4)
  {
    free(_6tap2x_buf4);
    _6tap2x_buf4 = NULL;
  }
  if (_6tap2x_buf5)
  {
    free(_6tap2x_buf5);
    _6tap2x_buf5 = NULL;
  }
}

/**********************************
 * rotate
 **********************************/
static void rotate_16_16(void *dst, struct mame_bitmap *bitmap, int y, struct rectangle *bounds)
{
  int x;
  unsigned short * u16dst = (unsigned short *)dst;

  if ((sysdep_display_params.orientation & SYSDEP_DISPLAY_SWAPXY)) {
    if ((sysdep_display_params.orientation & SYSDEP_DISPLAY_FLIPX) && (sysdep_display_params.orientation & SYSDEP_DISPLAY_FLIPY))
      for (x = bounds->min_x; x < bounds->max_x; x++)
        u16dst[x-bounds->min_x] = ((unsigned short *)bitmap->line[bitmap->height - x - 1])[bitmap->width - y - 1];
    else if ((sysdep_display_params.orientation & SYSDEP_DISPLAY_FLIPX))
      for (x = bounds->min_x; x < bounds->max_x; x++)
        u16dst[x-bounds->min_x] = ((unsigned short *)bitmap->line[bitmap->height - x - 1])[y];
    else if ((sysdep_display_params.orientation & SYSDEP_DISPLAY_FLIPY))
      for (x = bounds->min_x; x < bounds->max_x; x++)
        u16dst[x-bounds->min_x] = ((unsigned short *)bitmap->line[x])[bitmap->width - y - 1];
    else
      for (x = bounds->min_x; x < bounds->max_x; x++)
        u16dst[x-bounds->min_x] = ((unsigned short *)bitmap->line[x])[y];
  } else if ((sysdep_display_params.orientation & SYSDEP_DISPLAY_FLIPX) && (sysdep_display_params.orientation & SYSDEP_DISPLAY_FLIPY))
    for (x = bounds->min_x; x < bounds->max_x; x++)
      u16dst[x-bounds->min_x] = ((unsigned short *)bitmap->line[bitmap->height - y - 1])[bitmap->width - x - 1];
       else if ((sysdep_display_params.orientation & SYSDEP_DISPLAY_FLIPX))
         for (x = bounds->min_x; x < bounds->max_x; x++)
           u16dst[x-bounds->min_x] = ((unsigned short *)bitmap->line[y])[bitmap->width - x - 1];
       else if ((sysdep_display_params.orientation & SYSDEP_DISPLAY_FLIPY))
         for (x = bounds->min_x; x < bounds->max_x; x++)
           u16dst[x-bounds->min_x] = ((unsigned short *)bitmap->line[bitmap->height - y -1])[x];
}

static void rotate_32_32(void *dst, struct mame_bitmap *bitmap, int y, struct rectangle *bounds)
{
  int x;
  unsigned int * u32dst = (unsigned int *)dst;

  if ((sysdep_display_params.orientation & SYSDEP_DISPLAY_SWAPXY)) {
    if ((sysdep_display_params.orientation & SYSDEP_DISPLAY_FLIPX) && (sysdep_display_params.orientation & SYSDEP_DISPLAY_FLIPY))
      for (x = bounds->min_x; x < bounds->max_x; x++)
        u32dst[x-bounds->min_x] = ((unsigned int *)bitmap->line[bitmap->height - x - 1])[bitmap->width - y - 1];
    else if ((sysdep_display_params.orientation & SYSDEP_DISPLAY_FLIPX))
      for (x = bounds->min_x; x < bounds->max_x; x++)
        u32dst[x-bounds->min_x] = ((unsigned int *)bitmap->line[bitmap->height - x - 1])[y];
    else if ((sysdep_display_params.orientation & SYSDEP_DISPLAY_FLIPY))
      for (x = bounds->min_x; x < bounds->max_x; x++)
        u32dst[x-bounds->min_x] = ((unsigned int *)bitmap->line[x])[bitmap->width - y - 1];
    else
      for (x = bounds->min_x; x < bounds->max_x; x++)
        u32dst[x-bounds->min_x] = ((unsigned int *)bitmap->line[x])[y];
  } else if ((sysdep_display_params.orientation & SYSDEP_DISPLAY_FLIPX) && (sysdep_display_params.orientation & SYSDEP_DISPLAY_FLIPY))
    for (x = bounds->min_x; x < bounds->max_x; x++)
      u32dst[x-bounds->min_x] = ((unsigned int *)bitmap->line[bitmap->height - y - 1])[bitmap->width - x - 1];
       else if ((sysdep_display_params.orientation & SYSDEP_DISPLAY_FLIPX))
         for (x = bounds->min_x; x < bounds->max_x; x++)
           u32dst[x-bounds->min_x] = ((unsigned int *)bitmap->line[y])[bitmap->width - x - 1];
       else if ((sysdep_display_params.orientation & SYSDEP_DISPLAY_FLIPY))
         for (x = bounds->min_x; x < bounds->max_x; x++)
           u32dst[x-bounds->min_x] = ((unsigned int *)bitmap->line[bitmap->height - y -1])[x];
}



#if 0
/* These can't be moved to effect_funcs.c because they
   use inlined render functions */

/* high quality 2x scaling effect, see effect_renderers for the
   real stuff */
void effect_hq2x_16_YUY2
    (void *dst0, void *dst1,
    const void *src0, const void *src1, const void *src2,
    unsigned count, unsigned int *u32lookup)
{
  unsigned int *u32dst0   = (unsigned int *)dst0;
  unsigned int *u32dst1   = (unsigned int *)dst1;
  unsigned short *u16src0 = (unsigned short *)src0;
  unsigned short *u16src1 = (unsigned short *)src1;
  unsigned short *u16src2 = (unsigned short *)src2;
  INT32 y,y2,u,v;
  unsigned int p1[2], p2[2];
  unsigned int w[9];

  w[1] = u32lookup[u16src0[-1]];
  w[2] = u32lookup[u16src0[ 0]];
  w[4] = u32lookup[u16src1[-1]];
  w[5] = u32lookup[u16src1[ 0]];
  w[7] = u32lookup[u16src2[-1]];
  w[8] = u32lookup[u16src2[ 0]];

  w[1] = (w[1]>>24) | (w[1]<<8);
  w[2] = (w[2]>>24) | (w[2]<<8);
  w[4] = (w[4]>>24) | (w[4]<<8);
  w[5] = (w[5]>>24) | (w[5]<<8);
  w[7] = (w[7]>>24) | (w[7]<<8);
  w[8] = (w[8]>>24) | (w[8]<<8);

  while (count--)
  {

    /*
     *   +----+----+----+
     *   |    |    |    |
     *   | w0 | w1 | w2 |
     *   +----+----+----+
     *   |    |    |    |
     *   | w3 | w4 | w5 |
     *   +----+----+----+
     *   |    |    |    |
     *   | w6 | w7 | w8 |
     *   +----+----+----+
     */

    w[0] = w[1];
    w[1] = w[2];
    w[2] = u32lookup[u16src0[ 1]];
    w[2] = (w[2]>>24) | (w[2]<<8);
    w[3] = w[4];
    w[4] = w[5];
    w[5] = u32lookup[u16src1[ 1]];
    w[5] = (w[5]>>24) | (w[5]<<8);
    w[6] = w[7];
    w[7] = w[8];
    w[8] = u32lookup[u16src2[ 1]];
    w[8] = (w[8]>>24) | (w[8]<<8);

    hq2x_YUY2( p1, p2, w );

    ++u16src0;
    ++u16src1;
    ++u16src2;
    y=p1[0];
    y2=p1[1];
    u=(((y&0xff0000)+(y2&0xff0000))>>9)&0xff00;
    v=(((y&0xff)+(y2&0xff))<<23)&0xff000000;
    y=(y&0xff00)>>8;
    y2=(y2<<8)&0xff0000;
    *u32dst0++=y|y2|u|v;

    y=p2[0];
    y2=p2[1];
    u=(((y&0xff0000)+(y2&0xff0000))>>9)&0xff00;
    v=(((y&0xff)+(y2&0xff))<<23)&0xff000000;
    y=(y&0xff00)>>8;
    y2=(y2<<8)&0xff0000;
    *u32dst1++=y|y2|u|v;
  }
}


void effect_hq2x_32_YUY2_direct
    (void *dst0, void *dst1,
    const void *src0, const void *src1, const void *src2,
    unsigned count, unsigned int *u32lookup)
{
  unsigned int *u32dst0   = (unsigned int *)dst0;
  unsigned int *u32dst1   = (unsigned int *)dst1;
  unsigned int *u32src0 = (unsigned int *)src0;
  unsigned int *u32src1 = (unsigned int *)src1;
  unsigned int *u32src2 = (unsigned int *)src2;
  INT32 r,g,b,y,y2,u,v;
  unsigned int p1[2],p2[2];
  unsigned int w[9];

  w[1]=u32src0[-1];
  r=RMASK32(w[1]); r>>=16; g=GMASK32(w[1]);  g>>=8; b=BMASK32(w[1]);
  y = (( 9836*r + 19310*g + 3750*b ) >> 7)&0xff00;
  u = ((( -5527*r - 10921*g + 16448*b ) <<1) + 0x800000)&0xff0000;
  v = ((( 16448*r - 13783*g - 2665*b ) >> 15) + 128)&0xff;
  w[1]=y|u|v;

  w[2]=u32src0[0];
  r=RMASK32(w[2]); r>>=16; g=GMASK32(w[2]);  g>>=8; b=BMASK32(w[2]);
  y = (( 9836*r + 19310*g + 3750*b ) >> 7)&0xff00;
  u = ((( -5527*r - 10921*g + 16448*b ) <<1) + 0x800000)&0xff0000;
  v = ((( 16448*r - 13783*g - 2665*b ) >> 15) + 128)&0xff;
  w[2]=y|u|v;

  w[4]=u32src1[-1];
  r=RMASK32(w[4]); r>>=16; g=GMASK32(w[4]);  g>>=8; b=BMASK32(w[4]);
  y = (( 9836*r + 19310*g + 3750*b ) >> 7)&0xff00;
  u = ((( -5527*r - 10921*g + 16448*b ) <<1) + 0x800000)&0xff0000;
  v = ((( 16448*r - 13783*g - 2665*b ) >> 15) + 128)&0xff;
  w[4]=y|u|v;

  w[5]=u32src1[0];
  r=RMASK32(w[5]); r>>=16; g=GMASK32(w[5]);  g>>=8; b=BMASK32(w[5]);
  y = (( 9836*r + 19310*g + 3750*b ) >> 7)&0xff00;
  u = ((( -5527*r - 10921*g + 16448*b ) <<1) + 0x800000)&0xff0000;
  v = ((( 16448*r - 13783*g - 2665*b ) >> 15) + 128)&0xff;
  w[5]=y|u|v;

  w[7]=u32src2[-1];
  r=RMASK32(w[7]); r>>=16; g=GMASK32(w[7]);  g>>=8; b=BMASK32(w[7]);
  y = (( 9836*r + 19310*g + 3750*b ) >> 7)&0xff00;
  u = ((( -5527*r - 10921*g + 16448*b ) <<1) + 0x800000)&0xff0000;
  v = ((( 16448*r - 13783*g - 2665*b ) >> 15) + 128)&0xff;
  w[7]=y|u|v;

  w[8]=u32src2[0];
  r=RMASK32(w[8]); r>>=16; g=GMASK32(w[8]);  g>>=8; b=BMASK32(w[8]);
  y = (( 9836*r + 19310*g + 3750*b ) >> 7)&0xff00;
  u = ((( -5527*r - 10921*g + 16448*b ) <<1) + 0x800000)&0xff0000;
  v = ((( 16448*r - 13783*g - 2665*b ) >> 15) + 128)&0xff;
  w[8]=y|u|v;

  while (count--)
  {

    /*
     *   +----+----+----+
     *   |    |    |    |
     *   | w0 | w1 | w2 |
     *   +----+----+----+
     *   |    |    |    |
     *   | w3 | w4 | w5 |
     *   +----+----+----+
     *   |    |    |    |
     *   | w6 | w7 | w8 |
     *   +----+----+----+
     */

    w[0] = w[1];
    w[1] = w[2];
    w[2] = u32src0[ 1];
    r = RMASK32(w[2]); r>>=16; g = GMASK32(w[2]);  g>>=8; b = BMASK32(w[2]);
    y = (( 9836*r + 19310*g + 3750*b ) >> 7)&0xff00;
    u = ((( -5527*r - 10921*g + 16448*b ) <<1) + 0x800000)&0xff0000;
    v = ((( 16448*r - 13783*g - 2665*b ) >> 15) + 128)&0xff;
    w[2]=y|u|v;

    w[3] = w[4];
    w[4] = w[5];
    w[5] = u32src1[ 1];
    r = RMASK32(w[5]); r>>=16; g = GMASK32(w[5]);  g>>=8; b = BMASK32(w[5]);
    y = (( 9836*r + 19310*g + 3750*b ) >> 7)&0xff00;
    u = ((( -5527*r - 10921*g + 16448*b ) <<1) + 0x800000)&0xff0000;
    v = ((( 16448*r - 13783*g - 2665*b ) >> 15) + 128)&0xff;
    w[5]=y|u|v;

    w[6] = w[7];
    w[7] = w[8];
    w[8] = u32src2[ 1];
    r = RMASK32(w[8]); r>>=16; g = GMASK32(w[8]);  g>>=8; b = BMASK32(w[8]);
    y = (( 9836*r + 19310*g + 3750*b ) >> 7)&0xff00;
    u = ((( -5527*r - 10921*g + 16448*b ) <<1) + 0x800000)&0xff0000;
    v = ((( 16448*r - 13783*g - 2665*b ) >> 15) + 128)&0xff;
    w[8]=y|u|v;

    hq2x_YUY2( &p1[0], &p2[0], w );

    ++u32src0;
    ++u32src1;
    ++u32src2;

    y=p1[0];
    y2=p1[1];
    u=(((y&0xff0000)+(y2&0xff0000))>>9)&0xff00;
    v=(((y&0xff)+(y2&0xff))<<23)&0xff000000;
    y=(y&0xff00)>>8;
    y2=(y2<<8)&0xff0000;
    *u32dst0++=y|y2|u|v;

    y=p2[0];
    y2=p2[1];
    u=(((y&0xff0000)+(y2&0xff0000))>>9)&0xff00;
    v=(((y&0xff)+(y2&0xff))<<23)&0xff000000;
    y=(y&0xff00)>>8;
    y2=(y2<<8)&0xff0000;
    *u32dst1++=y|y2|u|v;
  }
}

/* low quality 2x scaling effect, see effect_renderers for the
   real stuff */
void effect_lq2x_16_YUY2
    (void *dst0, void *dst1,
    const void *src0, const void *src1, const void *src2,
    unsigned count, unsigned int *u32lookup)
{
  unsigned int *u32dst0   = (unsigned int *)dst0;
  unsigned int *u32dst1   = (unsigned int *)dst1;
  unsigned short *u16src0 = (unsigned short *)src0;
  unsigned short *u16src1 = (unsigned short *)src1;
  unsigned short *u16src2 = (unsigned short *)src2;
  INT32 y,y2,u,v;
  unsigned int p1[2], p2[2];
  unsigned int w[9];

  w[1] = u32lookup[u16src0[-1]];
  w[2] = u32lookup[u16src0[ 0]];
  w[4] = u32lookup[u16src1[-1]];
  w[5] = u32lookup[u16src1[ 0]];
  w[7] = u32lookup[u16src2[-1]];
  w[8] = u32lookup[u16src2[ 0]];

  w[1] = (w[1]>>24) | (w[1]<<8);
  w[2] = (w[2]>>24) | (w[2]<<8);
  w[4] = (w[4]>>24) | (w[4]<<8);
  w[5] = (w[5]>>24) | (w[5]<<8);
  w[7] = (w[7]>>24) | (w[7]<<8);
  w[8] = (w[8]>>24) | (w[8]<<8);

  while (count--)
  {

    /*
     *   +----+----+----+
     *   |    |    |    |
     *   | w0 | w1 | w2 |
     *   +----+----+----+
     *   |    |    |    |
     *   | w3 | w4 | w5 |
     *   +----+----+----+
     *   |    |    |    |
     *   | w6 | w7 | w8 |
     *   +----+----+----+
     */

    w[0] = w[1];
    w[1] = w[2];
    w[2] = u32lookup[u16src0[ 1]];
    w[2] = (w[2]>>24) | (w[2]<<8);
    w[3] = w[4];
    w[4] = w[5];
    w[5] = u32lookup[u16src1[ 1]];
    w[5] = (w[5]>>24) | (w[5]<<8);
    w[6] = w[7];
    w[7] = w[8];
    w[8] = u32lookup[u16src2[ 1]];
    w[8] = (w[8]>>24) | (w[8]<<8);

    lq2x_32( p1, p2, w );

    ++u16src0;
    ++u16src1;
    ++u16src2;
    y=p1[0];
    y2=p1[1];
    u=(((y&0xff0000)+(y2&0xff0000))>>9)&0xff00;
    v=(((y&0xff)+(y2&0xff))<<23)&0xff000000;
    y=(y&0xff00)>>8;
    y2=(y2<<8)&0xff0000;
    *u32dst0++=y|y2|u|v;

    y=p2[0];
    y2=p2[1];
    u=(((y&0xff0000)+(y2&0xff0000))>>9)&0xff00;
    v=(((y&0xff)+(y2&0xff))<<23)&0xff000000;
    y=(y&0xff00)>>8;
    y2=(y2<<8)&0xff0000;
    *u32dst1++=y|y2|u|v;
  }
}

void effect_lq2x_32_YUY2_direct
    (void *dst0, void *dst1,
    const void *src0, const void *src1, const void *src2,
    unsigned count, unsigned int *u32lookup)
{
  unsigned int *u32dst0   = (unsigned int *)dst0;
  unsigned int *u32dst1   = (unsigned int *)dst1;
  unsigned int *u32src0 = (unsigned int *)src0;
  unsigned int *u32src1 = (unsigned int *)src1;
  unsigned int *u32src2 = (unsigned int *)src2;
  INT32 r,g,b,y,y2,u,v;
  unsigned int p1[2],p2[2];
  unsigned int w[9];

  w[1]=u32src0[-1];
  r = RMASK32(w[1]); r>>=16; g = GMASK32(w[1]);  g>>=8; b = BMASK32(w[1]);
  y = (( 9836*r + 19310*g + 3750*b ) >> 7)&0xff00;
  u = ((( -5527*r - 10921*g + 16448*b ) <<1) + 0x800000)&0xff0000;
  v = ((( 16448*r - 13783*g - 2665*b ) >> 15) + 128)&0xff;
  w[1]=y|u|v;

  w[2]=u32src0[0];
  r = RMASK32(w[2]); r>>=16; g = GMASK32(w[2]);  g>>=8; b = BMASK32(w[2]);
  y = (( 9836*r + 19310*g + 3750*b ) >> 7)&0xff00;
  u = ((( -5527*r - 10921*g + 16448*b ) <<1) + 0x800000)&0xff0000;
  v = ((( 16448*r - 13783*g - 2665*b ) >> 15) + 128)&0xff;
  w[2]=y|u|v;

  w[4]=u32src1[-1];
  r = RMASK32(w[4]); r>>=16; g = GMASK32(w[4]);  g>>=8; b = BMASK32(w[4]);
  y = (( 9836*r + 19310*g + 3750*b ) >> 7)&0xff00;
  u = ((( -5527*r - 10921*g + 16448*b ) <<1) + 0x800000)&0xff0000;
  v = ((( 16448*r - 13783*g - 2665*b ) >> 15) + 128)&0xff;
  w[4]=y|u|v;

  w[5]=u32src1[0];
  r = RMASK32(w[5]); r>>=16; g = GMASK32(w[5]);  g>>=8; b = BMASK32(w[5]);
  y = (( 9836*r + 19310*g + 3750*b ) >> 7)&0xff00;
  u = ((( -5527*r - 10921*g + 16448*b ) <<1) + 0x800000)&0xff0000;
  v = ((( 16448*r - 13783*g - 2665*b ) >> 15) + 128)&0xff;
  w[5]=y|u|v;

  w[7]=u32src2[-1];
  r = RMASK32(w[7]); r>>=16; g = GMASK32(w[7]);  g>>=8; b = BMASK32(w[7]);
  y = (( 9836*r + 19310*g + 3750*b ) >> 7)&0xff00;
  u = ((( -5527*r - 10921*g + 16448*b ) <<1) + 0x800000)&0xff0000;
  v = ((( 16448*r - 13783*g - 2665*b ) >> 15) + 128)&0xff;
  w[7]=y|u|v;

  w[8]=u32src2[0];
  r = RMASK32(w[8]); r>>=16; g = GMASK32(w[8]);  g>>=8; b = BMASK32(w[8]);
  y = (( 9836*r + 19310*g + 3750*b ) >> 7)&0xff00;
  u = ((( -5527*r - 10921*g + 16448*b ) <<1) + 0x800000)&0xff0000;
  v = ((( 16448*r - 13783*g - 2665*b ) >> 15) + 128)&0xff;
  w[8]=y|u|v;

  while (count--)
  {

    /*
     *   +----+----+----+
     *   |    |    |    |
     *   | w0 | w1 | w2 |
     *   +----+----+----+
     *   |    |    |    |
     *   | w3 | w4 | w5 |
     *   +----+----+----+
     *   |    |    |    |
     *   | w6 | w7 | w8 |
     *   +----+----+----+
     */

    w[0] = w[1];
    w[1] = w[2];
    w[2] = u32src0[ 1];
    r = RMASK32(w[2]); r>>=16; g = GMASK32(w[2]);  g>>=8; b = BMASK32(w[2]);
    y = (( 9836*r + 19310*g + 3750*b ) >> 7)&0xff00;
    u = ((( -5527*r - 10921*g + 16448*b ) <<1) + 0x800000)&0xff0000;
    v = ((( 16448*r - 13783*g - 2665*b ) >> 15) + 128)&0xff;
    w[2]=y|u|v;

    w[3] = w[4];
    w[4] = w[5];
    w[5] = u32src1[ 1];
    r = RMASK32(w[5]); r>>=16; g = GMASK32(w[5]);  g>>=8; b = BMASK32(w[5]);
    y = (( 9836*r + 19310*g + 3750*b ) >> 7)&0xff00;
    u = ((( -5527*r - 10921*g + 16448*b ) <<1) + 0x800000)&0xff0000;
    v = ((( 16448*r - 13783*g - 2665*b ) >> 15) + 128)&0xff;
    w[5]=y|u|v;

    w[6] = w[7];
    w[7] = w[8];
    w[8] = u32src2[ 1];
    r = RMASK32(w[8]); r>>=16; g = GMASK32(w[8]);  g>>=8; b = BMASK32(w[8]);
    y = (( 9836*r + 19310*g + 3750*b ) >> 7)&0xff00;
    u = ((( -5527*r - 10921*g + 16448*b ) <<1) + 0x800000)&0xff0000;
    v = ((( 16448*r - 13783*g - 2665*b ) >> 15) + 128)&0xff;
    w[8]=y|u|v;

    lq2x_32( &p1[0], &p2[0], w );

    ++u32src0;
    ++u32src1;
    ++u32src2;

    y=p1[0];
    y2=p1[1];
    u=(((y&0xff0000)+(y2&0xff0000))>>9)&0xff00;
    v=(((y&0xff)+(y2&0xff))<<23)&0xff000000;
    y=(y&0xff00)>>8;
    y2=(y2<<8)&0xff0000;
    *u32dst0++=y|y2|u|v;

    y=p2[0];
    y2=p2[1];
    u=(((y&0xff0000)+(y2&0xff0000))>>9)&0xff00;
    v=(((y&0xff)+(y2&0xff))<<23)&0xff000000;
    y=(y&0xff00)>>8;
    y2=(y2<<8)&0xff0000;
    *u32dst1++=y|y2|u|v;
  }
}

#endif
