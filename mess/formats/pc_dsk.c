/*********************************************************************

	formats/pc_dsk.c

	PC disk images

*********************************************************************/

#include <string.h>

#include "formats/pc_dsk.h"
#include "formats/basicdsk.h"

struct pc_disk_sizes
{
	UINT32 image_size;
	int sectors;
	int heads;
};



static struct pc_disk_sizes disk_sizes[] =
{
	{ 8*1*40*512,  8, 1},	/* 5 1/4 inch double density single sided */
	{ 8*2*40*512,  8, 2},	/* 5 1/4 inch double density */
	{ 9*1*40*512,  9, 1},	/* 5 1/4 inch double density single sided */
	{ 9*2*40*512,  9, 2},	/* 5 1/4 inch double density */
	{10*2*40*512, 10, 2},	/* 5 1/4 inch double density single sided */
	{ 9*2*80*512,  9, 2},	/* 80 tracks 5 1/4 inch drives rare in PCs */
	{ 9*2*80*512,  9, 2},	/* 3 1/2 inch double density */
	{15*2*80*512, 15, 2},	/* 5 1/4 inch high density (or japanese 3 1/2 inch high density) */
	{18*2*80*512, 18, 2},	/* 3 1/2 inch high density */
	{36*2*80*512, 36, 2}	/* 3 1/2 inch enhanced density */
};



static floperr_t pc_dsk_compute_geometry(floppy_image *floppy, struct basicdsk_geometry *geometry)
{
	int i;
	UINT64 size;

	memset(geometry, 0, sizeof(*geometry));
	size = floppy_image_size(floppy);

	for (i = 0; i < sizeof(disk_sizes) / sizeof(disk_sizes[0]); i++)
	{
		if (disk_sizes[i].image_size == size)
		{
			geometry->sectors = disk_sizes[i].sectors;
			geometry->heads = disk_sizes[i].heads;
			geometry->sector_length = 512;
			geometry->tracks = (int) (size / disk_sizes[i].sectors / disk_sizes[i].heads / geometry->sector_length);
			return FLOPPY_ERROR_SUCCESS;
		}
	}
	return FLOPPY_ERROR_SUCCESS;
}



static floperr_t pc_dsk_identify(floppy_image *floppy, int *vote)
{
	floperr_t err;
	struct basicdsk_geometry geometry;

	err = pc_dsk_compute_geometry(floppy, &geometry);
	if (err)
		return err;

	*vote = geometry.heads ? 100 : 0;
	return FLOPPY_ERROR_SUCCESS;
}



static floperr_t pc_dsk_construct(floppy_image *floppy, option_resolution *create_params)
{
	floperr_t err;
	struct basicdsk_geometry geometry;

	if (create_params)
	{
		/* create */
		memset(&geometry, 0, sizeof(geometry));
		geometry.heads = option_resolution_lookup_int(create_params, PARAM_HEADS);
		geometry.tracks = option_resolution_lookup_int(create_params, PARAM_TRACKS);
		geometry.sectors = option_resolution_lookup_int(create_params, PARAM_SECTORS);
		geometry.first_sector_id = 0;
		geometry.sector_length = 512;
	}
	else
	{
		/* open */
		err = pc_dsk_compute_geometry(floppy, &geometry);
		if (err)
			return err;
	}

	return basicdsk_construct(floppy, &geometry);
}



/* ----------------------------------------------------------------------- */

FLOPPY_OPTIONS_START( pc )
	FLOPPY_OPTION( pc_dsk, "dsk\0ima\0img\0",		"PC floppy disk image",	pc_dsk_identify, pc_dsk_construct,
		HEADS([1]-2)
		TRACKS(40/[80])
		SECTORS([9]-10))
FLOPPY_OPTIONS_END
