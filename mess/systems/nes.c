/***************************************************************************

  nes.c

  Driver file to handle emulation of the Nintendo Entertainment System (Famicom).

  MESS driver by Brad Oliver (bradman@pobox.com), NES sound code by Matt Conte.
  Based in part on the old xNes code, by Nicolas Hamel, Chuck Mason, Brad Oliver,
  Richard Bannister and Jeff Mitchell.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/ppu2c03b.h"
#include "includes/nes.h"
#include "cpu/m6502/m6502.h"
#include "devices/cartslot.h"
#include "inputx.h"

unsigned char *battery_ram;
unsigned char *main_ram;

static  READ8_HANDLER ( nes_bogus_r )
{
    static int val = 0xff;
    val ^= 0xff;
    return val;
}

static ADDRESS_MAP_START( nes_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_RAM		AM_MIRROR(0x1800)	AM_BASE(&main_ram)	/* RAM */
	AM_RANGE(0x2000, 0x3fff) AM_READWRITE(ppu2c03b_0_r,     ppu2c03b_0_w)		/* PPU registers */
	AM_RANGE(0x4000, 0x4015) AM_WRITE(NESPSG_0_w)
	AM_RANGE(0x4015, 0x4015) AM_READ(nes_bogus_r)			/* ?? sound status ?? */
	AM_RANGE(0x4016, 0x4016) AM_READWRITE(nes_IN0_r,        nes_IN0_w)			/* IN0 - input port 1 */
	AM_RANGE(0x4017, 0x4017) AM_READWRITE(nes_IN1_r,        nes_IN1_w)			/* IN1 - input port 2 */
	AM_RANGE(0x4100, 0x5fff) AM_READWRITE(nes_low_mapper_r, nes_low_mapper_w)	/* Perform unholy acts on the machine */
ADDRESS_MAP_END


