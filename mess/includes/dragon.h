#ifndef DRAGON_H
#define DRAGON_H

#include "vidhrdw/m6847.h"
#include "videomap.h"

#define COCO_CPU_SPEED_HZ		894886	/* 0.894886 MHz */
#define COCO_FRAMES_PER_SECOND	(COCO_CPU_SPEED_HZ / 57.0 / 263)
#define COCO_CPU_SPEED			(TIME_IN_HZ(COCO_CPU_SPEED_HZ))
#define COCO_TIMER_CMPCARRIER	(COCO_CPU_SPEED * 0.25)

#define COCO_DIP_ARTIFACTING		12
#define COCO3_DIP_MONITORTYPE		12
#define COCO3_DIP_MONITORTYPE_MASK	0x08

/* ----------------------------------------------------------------------- *
 * Backdoors into mess/vidhrdw/m6847.c                                     *
 * ----------------------------------------------------------------------- */

int internal_video_start_m6847(const struct m6847_init_params *params, const struct videomap_interface *videointf,
	int dirtyramsize);

void internal_m6847_frame_callback(struct videomap_framecallback_info *info, int offset, int border_top, int rows);

struct internal_m6847_linecallback_interface
{
	int width_factor;
	charproc_callback charproc;
	UINT16 (*calculate_artifact_color)(UINT16 metacolor, int artifact_mode);
	int (*setup_dynamic_artifact_palette)(int artifact_mode, UINT8 *bgcolor, UINT8 *fgcolor);
};

void internal_m6847_line_callback(struct videomap_linecallback_info *info, const UINT16 *metapalette,
	struct internal_m6847_linecallback_interface *intf);

UINT8 internal_m6847_charproc(UINT32 c, UINT16 *charpalette, const UINT16 *metapalette, int row, int skew);

int internal_m6847_getadjustedscanline(void);
void internal_m6847_vh_interrupt(int scanline, int rise_scanline, int fall_scanline);

/* ----------------------------------------------------------------------- *
 * from vidhrdw/dragon.c                                                   *
 * ----------------------------------------------------------------------- */

extern void coco3_ram_b1_w (offs_t offset, data8_t data);
extern void coco3_ram_b2_w (offs_t offset, data8_t data);
extern void coco3_ram_b3_w (offs_t offset, data8_t data);
extern void coco3_ram_b4_w (offs_t offset, data8_t data);
extern void coco3_ram_b5_w (offs_t offset, data8_t data);
extern void coco3_ram_b6_w (offs_t offset, data8_t data);
extern void coco3_ram_b7_w (offs_t offset, data8_t data);
extern void coco3_ram_b8_w (offs_t offset, data8_t data);
extern void coco3_ram_b9_w (offs_t offset, data8_t data);
extern void coco3_vh_sethires(int hires);

extern int video_start_dragon(void);
extern int video_start_coco(void);
extern int video_start_coco2b(void);
extern int video_start_coco3(void);
extern void video_update_coco3(struct mame_bitmap *bitmap, const struct rectangle *cliprect);

extern WRITE_HANDLER ( coco_ram_w );
extern READ_HANDLER ( coco3_gimevh_r );
extern WRITE_HANDLER ( coco3_gimevh_w );
extern WRITE_HANDLER ( coco3_palette_w );
extern void coco3_vh_blink(void);

/* ----------------------------------------------------------------------- *
 * from machine/dragon.c                                                   *
 * ----------------------------------------------------------------------- */

extern void machine_init_dragon32(void);
extern void machine_init_dragon64(void);
extern void machine_init_coco(void);
extern void machine_init_coco2(void);
extern void machine_init_coco3(void);
extern void machine_stop_coco(void);

