/***************************************************************************

	Taito Field Goal driver

***************************************************************************/

#include "driver.h"

extern VIDEO_START( fgoal );
extern VIDEO_UPDATE( fgoal );

extern WRITE8_HANDLER( fgoal_color_w );
extern WRITE8_HANDLER( fgoal_xpos_w );
extern WRITE8_HANDLER( fgoal_ypos_w );

UINT8* fgoal_video_ram;

static UINT8 row;
static UINT8 col;

int fgoal_player;

static unsigned shift_data;
static unsigned shift_bits;

static int prev_coin;


static int intensity(int v)
{
	/* some resistor values are unreadable, following weights are guesswork */

	switch (v)
	{
	case 0:
		return 0x18;
	case 1:
		return 0x58;
	case 2:
		return 0xc0;
	case 3:
		return 0xff;
	}

	return 0;
}


static PALETTE_INIT( fgoal )
{
	int i;

	for (i = 0; i < 64; i++)
	{
		int r = (i >> 4) & 3;
		int g = (i >> 2) & 3;
		int b = (i >> 0) & 3;

		palette_set_color(i,
			intensity(r),
			intensity(g),
			intensity(b));
	}

	/* for B/W screens PCB can be jumpered to use lower half of PROM */

	for (i = 0; i < 128; i++)
	{
		colortable[i] = color_prom[0x80 | i] & 63;
	}

	colortable[0x80] = 8;
	colortable[0x81] = 8;
	colortable[0x82] = 8;
	colortable[0x83] = 8;
	colortable[0x84] = 8;
	colortable[0x85] = 8;
	colortable[0x86] = 8;
	colortable[0x87] = 8;

	colortable[0x88] = 0;
	colortable[0x89] = 0;
	colortable[0x8a] = 0;
	colortable[0x8b] = 0;
	colortable[0x8c] = 0;
	colortable[0x8d] = 0;
	colortable[0x8e] = 0;
	colortable[0x8f] = 0;
}


static void interrupt_callback(int scanline)
{
	int coin = (readinputport(1) & 2);

	cpunum_set_input_line(0, 0, ASSERT_LINE);

	if (!coin && prev_coin)
	{
		cpunum_set_input_line(0, INPUT_LINE_NMI, ASSERT_LINE);
	}

	prev_coin = coin;

	scanline += 128;

	if (scanline > 256)
	{
		scanline = 0;
	}

	timer_set(cpu_getscanlinetime(scanline), scanline, interrupt_callback);
}


static MACHINE_INIT( fgoal )
{
	timer_set(cpu_getscanlinetime(0), 0, interrupt_callback);
}


static unsigned video_ram_address(void)
{
	return 0x4000 | (row << 5) | (col >> 3);
}


static READ8_HANDLER( fgoal_analog_r )
{
	return readinputport(2 + fgoal_player); /* PCB can be jumpered to use a single dial */
}


static READ8_HANDLER( fgoal_switches_r )
{
	if (cpu_getscanline() & 0x80)
	{
		return readinputport(1) | 0x80;
	}
	else
	{
		return readinputport(1);
	}
}


static READ8_HANDLER( fgoal_nmi_reset_r )
{
	cpunum_set_input_line(0, INPUT_LINE_NMI, CLEAR_LINE);

	return 0;
}


static READ8_HANDLER( fgoal_irq_reset_r )
{
	cpunum_set_input_line(0, 0, CLEAR_LINE);

	return 0;
}


static READ8_HANDLER( fgoal_row_r )
{
	return row;
}


static WRITE8_HANDLER( fgoal_row_w )
{
	row = data;

	shift_data = shift_data >> 8;
}
static WRITE8_HANDLER( fgoal_col_w )
{
	col = data;

	shift_bits = data & 7;
}


static WRITE8_HANDLER( fgoal_shifter_w )
{
	shift_data = (shift_data >> 8) | (data << 8); /* MB14241 custom chip */
}