INPUT_PORTS_START( nes )
	PORT_START  /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1) PORT_NAME("P1 A") PORT_CODE(KEYCODE_LALT) PORT_CODE(JOYCODE_1_BUTTON1 )	PORT_CATEGORY(1) PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON2) PORT_NAME("P1 B") PORT_CODE(KEYCODE_LCONTROL) PORT_CODE(JOYCODE_1_BUTTON2 )	PORT_CATEGORY(1) PORT_PLAYER(1)
	PORT_BIT ( 0x04, IP_ACTIVE_HIGH, IPT_SELECT)															PORT_CATEGORY(1) PORT_PLAYER(1)
	PORT_BIT ( 0x08, IP_ACTIVE_HIGH, IPT_START)															PORT_CATEGORY(1) PORT_PLAYER(1)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP) PORT_NAME("P1 Up") PORT_CODE(KEYCODE_UP) PORT_CODE(JOYCODE_1_UP )		PORT_CATEGORY(1) PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN) PORT_NAME("P1 Down") PORT_CODE(KEYCODE_DOWN) PORT_CODE(JOYCODE_1_DOWN )	PORT_CATEGORY(1) PORT_PLAYER(1)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT) PORT_NAME("P1 Left") PORT_CODE(KEYCODE_LEFT) PORT_CODE(JOYCODE_1_LEFT )	PORT_CATEGORY(1) PORT_PLAYER(1)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT) PORT_NAME("P1 Right") PORT_CODE(KEYCODE_RIGHT) PORT_CODE(JOYCODE_1_RIGHT )	PORT_CATEGORY(1) PORT_PLAYER(1)

	PORT_START  /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1) PORT_NAME("P2 A") PORT_CODE(KEYCODE_0_PAD) PORT_CODE(JOYCODE_2_BUTTON1 )	PORT_CATEGORY(2) PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON2) PORT_NAME("P2 B") PORT_CODE(KEYCODE_DEL_PAD) PORT_CODE(JOYCODE_2_BUTTON2 )	PORT_CATEGORY(2) PORT_PLAYER(2)
	PORT_BIT ( 0x04, IP_ACTIVE_HIGH, IPT_SELECT)															PORT_CATEGORY(2) PORT_PLAYER(2)
	PORT_BIT ( 0x08, IP_ACTIVE_HIGH, IPT_START)															PORT_CATEGORY(2) PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP) PORT_NAME("P2 Up") PORT_CODE(KEYCODE_8_PAD) PORT_CODE(JOYCODE_2_UP )		PORT_CATEGORY(2) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN) PORT_NAME("P2 Down") PORT_CODE(KEYCODE_2_PAD) PORT_CODE(JOYCODE_2_DOWN )	PORT_CATEGORY(2) PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT) PORT_NAME("P2 Left") PORT_CODE(KEYCODE_4_PAD) PORT_CODE(JOYCODE_2_LEFT )	PORT_CATEGORY(2) PORT_PLAYER(2)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT) PORT_NAME("P2 Right") PORT_CODE(KEYCODE_6_PAD) PORT_CODE(JOYCODE_2_RIGHT )	PORT_CATEGORY(2) PORT_PLAYER(2)

	PORT_START  /* IN2 */
	PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1)															PORT_CATEGORY(3) PORT_PLAYER(3)
	PORT_BIT ( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON2)															PORT_CATEGORY(3) PORT_PLAYER(3)
	PORT_BIT ( 0x04, IP_ACTIVE_HIGH, IPT_SELECT)															PORT_CATEGORY(3) PORT_PLAYER(3)
	PORT_BIT ( 0x08, IP_ACTIVE_HIGH, IPT_START)															PORT_CATEGORY(3) PORT_PLAYER(3)
	PORT_BIT ( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP)															PORT_CATEGORY(3) PORT_PLAYER(3)
	PORT_BIT ( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN)															PORT_CATEGORY(3) PORT_PLAYER(3)
	PORT_BIT ( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT)															PORT_CATEGORY(3) PORT_PLAYER(3)
	PORT_BIT ( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT)															PORT_CATEGORY(3) PORT_PLAYER(3)

	PORT_START  /* IN3 */
	PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1)															PORT_CATEGORY(4) PORT_PLAYER(4)
	PORT_BIT ( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON2)															PORT_CATEGORY(4) PORT_PLAYER(4)
	PORT_BIT ( 0x04, IP_ACTIVE_HIGH, IPT_SELECT)															PORT_CATEGORY(4) PORT_PLAYER(4)
	PORT_BIT ( 0x08, IP_ACTIVE_HIGH, IPT_START)															PORT_CATEGORY(4) PORT_PLAYER(4)
	PORT_BIT ( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP)															PORT_CATEGORY(4) PORT_PLAYER(4)
	PORT_BIT ( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN)															PORT_CATEGORY(4) PORT_PLAYER(4)
	PORT_BIT ( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT)															PORT_CATEGORY(4) PORT_PLAYER(4)
	PORT_BIT ( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT)															PORT_CATEGORY(4) PORT_PLAYER(4)

	PORT_START  /* IN4 - P1 zapper */
	PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_X) PORT_SENSITIVITY(70) PORT_KEYDELTA(30) PORT_MINMAX(0,255 )														PORT_CATEGORY(5) PORT_PLAYER(1)
	PORT_START  /* IN5 - P1 zapper */
	PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_Y) PORT_SENSITIVITY(50) PORT_KEYDELTA(30) PORT_MINMAX(0,255 )														PORT_CATEGORY(5) PORT_PLAYER(1)
	PORT_START  /* IN6 - P1 zapper trigger */
	PORT_BIT( 0x03, IP_ACTIVE_HIGH, IPT_BUTTON3) PORT_NAME("Lightgun Trigger") 										PORT_CATEGORY(5) PORT_PLAYER(1)

	PORT_START  /* IN7 - P2 zapper */
	PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_X) PORT_SENSITIVITY(70) PORT_KEYDELTA(30) PORT_MINMAX(0,255 )														PORT_CATEGORY(6) PORT_PLAYER(2)
	PORT_START  /* IN8 - P2 zapper */
	PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_Y) PORT_SENSITIVITY(50) PORT_KEYDELTA(30) PORT_MINMAX(0,255 )														PORT_CATEGORY(6) PORT_PLAYER(2)
	PORT_START  /* IN9 - P2 zapper trigger */
	PORT_BIT( 0x03, IP_ACTIVE_HIGH, IPT_BUTTON3) PORT_NAME("Lightgun 2 Trigger") 										PORT_CATEGORY(6) PORT_PLAYER(2)

	PORT_START  /* IN10 - arkanoid paddle */
	PORT_BIT( 0xff, 0x7f, IPT_PADDLE) PORT_SENSITIVITY(25) PORT_KEYDELTA(3) PORT_MINMAX(0x62,0xf2 )																	PORT_CATEGORY(7)

	PORT_START  /* IN11 - configuration */
	PORT_CATEGORY_CLASS( 0x000f, 0x0001, "P1 Controller")
	PORT_CATEGORY_ITEM(  0x0000, "Unconnected",		0 )
	PORT_CATEGORY_ITEM(  0x0001, "Gamepad",			1 )
	PORT_CATEGORY_ITEM(  0x0002, "Zapper 1",		5 )
	PORT_CATEGORY_ITEM(  0x0003, "Zapper 2",		6 )
	PORT_CATEGORY_CLASS( 0x00f0, 0x0010, "P2 Controller")
	PORT_CATEGORY_ITEM(  0x0000, "Unconnected",		0 )
	PORT_CATEGORY_ITEM(  0x0010, "Gamepad",			2 )
	PORT_CATEGORY_ITEM(  0x0020, "Zapper 1",		5 )
	PORT_CATEGORY_ITEM(  0x0030, "Zapper 2",		6 )
	PORT_CATEGORY_ITEM(  0x0040, "Arkanoid paddle",	7 )
	PORT_CATEGORY_CLASS( 0x0f00, 0x0000, "P3 Controller")
	PORT_CATEGORY_ITEM(  0x0000, "Unconnected",		0 )
	PORT_CATEGORY_ITEM(  0x0100, "Gamepad",			3 )
	PORT_CATEGORY_CLASS( 0xf000, 0x0000, "P4 Controller")
	PORT_CATEGORY_ITEM(  0x0000, "Unconnected",		0 )
	PORT_CATEGORY_ITEM(  0x1000, "Gamepad",			4 )

	PORT_START  /* IN12 - configuration */
	PORT_CONFNAME( 0x01, 0x00, "Draw Top/Bottom 8 Lines")
	PORT_CONFSETTING(    0x01, DEF_STR(No) )
	PORT_CONFSETTING(    0x00, DEF_STR(Yes) )
	PORT_CONFNAME( 0x02, 0x00, "Enforce 8 Sprites/line")
	PORT_CONFSETTING(    0x02, DEF_STR(No) )
	PORT_CONFSETTING(    0x00, DEF_STR(Yes) )
