/***************************************************************************

	vidhrdw.c

	Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



unsigned char *tp84_videoram2;
unsigned char *tp84_colorram2;
static struct mame_bitmap *tmpbitmap2;
static unsigned char *dirtybuffer2;

unsigned char *tp84_scrollx;
unsigned char *tp84_scrolly;

int col0;

/*
sprites are multiplexed, so we have to buffer the spriteram
scanline by scanline.
*/
static unsigned char *sprite_mux_buffer;
static int scanline;



static struct rectangle topvisiblearea =
{
	0*8, 2*8-1,
	2*8, 30*8-1
};
static struct rectangle bottomvisiblearea =
{
	30*8, 32*8-1,
	2*8, 30*8-1
};



/*
-The colortable is divided in 2 part:
 -The characters colors
 -The sprites colors

-The characters colors are indexed like this:
 -2 bits from the characters
 -4 bits from the attribute in colorram
 -2 bits from col0 (d3-d4)
 -3 bits from col0 (d0-d1-d2)
-So, there is 2048 bytes for the characters

-The sprites colors are indexed like this:
 -4 bits from the sprites (16 colors)
 -4 bits from the attribute of the sprites
 -3 bits from col0 (d0-d1-d2)
-So, there is 2048 bytes for the sprites

*/
/*
	 The RGB signals are generated by 3 proms 256X4 (prom 2C, 2D and 1E)
		The resistors values are:
			1K  ohm
			470 ohm
			220 ohm
			100 ohm
*/
PALETTE_INIT( tp84 )
{
	int i;
	#define TOTAL_COLORS(gfxn) (Machine->gfx[gfxn]->total_colors * Machine->gfx[gfxn]->color_granularity)
	#define COLOR(gfxn,offs) (colortable[Machine->drv->gfxdecodeinfo[gfxn].color_codes_start + offs])


	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		int bit0,bit1,bit2,bit3,r,g,b;

		/* red component */
		bit0 = (color_prom[0] >> 0) & 0x01;
		bit1 = (color_prom[0] >> 1) & 0x01;
		bit2 = (color_prom[0] >> 2) & 0x01;
		bit3 = (color_prom[0] >> 3) & 0x01;
		r = 0x0e * bit0 + 0x1f * bit1 + 0x42 * bit2 + 0x90 * bit3;
		/* green component */
		bit0 = (color_prom[Machine->drv->total_colors] >> 0) & 0x01;
		bit1 = (color_prom[Machine->drv->total_colors] >> 1) & 0x01;
		bit2 = (color_prom[Machine->drv->total_colors] >> 2) & 0x01;
		bit3 = (color_prom[Machine->drv->total_colors] >> 3) & 0x01;
		g = 0x0e * bit0 + 0x1f * bit1 + 0x42 * bit2 + 0x90 * bit3;
		/* blue component */
		bit0 = (color_prom[2*Machine->drv->total_colors] >> 0) & 0x01;
		bit1 = (color_prom[2*Machine->drv->total_colors] >> 1) & 0x01;
		bit2 = (color_prom[2*Machine->drv->total_colors] >> 2) & 0x01;
		bit3 = (color_prom[2*Machine->drv->total_colors] >> 3) & 0x01;
		b = 0x0e * bit0 + 0x1f * bit1 + 0x42 * bit2 + 0x90 * bit3;

		palette_set_color(i,r,g,b);

		color_prom++;
	}

	color_prom += 2*Machine->drv->total_colors;
	/* color_prom now points to the beginning of the lookup table */


	/* characters use colors 128-255 */
	for (i = 0;i < TOTAL_COLORS(0)/8;i++)
	{
		int j;


		for (j = 0;j < 8;j++)
			COLOR(0,i+256*j) = *color_prom + 128 + 16*j;

		color_prom++;
	}

	/* sprites use colors 0-127 */
	for (i = 0;i < TOTAL_COLORS(1)/8;i++)
	{
		int j;


		for (j = 0;j < 8;j++)
		{
			if (*color_prom) COLOR(1,i+256*j) = *color_prom + 16*j;
			else COLOR(1,i+256*j) = 0;	/* preserve transparency */
		}

		color_prom++;
	}
}



/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
VIDEO_START( tp84 )
{
	if (video_start_generic() != 0)
		return 1;

	if ((dirtybuffer2 = auto_malloc(videoram_size)) == 0)
		return 1;
	memset(dirtybuffer2,1,videoram_size);

	if ((tmpbitmap2 = auto_bitmap_alloc(Machine->drv->screen_width,Machine->drv->screen_height)) == 0)
		return 1;

	sprite_mux_buffer = auto_malloc(256 * spriteram_size);

	if (!sprite_mux_buffer)
		return 1;

	return 0;
}



