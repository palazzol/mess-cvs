/***************************************************************************

Truco Clemente (c) 1991 Caloi Miky SRL

driver by Ernesto Corvi

Notes:
- Wrong colors.
- Audio is almost there.
- I think this runs on a heavily modified PacMan type of board.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

/* from vidhrdw */
extern WRITE8_HANDLER( trucocl_videoram_w );
extern WRITE8_HANDLER( trucocl_colorram_w );
extern VIDEO_START( trucocl );
extern VIDEO_UPDATE( trucocl );


static WRITE8_HANDLER( irq_enable_w)
{
	interrupt_enable_w( 0, (~data) & 1 );
}

static int cur_dac_address = -1;
static int cur_dac_address_index = 0;

static void dac_irq( int param )
{
	cpunum_set_input_line( 0, INPUT_LINE_NMI, PULSE_LINE );
}

static WRITE8_HANDLER( audio_dac_w)
{
	data8_t *rom = memory_region(REGION_CPU1);
	int	dac_address = ( data & 0xf0 ) << 8;
	int	sel = ( ( (~data) >> 1 ) & 2 ) | ( data & 1 );

	if ( cur_dac_address != dac_address )
	{
		cur_dac_address_index = 0;
		cur_dac_address = dac_address;
	}
	else
	{
		cur_dac_address_index++;
	}

	if ( sel & 1 )
		dac_address += 0x10000;

	if ( sel & 2 )
		dac_address += 0x10000;

	dac_address += 0x10000;

	DAC_data_w( 0, rom[dac_address+cur_dac_address_index] );

	timer_set( TIME_IN_HZ( 16000 ), 0, dac_irq );
}

static ADDRESS_MAP_START( readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x4000, 0x43ff) AM_READ(MRA8_RAM)
	AM_RANGE(0x4400, 0x47ff) AM_READ(MRA8_RAM)
	AM_RANGE(0x4c00, 0x4fff) AM_READ(MRA8_RAM)
	AM_RANGE(0x5000, 0x503f) AM_READ(input_port_0_r)	/* IN0 */
	AM_RANGE(0x8000, 0xffff) AM_READ(MRA8_ROM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x4000, 0x43ff) AM_WRITE(trucocl_videoram_w) AM_BASE(&videoram)
	AM_RANGE(0x4400, 0x47ff) AM_WRITE(trucocl_colorram_w) AM_BASE(&colorram)
	AM_RANGE(0x4c00, 0x4fff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x5000, 0x5000) AM_WRITE(irq_enable_w)
	AM_RANGE(0x5080, 0x5080) AM_WRITE(audio_dac_w)
	AM_RANGE(0x50c0, 0x50c0) AM_WRITE(watchdog_reset_w)
	AM_RANGE(0x8000, 0xffff) AM_WRITE(MWA8_ROM)
ADDRESS_MAP_END

INPUT_PORTS_START( trucocl )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_IMPULSE(2)
INPUT_PORTS_END


static struct GfxLayout tilelayout =
{
	16,8,		/* 16*16 characters */
	0x10000/32,	/* 2048 characters */
	2,			/* 2 bits per pixel */
	{ 0, 1 },
	{ 0, 2, 4, 6, 0x8000*8+0, 0x8000*8+2, 0x8000*8+4, 0x8000*8+6, 8*8+0, 8*8+2, 8*8+4, 8*8+6, 0x8008*8+0, 0x8008*8+2, 0x8008*8+4, 0x8008*8+6 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	16*8		/* every char takes 16 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, 		&tilelayout,      0, 4 },
	{ REGION_GFX1, 0x10000, &tilelayout,      0, 4 },
	{ -1 } /* end of array */
};

static INTERRUPT_GEN( trucocl_interrupt )
{
	irq0_line_hold();
}

static struct DACinterface dac_interface =
{
	1,
	{ 100 },
};

static MACHINE_DRIVER_START( trucocl )
	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 18432000/6)
	MDRV_CPU_PROGRAM_MAP(readmem,writemem)
	MDRV_CPU_VBLANK_INT(trucocl_interrupt,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*16, 32*8)
	MDRV_VISIBLE_AREA(0*16, 32*16-1, 0*8, 32*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(16)
	MDRV_COLORTABLE_LENGTH(16)

	MDRV_VIDEO_START(trucocl)
	MDRV_VIDEO_UPDATE(trucocl)

	/* sound hardware */
	MDRV_SOUND_ADD(DAC, dac_interface)

MACHINE_DRIVER_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( trucocl )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )	/* ROMs + space for additional RAM + samples */
	ROM_LOAD( "trucocl.01", 0x00000, 0x20000, CRC(c9511c37) SHA1(d6a0fa573c8d2faf1a94a2be26fcaafe631d0699) )
	ROM_LOAD( "trucocl.03", 0x20000, 0x20000, CRC(b37ce38c) SHA1(00bd506e9a03cb8ed65b0b599514db6b9b0ee5f3) ) /* samples */

	ROM_REGION( 0x20000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "trucocl.02", 0x0000, 0x20000, CRC(bda803e5) SHA1(e4fee42f23be4e0dc8926b6294e4b3e4a38ff185) ) /* tiles */
ROM_END

/*************************************
 *
 *	Game drivers
 *
 *************************************/

/******************************************************************************/

/*    YEAR	 NAME     PARENT  MACHINE  INPUT    INIT  MONITOR  */

GAMEX( 1991, trucocl,  0,     trucocl, trucocl, 0,    ROT0, "Caloi Miky SRL", "Truco Clemente", GAME_WRONG_COLORS | GAME_IMPERFECT_SOUND )