INPUT_PORTS_END

INPUT_PORTS_START( famicom )
	PORT_START  /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1) PORT_NAME("P1 A") PORT_CODE(KEYCODE_LALT) PORT_CODE(JOYCODE_1_BUTTON1 )	PORT_CATEGORY(1) PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON2) PORT_NAME("P1 B") PORT_CODE(KEYCODE_LCONTROL) PORT_CODE(JOYCODE_1_BUTTON2 )	PORT_CATEGORY(1) PORT_PLAYER(1)
	PORT_BIT ( 0x04, IP_ACTIVE_HIGH, IPT_SELECT)															PORT_CATEGORY(1) PORT_PLAYER(1)
	PORT_BIT ( 0x08, IP_ACTIVE_HIGH, IPT_START)															PORT_CATEGORY(1) PORT_PLAYER(1)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP) PORT_NAME("P1 Up") PORT_CODE(KEYCODE_UP) PORT_CODE(JOYCODE_1_UP )		PORT_CATEGORY(1) PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN) PORT_NAME("P1 Down") PORT_CODE(KEYCODE_DOWN) PORT_CODE(JOYCODE_1_DOWN )	PORT_CATEGORY(1) PORT_PLAYER(1)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT) PORT_NAME("P1 Left") PORT_CODE(KEYCODE_LEFT) PORT_CODE(JOYCODE_1_LEFT )	PORT_CATEGORY(1) PORT_PLAYER(1)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT) PORT_NAME("P1 Right") PORT_CODE(KEYCODE_RIGHT) PORT_CODE(JOYCODE_1_RIGHT )	PORT_CATEGORY(1) PORT_PLAYER(1)

	PORT_START  /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1) PORT_NAME("P2 A") PORT_CODE(KEYCODE_0_PAD) PORT_CODE(JOYCODE_2_BUTTON1 )	PORT_CATEGORY(2) PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON2) PORT_NAME("P2 B") PORT_CODE(KEYCODE_DEL_PAD) PORT_CODE(JOYCODE_2_BUTTON2 )	PORT_CATEGORY(2) PORT_PLAYER(2)
	PORT_BIT ( 0x04, IP_ACTIVE_HIGH, IPT_SELECT)															PORT_CATEGORY(2) PORT_PLAYER(2)
	PORT_BIT ( 0x08, IP_ACTIVE_HIGH, IPT_START)															PORT_CATEGORY(2) PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP) PORT_NAME("P2 Up") PORT_CODE(KEYCODE_8_PAD) PORT_CODE(JOYCODE_2_UP )		PORT_CATEGORY(2) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN) PORT_NAME("P2 Down") PORT_CODE(KEYCODE_2_PAD) PORT_CODE(JOYCODE_2_DOWN )	PORT_CATEGORY(2) PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT) PORT_NAME("P2 Left") PORT_CODE(KEYCODE_4_PAD) PORT_CODE(JOYCODE_2_LEFT )	PORT_CATEGORY(2) PORT_PLAYER(2)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT) PORT_NAME("P2 Right") PORT_CODE(KEYCODE_6_PAD) PORT_CODE(JOYCODE_2_RIGHT )	PORT_CATEGORY(2) PORT_PLAYER(2)

	PORT_START  /* IN2 */
	PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1)															PORT_CATEGORY(3) PORT_PLAYER(3)
	PORT_BIT ( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON2)															PORT_CATEGORY(3) PORT_PLAYER(3)
	PORT_BIT ( 0x04, IP_ACTIVE_HIGH, IPT_SELECT)															PORT_CATEGORY(3) PORT_PLAYER(3)
	PORT_BIT ( 0x08, IP_ACTIVE_HIGH, IPT_START)															PORT_CATEGORY(3) PORT_PLAYER(3)
	PORT_BIT ( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP)															PORT_CATEGORY(3) PORT_PLAYER(3)
	PORT_BIT ( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN)															PORT_CATEGORY(3) PORT_PLAYER(3)
	PORT_BIT ( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT)															PORT_CATEGORY(3) PORT_PLAYER(3)
	PORT_BIT ( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT)															PORT_CATEGORY(3) PORT_PLAYER(3)

	PORT_START  /* IN3 */
	PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1)															PORT_CATEGORY(4) PORT_PLAYER(4)
	PORT_BIT ( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON2)															PORT_CATEGORY(4) PORT_PLAYER(4)
	PORT_BIT ( 0x04, IP_ACTIVE_HIGH, IPT_SELECT)															PORT_CATEGORY(4) PORT_PLAYER(4)
	PORT_BIT ( 0x08, IP_ACTIVE_HIGH, IPT_START)															PORT_CATEGORY(4) PORT_PLAYER(4)
	PORT_BIT ( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP)															PORT_CATEGORY(4) PORT_PLAYER(4)
	PORT_BIT ( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN)															PORT_CATEGORY(4) PORT_PLAYER(4)
	PORT_BIT ( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT)															PORT_CATEGORY(4) PORT_PLAYER(4)
	PORT_BIT ( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT)															PORT_CATEGORY(4) PORT_PLAYER(4)

	PORT_START  /* IN4 - P1 zapper */
	PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_X) PORT_SENSITIVITY(70) PORT_KEYDELTA(30) PORT_MINMAX(0,255 )														PORT_CATEGORY(5) PORT_PLAYER(1)
	PORT_START  /* IN5 - P1 zapper */
	PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_Y) PORT_SENSITIVITY(50) PORT_KEYDELTA(30) PORT_MINMAX(0,255 )														PORT_CATEGORY(5) PORT_PLAYER(1)
	PORT_START  /* IN6 - P1 zapper trigger */
	PORT_BIT( 0x03, IP_ACTIVE_HIGH, IPT_BUTTON3) PORT_NAME("Lightgun Trigger") 										PORT_CATEGORY(5) PORT_PLAYER(1)

	PORT_START  /* IN7 - P2 zapper */
	PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_X) PORT_SENSITIVITY(70) PORT_KEYDELTA(30) PORT_MINMAX(0,255 )														PORT_CATEGORY(6) PORT_PLAYER(2)
	PORT_START  /* IN8 - P2 zapper */
	PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_Y) PORT_SENSITIVITY(50) PORT_KEYDELTA(30) PORT_MINMAX(0,255 )														PORT_CATEGORY(6) PORT_PLAYER(2)
	PORT_START  /* IN9 - P2 zapper trigger */
	PORT_BIT( 0x03, IP_ACTIVE_HIGH, IPT_BUTTON3) PORT_NAME("Lightgun 2 Trigger") 										PORT_CATEGORY(6) PORT_PLAYER(2)

	PORT_START  /* IN10 - arkanoid paddle */
	PORT_BIT( 0xff, 0x7f, IPT_PADDLE) PORT_SENSITIVITY(25) PORT_KEYDELTA(3) PORT_MINMAX(0x62,0xf2 )																	PORT_CATEGORY(7)

	PORT_START  /* IN11 - configuration */
	PORT_CATEGORY_CLASS( 0x000f, 0x0001, "P1 Controller")
	PORT_CATEGORY_ITEM(  0x0000, "Unconnected",		0 )
	PORT_CATEGORY_ITEM(  0x0001, "Gamepad",			1 )
	PORT_CATEGORY_ITEM(  0x0002, "Zapper 1",		5 )
	PORT_CATEGORY_ITEM(  0x0003, "Zapper 2",		6 )
	PORT_CATEGORY_CLASS( 0x00f0, 0x0010, "P2 Controller")
	PORT_CATEGORY_ITEM(  0x0000, "Unconnected",		0 )
	PORT_CATEGORY_ITEM(  0x0010, "Gamepad",			2 )
	PORT_CATEGORY_ITEM(  0x0020, "Zapper 1",		5 )
	PORT_CATEGORY_ITEM(  0x0030, "Zapper 2",		6 )
	PORT_CATEGORY_ITEM(  0x0040, "Arkanoid paddle",	7 )
	PORT_CATEGORY_CLASS( 0x0f00, 0x0000, "P3 Controller")
	PORT_CATEGORY_ITEM(  0x0000, "Unconnected",		0 )
	PORT_CATEGORY_ITEM(  0x0100, "Gamepad",			3 )
	PORT_CATEGORY_CLASS( 0xf000, 0x0000, "P4 Controller")
	PORT_CATEGORY_ITEM(  0x0000, "Unconnected",		0 )
	PORT_CATEGORY_ITEM(  0x1000, "Gamepad",			4 )

	PORT_START /* IN12 - fake keys */