static READ8_HANDLER( fgoal_address_hi_r )
{
	return video_ram_address() >> 8;
}
static READ8_HANDLER( fgoal_address_lo_r )
{
	return video_ram_address() & 0xff;
}


static READ8_HANDLER( fgoal_shifter_r )
{
	UINT8 v = shift_data >> (8 - shift_bits);

	return BITSWAP8(v, 7, 6, 5, 4, 3, 2, 1, 0);
}
static READ8_HANDLER( fgoal_shifter_reverse_r )
{
	UINT8 v = shift_data >> (8 - shift_bits);

	return BITSWAP8(v, 0, 1, 2, 3, 4, 5, 6, 7);
}


static WRITE8_HANDLER( fgoal_sound1_w )
{
	/* BIT0 => SX2 */
	/* BIT1 => SX1 */
	/* BIT2 => SX1 */
	/* BIT3 => SX1 */
	/* BIT4 => SX1 */
	/* BIT5 => SX1 */
	/* BIT6 => SX1 */
	/* BIT7 => SX1 */
}


static WRITE8_HANDLER( fgoal_sound2_w )
{
	/* BIT0 => CX0 */
	/* BIT1 => SX6 */
	/* BIT2 => N/C */
	/* BIT3 => SX5 */
	/* BIT4 => SX4 */
	/* BIT5 => SX3 */

	fgoal_player = data & 1;
}


static ADDRESS_MAP_START( cpu_map, ADDRESS_SPACE_PROGRAM, 8 )

	AM_RANGE(0x0000, 0x00ef) AM_RAM

	AM_RANGE(0x00f0, 0x00f0) AM_READ(fgoal_row_r)
	AM_RANGE(0x00f1, 0x00f1) AM_READ(fgoal_analog_r)
	AM_RANGE(0x00f2, 0x00f2) AM_READ(input_port_0_r)
	AM_RANGE(0x00f3, 0x00f3) AM_READ(fgoal_switches_r)
	AM_RANGE(0x00f4, 0x00f4) AM_READ(fgoal_address_hi_r)
	AM_RANGE(0x00f5, 0x00f5) AM_READ(fgoal_address_lo_r)
	AM_RANGE(0x00f6, 0x00f6) AM_READ(fgoal_shifter_r)
	AM_RANGE(0x00f7, 0x00f7) AM_READ(fgoal_shifter_reverse_r)
	AM_RANGE(0x00f8, 0x00fb) AM_READ(fgoal_nmi_reset_r)
	AM_RANGE(0x00fc, 0x00ff) AM_READ(fgoal_irq_reset_r)

	AM_RANGE(0x00f0, 0x00f0) AM_WRITE(fgoal_row_w)
	AM_RANGE(0x00f1, 0x00f1) AM_WRITE(fgoal_col_w)
	AM_RANGE(0x00f2, 0x00f2) AM_WRITE(fgoal_row_w)
	AM_RANGE(0x00f3, 0x00f3) AM_WRITE(fgoal_col_w)
	AM_RANGE(0x00f4, 0x00f7) AM_WRITE(fgoal_shifter_w)
	AM_RANGE(0x00f8, 0x00fb) AM_WRITE(fgoal_sound1_w)
	AM_RANGE(0x00fc, 0x00ff) AM_WRITE(fgoal_sound2_w)

	AM_RANGE(0x0100, 0x03ff) AM_RAM
	AM_RANGE(0x2000, 0x3fff) AM_ROM
	AM_RANGE(0x4000, 0x7fff) AM_RAM AM_BASE(&fgoal_video_ram) 

	AM_RANGE(0x8000, 0x8000) AM_WRITE(fgoal_ypos_w)
	AM_RANGE(0x8001, 0x8001) AM_WRITE(fgoal_xpos_w)
	AM_RANGE(0x8002, 0x8002) AM_WRITE(fgoal_color_w)
	
	AM_RANGE(0xa000, 0xbfff) AM_ROM
	AM_RANGE(0xe000, 0xffff) AM_ROM
ADDRESS_MAP_END