extern void coco3_vh_interrupt(void);
extern int coco_cassette_init(int id);
extern int coco3_cassette_init(int id);
extern void coco_cassette_exit(int id);
extern int coco_rom_load(int id);
extern int coco3_rom_load(int id);
extern int coco_pak_load(int id);
extern int coco3_pak_load(int id);
extern READ_HANDLER ( dragon_mapped_irq_r );
extern READ_HANDLER ( coco3_mapped_irq_r );
extern READ_HANDLER ( coco3_mmu_r );
extern WRITE_HANDLER ( coco3_mmu_w );
extern READ_HANDLER ( coco3_gime_r );
extern WRITE_HANDLER ( coco3_gime_w );
extern READ_HANDLER ( coco_cartridge_r);
extern WRITE_HANDLER ( coco_cartridge_w );
extern READ_HANDLER ( coco3_cartridge_r);
extern WRITE_HANDLER ( coco3_cartridge_w );
extern int coco_floppy_init(int id);
extern void coco_floppy_exit(int id);
extern WRITE_HANDLER( coco_m6847_hs_w );
extern WRITE_HANDLER( coco_m6847_fs_w );
extern WRITE_HANDLER( coco3_m6847_hs_w );
extern WRITE_HANDLER( coco3_m6847_fs_w );
extern int coco3_mmu_translate(int bank, int offset);
extern int dragon_floppy_init(int id);
extern int dragon_floppy_id(int id);
extern void dragon_floppy_exit(int id);
extern int coco_vhd_init(int id);
extern void coco_vhd_exit(int id);
extern READ_HANDLER(coco_vhd_io_r);
extern WRITE_HANDLER(coco_vhd_io_w);
extern int coco_bitbanger_init (int id);
extern void coco_bitbanger_exit (int id);
extern void coco_bitbanger_output (int id, int data);
extern READ_HANDLER( coco_pia_1_r );
extern READ_HANDLER( coco3_pia_1_r );
extern void dragon_sound_update(void);

/* Returns whether a given piece of logical memory is contiguous or not */
extern int coco3_mmu_ismemorycontiguous(int logicaladdr, int len);

/* Reads logical memory into a buffer */
extern void coco3_mmu_readlogicalmemory(UINT8 *buffer, int logicaladdr, int len);

/* Translates a logical address to a physical address */
extern int coco3_mmu_translatelogicaladdr(int logicaladdr);

#define IO_FLOPPY_COCO \
	{\
		IO_FLOPPY,\
		4,\
		"dsk\0",\
		IO_RESET_NONE,\
		0,\
		dragon_floppy_init,\
		dragon_floppy_exit,\
        NULL,\
        NULL,\
        NULL,\
        NULL,\
        NULL,\
        NULL,\
        NULL,\
        NULL,\
        NULL \
    }

#define IO_CARTRIDGE_COCO(loadproc) \
	{\
		IO_CARTSLOT,\
		1,\
		"rom\0",\
		IO_RESET_ALL,\
        NULL,\
		loadproc,\
		NULL,\
        NULL,\
        NULL,\
        NULL,\
        NULL,\
        NULL,\
        NULL,\
        NULL,\
        NULL,\
        NULL\
    }

#define IO_SNAPSHOT_COCOPAK(loadproc) \
	{\
		IO_SNAPSHOT,\
		1,\
		"pak\0",\
		IO_RESET_ALL,\
        NULL,\
		loadproc,\
		NULL,\
        NULL,\
        NULL,\
        NULL,\
        NULL,\
        NULL,\
        NULL,\
        NULL,\
        NULL,\
        NULL\
    }

#define IO_BITBANGER IO_PRINTER

#define IO_BITBANGER_PORT								\
{														\
	IO_BITBANGER,				/* type */				\
	1,							/* count */				\
	"prn\0",					/* file extensions */	\
	IO_RESET_NONE,				/* reset depth */		\
	NULL,						/* id */				\
	coco_bitbanger_init,		/* init */				\
	coco_bitbanger_exit,		/* exit */				\
	NULL,						/* info */				\
	NULL,						/* open */				\
	NULL,						/* close */				\
	NULL,						/* status */			\
	NULL,						/* seek */				\
	NULL,						/* tell */				\
	NULL,						/* input */				\
	coco_bitbanger_output,		/* output */			\
	NULL,						/* input chunk */		\
	NULL						/* output chunk */		\
}

#define IO_VHD IO_HARDDISK

#define IO_VHD_PORT								\
{														\
	IO_VHD,						/* type */				\
	1,							/* count */				\
	"vhd\0",					/* file extensions */	\
	IO_RESET_NONE,				/* reset depth */		\
	NULL,						/* id */				\
	coco_vhd_init,				/* init */				\
	coco_vhd_exit,				/* exit */				\
	NULL,						/* info */				\
	NULL,						/* open */				\
	NULL,						/* close */				\
	NULL,						/* status */			\
	NULL,						/* seek */				\
	NULL,						/* tell */				\
	NULL,						/* input */				\
	NULL,						/* output */			\
	NULL,						/* input chunk */		\
	NULL						/* output chunk */		\
}



#endif /* DRAGON_H */