//  PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON3 )
	PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON3) PORT_NAME("Change Disk Side")
INPUT_PORTS_END

/* This layout is not changed at runtime */
struct GfxLayout nes_vram_charlayout =
{
    8,8,    /* 8*8 characters */
    512,    /* 512 characters */
    2,  /* 2 bits per pixel */
    { 8*8, 0 }, /* the two bitplanes are separated */
    { 0, 1, 2, 3, 4, 5, 6, 7 },
    { 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
    16*8    /* every char takes 16 consecutive bytes */
};


static WRITE8_HANDLER(nes_vh_sprite_dma_w)
{
	unsigned char *RAM = memory_region(REGION_CPU1);
	ppu2c03b_spriteram_dma(0, &RAM[data * 0x100]);
}

static struct NESinterface nes_interface =
{
    1,
    N2A03_DEFAULTCLOCK ,
    { 100 },
    { 0 },
    { nes_vh_sprite_dma_w },
    { NULL }
};

static struct NESinterface nespal_interface =
{
    1,
    26601712/15,
    { 100 },
    { 0 },
    { nes_vh_sprite_dma_w },
    { NULL }
};

ROM_START( nes )
    ROM_REGION( 0x10000, REGION_CPU1,0 )  /* Main RAM + program banks */
    ROM_REGION( 0x2000,  REGION_GFX1,0 )  /* VROM */
    ROM_REGION( 0x2000,  REGION_GFX2,0 )  /* VRAM */
    ROM_REGION( 0x10000, REGION_USER1,0 ) /* WRAM */
ROM_END

ROM_START( nespal )
    ROM_REGION( 0x10000, REGION_CPU1,0 )  /* Main RAM + program banks */
    ROM_REGION( 0x2000,  REGION_GFX1,0 )  /* VROM */
    ROM_REGION( 0x2000,  REGION_GFX2,0 )  /* VRAM */
    ROM_REGION( 0x10000, REGION_USER1,0 ) /* WRAM */
ROM_END

ROM_START( famicom )
    ROM_REGION( 0x10000, REGION_CPU1,0 )  /* Main RAM + program banks */
    ROM_LOAD_OPTIONAL ("disksys.rom", 0xe000, 0x2000, CRC(5e607dcf) SHA1(57fe1bdee955bb48d357e463ccbf129496930b62))