INPUT_PORTS_START( fgoal )

	PORT_START
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_TILT )
	PORT_DIPNAME( 0x40, 0x40, "Display Coinage Settings" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ))
	PORT_DIPSETTING(    0x40, DEF_STR( On ))
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Lives ))
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x20, "5" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown )) /* actually a jumper */
	PORT_DIPSETTING(    0x10, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))
	PORT_DIPNAME( 0x08, 0x08, "Enable Extra Credits" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ))
	PORT_DIPSETTING(    0x08, DEF_STR( Yes ))
	PORT_DIPNAME( 0x07, 0x05, "Initial Extra Credit Score" )
	PORT_DIPSETTING(    0x00, "9000" )
	PORT_DIPSETTING(    0x01, "17000" )
	PORT_DIPSETTING(    0x02, "28000" )
	PORT_DIPSETTING(    0x03, "39000" )
	PORT_DIPSETTING(    0x04, "50000" )
	PORT_DIPSETTING(    0x05, "65000" )
	PORT_DIPSETTING(    0x06, "79000" )
	PORT_DIPSETTING(    0x07, "93000" )
	PORT_DIPSETTING(    0x08, DEF_STR( None ))

	/* extra credit score changes depending on player's performance */

	PORT_START
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_SPECIAL ) /* 128V */
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Cabinet ))
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ))
	PORT_DIPSETTING(    0x40, DEF_STR( Cocktail ))
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Coinage ))
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_1C ))
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_2C ))
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Language ))
	PORT_DIPSETTING(    0x00, DEF_STR( Japanese ))
	PORT_DIPSETTING(    0x10, DEF_STR( English ))
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ))
	PORT_DIPSETTING(    0x01, DEF_STR( Off ))
	PORT_DIPSETTING(    0x00, DEF_STR( On ))

	PORT_START
	PORT_BIT( 0xff, 0x80, IPT_PADDLE ) PORT_MINMAX(1, 254) PORT_SENSITIVITY(50) PORT_KEYDELTA(10) PORT_CENTERDELTA(0) PORT_REVERSE PORT_PLAYER(1)

	PORT_START
	PORT_BIT( 0xff, 0x80, IPT_PADDLE ) PORT_MINMAX(1, 254) PORT_SENSITIVITY(50) PORT_KEYDELTA(10) PORT_CENTERDELTA(0) PORT_REVERSE PORT_PLAYER(2)

INPUT_PORTS_END


static struct GfxLayout gfxlayout =
{
	64, 64,
	1,
	4,
	{ 4, 5, 6, 7 },
	{
		0x000, 0x008, 0x010, 0x018, 0x020, 0x028, 0x030, 0x038,
		0x040, 0x048, 0x050, 0x058, 0x060, 0x068, 0x070, 0x078,
		0x080, 0x088, 0x090, 0x098, 0x0a0, 0x0a8, 0x0b0, 0x0b8,
		0x0c0, 0x0c8, 0x0d0, 0x0d8, 0x0e0, 0x0e8, 0x0f0, 0x0f8,
		0x100, 0x108, 0x110, 0x118, 0x120, 0x128, 0x130, 0x138,
		0x140, 0x148, 0x150, 0x158, 0x160, 0x168, 0x170, 0x178,
		0x180, 0x188, 0x190, 0x198, 0x1a0, 0x1a8, 0x1b0, 0x1b8,
		0x1c0, 0x1c8, 0x1d0, 0x1d8, 0x1e0, 0x1e8, 0x1f0, 0x1f8
	},
	{
		0x0000, 0x0200, 0x0400, 0x0600, 0x0800, 0x0a00, 0x0c00, 0x0e00,
		0x1000, 0x1200, 0x1400, 0x1600, 0x1800, 0x1a00, 0x1c00, 0x1e00,
		0x2000, 0x2200, 0x2400, 0x2600, 0x2800, 0x2a00, 0x2c00, 0x2e00,
		0x3000, 0x3200, 0x3400, 0x3600, 0x3800, 0x3a00, 0x3c00, 0x3e00,
		0x4000, 0x4200, 0x4400, 0x4600, 0x4800, 0x4a00, 0x4c00, 0x4e00,
		0x5000, 0x5200, 0x5400, 0x5600, 0x5800, 0x5a00, 0x5c00, 0x5e00,
		0x6000, 0x6200, 0x6400, 0x6600, 0x6800, 0x6a00, 0x6c00, 0x6e00,
		0x7000, 0x7200, 0x7400, 0x7600, 0x7800, 0x7a00, 0x7c00, 0x7e00
	},
	0
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &gfxlayout, 0x00, 8 }, /* foreground */
	{ REGION_GFX1, 0, &gfxlayout, 0x80, 1 }, /* background */
	{ -1 }
};