WRITE_HANDLER( tp84_videoram2_w )
{
	if (tp84_videoram2[offset] != data)
	{
		dirtybuffer2[offset] = 1;

		tp84_videoram2[offset] = data;
	}
}



WRITE_HANDLER( tp84_colorram2_w )
{
	if (tp84_colorram2[offset] != data)
	{
		dirtybuffer2[offset] = 1;

		tp84_colorram2[offset] = data;
	}
}



/*****
  col0 is a register to index the color Proms
*****/
WRITE_HANDLER( tp84_col0_w )
{
	if(col0 != data)
	{
		col0 = data;

		memset(dirtybuffer,1,videoram_size);
		memset(dirtybuffer2,1,videoram_size);
	}
}



/* Return the current video scan line */
READ_HANDLER( tp84_scanline_r )
{
	return scanline;
}


/***************************************************************************

  Display refresh

***************************************************************************/

static void draw_sprites(struct mame_bitmap *bitmap)
{
	const struct GfxElement *gfx = Machine->gfx[1];
	struct rectangle clip = Machine->visible_area;
	int offs;
	int line;
	int coloffset = ((col0&0x07) << 4);

	for (line = 0;line < 256;line++)
	{
		if (line >= Machine->visible_area.min_y && line <= Machine->visible_area.max_y)
		{
			unsigned char *sr;

			sr = sprite_mux_buffer + line * spriteram_size;
			clip.min_y = clip.max_y = line;

			for (offs = spriteram_size - 4;offs >= 0;offs -= 4)
			{
				int code,color,sx,sy,flipx,flipy;

				sx = sr[offs];
				sy = 240 - sr[offs + 3];

				if (sy > line-16 && sy <= line)
				{
					code = sr[offs + 1];
					color = (sr[offs + 2] & 0x0f) + coloffset;
					flipx = ~sr[offs + 2] & 0x40;
					flipy = sr[offs + 2] & 0x80;

					drawgfx(bitmap,gfx,
							code,
							color,
							flipx,flipy,
							sx,sy,
							&clip,TRANSPARENCY_COLOR,0);
				}
			}
		}
	}
}


VIDEO_UPDATE( tp84 )
{
	int offs;
	int coloffset;


	coloffset = ((col0&0x18) << 1) + ((col0&0x07) << 6);

	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		if (dirtybuffer[offs])
		{
			int sx,sy;


			dirtybuffer[offs] = 0;

			sx = offs % 32;
			sy = offs / 32;

			drawgfx(tmpbitmap,Machine->gfx[0],
					videoram[offs] + ((colorram[offs] & 0x30) << 4),
					(colorram[offs] & 0x0f) + coloffset,
					colorram[offs] & 0x40,colorram[offs] & 0x80,
					8*sx,8*sy,
					0,TRANSPARENCY_NONE,0);
		}

		if (dirtybuffer2[offs])
		{
			int sx,sy;


			dirtybuffer2[offs] = 0;

			sx = offs % 32;
			sy = offs / 32;

		/* Skip the middle of the screen, this ram seem to be used as normal ram. */
			if (sx < 2 || sx >= 30)
				drawgfx(tmpbitmap2,Machine->gfx[0],
						tp84_videoram2[offs] + ((tp84_colorram2[offs] & 0x30) << 4),
						(tp84_colorram2[offs] & 0x0f) + coloffset,
						tp84_colorram2[offs] & 0x40,tp84_colorram2[offs] & 0x80,
						8*sx,8*sy,
						&Machine->visible_area,TRANSPARENCY_NONE,0);
		}
	}


	/* copy the temporary bitmap to the screen */
	{
		int scrollx,scrolly;


		scrollx = -*tp84_scrollx;
		scrolly = -*tp84_scrolly;

		copyscrollbitmap(bitmap,tmpbitmap,1,&scrollx,1,&scrolly,&Machine->visible_area,TRANSPARENCY_NONE,0);
	}

	draw_sprites(bitmap);

	/* Copy the frontmost playfield. */
	copybitmap(bitmap,tmpbitmap2,0,0,0,0,&topvisiblearea,TRANSPARENCY_NONE,0);
	copybitmap(bitmap,tmpbitmap2,0,0,0,0,&bottomvisiblearea,TRANSPARENCY_NONE,0);
}


INTERRUPT_GEN( tp84_6809_interrupt )
{
	scanline = 255 - cpu_getiloops();

	memcpy(sprite_mux_buffer + scanline * spriteram_size,spriteram,spriteram_size);

	if (scanline == 255)
		irq0_line_hold();
}