    ROM_REGION( 0x2000,  REGION_GFX1,0 )  /* VROM */

    ROM_REGION( 0x2000,  REGION_GFX2,0 )  /* VRAM */

    ROM_REGION( 0x10000, REGION_USER1,0 ) /* WRAM */
ROM_END



static MACHINE_DRIVER_START( nes )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", N2A03, N2A03_DEFAULTCLOCK)        /* 0,894886 Mhz */
	MDRV_CPU_PROGRAM_MAP(nes_map, 0)
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(114*(NTSC_SCANLINES_PER_FRAME-BOTTOM_VISIBLE_SCANLINE))

	MDRV_MACHINE_INIT( nes )
	MDRV_MACHINE_STOP( nes )

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 30*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 0*8, 30*8-1)
	MDRV_PALETTE_INIT(nes)
	MDRV_VIDEO_START(nes)
	MDRV_VIDEO_UPDATE(nes)

	MDRV_PALETTE_LENGTH(4*16*8)
	MDRV_COLORTABLE_LENGTH(4*8)

    /* sound hardware */
	MDRV_SOUND_ADD_TAG("nessound", NES, nes_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( nespal )
	MDRV_IMPORT_FROM( nes )

	/* basic machine hardware */
	MDRV_CPU_REPLACE("main", N2A03, 26601712/15)        /* 0,894886 Mhz */
	MDRV_FRAMES_PER_SECOND(50)

    /* sound hardware */
	MDRV_SOUND_REPLACE("nessound", NES, nespal_interface)
MACHINE_DRIVER_END

SYSTEM_CONFIG_START(nes)
	CONFIG_DEVICE_CARTSLOT_REQ(1, "nes\0", NULL, NULL, device_load_nes_cart, NULL, NULL, nes_partialhash)
SYSTEM_CONFIG_END

SYSTEM_CONFIG_START(famicom)
	CONFIG_DEVICE_CARTSLOT_OPT(1, "nes\0", NULL, NULL, device_load_nes_cart, NULL, NULL, nes_partialhash)
	CONFIG_DEVICE_LEGACY(IO_FLOPPY, 1, "dsk\0fds\0", DEVICE_LOAD_RESETS_NONE, OSD_FOPEN_READ, NULL, NULL, device_load_nes_disk, device_unload_nes_disk, NULL)
SYSTEM_CONFIG_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

/*     YEAR  NAME      PARENT    COMPAT	MACHINE   INPUT     INIT      CONFIG	COMPANY   FULLNAME */
CONS( 1983, famicom,   0,        0,		nes,      famicom,  nes,      famicom,	"Nintendo", "Famicom" )
CONS( 1985, nes,       0,        0,		nes,      nes,      nes,      nes,		"Nintendo", "Nintendo Entertainment System (NTSC)" )
CONS( 1987, nespal,    nes,      0,		nespal,   nes,      nespal,   nes,		"Nintendo", "Nintendo Entertainment System (PAL)" )

