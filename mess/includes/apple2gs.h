/*********************************************************************

	apple2gs.h

	Apple IIgs code

*********************************************************************/

#ifndef APPLE2GS_H
#define APPLE2GS_H

extern UINT8 *apple2gs_slowmem;
extern data8_t apple2gs_newvideo;
extern UINT16 apple2gs_bordercolor;

INTERRUPT_GEN( apple2gs_interrupt );

DRIVER_INIT( apple2gs );
VIDEO_START( apple2gs );
VIDEO_UPDATE( apple2gs );

#endif /* APPLE2GS_H */