static MACHINE_DRIVER_START( fgoal )

	/* basic machine hardware */
	MDRV_CPU_ADD(M6800, 10065000 / 10) /* ? */
	MDRV_CPU_PROGRAM_MAP(cpu_map, 0)

	MDRV_MACHINE_INIT(fgoal)
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(0)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 263)
	MDRV_VISIBLE_AREA(0, 255, 16, 255)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(64)
	MDRV_COLORTABLE_LENGTH(128 + 16)

	MDRV_PALETTE_INIT(fgoal)
	MDRV_VIDEO_START(fgoal)
	MDRV_VIDEO_UPDATE(fgoal)

	/* sound hardware */
MACHINE_DRIVER_END


ROM_START( fgoal )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "tf04.bin", 0xA000, 0x0800, CRC(45fd7b03) SHA1(adc75a7fff6402c5c668ac28aec5d7c31c67c948) ) 
	ROM_RELOAD(           0xE000, 0x0800 ) 
	ROM_LOAD( "tf03.bin", 0xA800, 0x0800, CRC(01891c32) SHA1(013480dc970da83bda969506b2bd8865753a78ad) ) 
	ROM_RELOAD(           0xE800, 0x0800 ) 
	ROM_LOAD( "tf02.bin", 0xB000, 0x0800, CRC(c297d509) SHA1(a180e5203008db6b358dceee7349682ae3675c20) ) 
	ROM_RELOAD(           0xF000, 0x0800 ) 
	ROM_LOAD( "tf01.bin", 0xB800, 0x0800, CRC(1b0bfa5c) SHA1(768e14f08063cc022d7e18a9cb2197d64a9e1b8d) ) 
	ROM_RELOAD(           0xF800, 0x0800 ) 

	ROM_REGION( 0x1000, REGION_GFX1, ROMREGION_DISPOSE ) /* background */
	ROM_LOAD( "tf05.bin", 0x0000, 0x0400, CRC(925b78ab) SHA1(97d6e572658715dc4f6c37b98ba5352643fc8e27) ) 
	ROM_LOAD( "tf06.bin", 0x0400, 0x0400, CRC(3d2f007b) SHA1(7f4b6f3f08be8c886af3e2ccd3c0d93ae54d4649) ) 
	ROM_LOAD( "tf07.bin", 0x0800, 0x0400, CRC(0b1d01c4) SHA1(8680602fecd412e5136e1107618a2e0a59b37d08) ) 
	ROM_LOAD( "tf08.bin", 0x0c00, 0x0400, CRC(5cbc7dfd) SHA1(1a054dc72d25615ea6f903f6da8108033514fd1f) ) 

	ROM_REGION( 0x0100, REGION_PROMS, ROMREGION_INVERT )
	ROM_LOAD_NIB_LOW ( "tf09.bin", 0x0000, 0x0100, CRC(b0fc4b80) SHA1(c6029f6d912275aa65302ca97281e10ccbf63159) ) 
	ROM_LOAD_NIB_HIGH( "tf10.bin", 0x0000, 0x0100, CRC(7b30b15d) SHA1(e9826a107b209e18d891ead341eda3d4523ce195) ) 
ROM_END


GAMEX( 1979, fgoal, 0, fgoal, fgoal, 0, ROT90, "Taito", "Field Goal", GAME_NO_SOUND )