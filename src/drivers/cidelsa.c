#include "driver.h"
#include "cpu/cdp1802/cdp1802.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/cdp1869.h"
#include "sound/cdp1869.h"
#include "sound/ay8910.h"

/* Read/Write Handlers */

WRITE8_HANDLER ( destryer_out0_w )
{
	/*
          bit   description

            0
            1
            2
            3
            4
            5
            6
            7
    */
}

WRITE8_HANDLER ( altair_out0_w )
{
	/*
          bit   description

            0   S1
            1   S2
            2   S3
            3   LG1
            4   LG2
            5   LGF
            6   CONT. M4
            7   CONT. M1
    */

	set_led_status(0, data & 0x08); // 1P
	set_led_status(1, data & 0x10); // 2P
	set_led_status(2, data & 0x20); // FIRE
}

WRITE8_HANDLER ( draco_out0_w )
{
	/*
          bit   description

            0   3K9 to Green signal
            1   820R to Blue signal
            2   510R to Red signal
            3   1K to N/C
            4   N/C
            5   SONIDO A
            6   SONIDO B
            7   SONIDO C
    */
}

/* Memory Maps */

// Destroyer

static ADDRESS_MAP_START( destryer_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_ROM
	AM_RANGE(0x2000, 0x20ff) AM_RAM
	AM_RANGE(0xf800, 0xfbff) AM_RAM AM_BASE(&videoram) AM_SIZE(&videoram_size)
ADDRESS_MAP_END

static ADDRESS_MAP_START( destryer_io_map, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x00, 0x00) AM_WRITE(destryer_out0_w)
	AM_RANGE(0x01, 0x01) AM_READ(input_port_0_r)
	AM_RANGE(0x02, 0x02) AM_READ(input_port_1_r)
	AM_RANGE(0x03, 0x03) AM_WRITE(cdp1870_out3_w)
	AM_RANGE(0x04, 0x07) AM_WRITE(cdp1869_out_w)
ADDRESS_MAP_END

// Altair

static ADDRESS_MAP_START( altair_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x2fff) AM_ROM
	AM_RANGE(0x3000, 0x30ff) AM_RAM
	AM_RANGE(0xf800, 0xfbff) AM_RAM AM_BASE(&videoram) AM_SIZE(&videoram_size)
ADDRESS_MAP_END

static ADDRESS_MAP_START( altair_io_map, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x00, 0x00) AM_WRITE(altair_out0_w)
	AM_RANGE(0x01, 0x01) AM_READ(input_port_0_r)
	AM_RANGE(0x02, 0x02) AM_READ(input_port_1_r)
	AM_RANGE(0x03, 0x03) AM_WRITE(cdp1870_out3_w)
	AM_RANGE(0x04, 0x04) AM_READ(input_port_2_r)
	AM_RANGE(0x04, 0x07) AM_WRITE(cdp1869_out_w)
ADDRESS_MAP_END

// Draco

static ADDRESS_MAP_START( draco_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_ROM
	AM_RANGE(0x4000, 0x43ff) AM_RAM
	AM_RANGE(0xf800, 0xffff) AM_RAM AM_BASE(&videoram) AM_SIZE(&videoram_size)
ADDRESS_MAP_END

static ADDRESS_MAP_START( draco_io_map, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x00, 0x00) AM_WRITE(draco_out0_w)
	AM_RANGE(0x01, 0x01) AM_READ(input_port_0_r)
	AM_RANGE(0x02, 0x02) AM_READ(input_port_1_r)
	AM_RANGE(0x03, 0x03) AM_WRITE(cdp1870_out3_w)
	AM_RANGE(0x04, 0x04) AM_READ(input_port_2_r)
	AM_RANGE(0x04, 0x07) AM_WRITE(cdp1869_out_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( draco_sound_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x07ff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( draco_sound_io_map, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x00, 0x00) AM_WRITE(AY8910_control_port_0_w)
	AM_RANGE(0x01, 0x01) AM_WRITE(AY8910_write_port_0_w)
ADDRESS_MAP_END

/* Input Ports */

INPUT_PORTS_START( destryer )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("EF")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SPECIAL ) // _PRD from CDP1869
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SERVICE ) // ST
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN2 ) // M2
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN1 ) // M1
INPUT_PORTS_END

INPUT_PORTS_START( altair )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN ) // CARTUCHO
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START1 ) // 1P
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START2 ) // 2P
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) // RG
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) // LF
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 ) // FR
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN ) // DF0
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN ) // DF1
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN ) // EX0
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN ) // EX1
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN ) // RV0
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN ) // RV1
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) // MN0
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN ) // MN1

	PORT_START_TAG("IN2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) // UP
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) // DN
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 ) // IN
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("EF")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SPECIAL ) // _PRD from CDP1869
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SERVICE ) // ST
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN2 ) // M2
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN1 ) // M1
INPUT_PORTS_END

INPUT_PORTS_START( draco )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SPECIAL ) // PCB from CDP1869

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN ) // SW1
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN ) // SW2
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN ) // SW3
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN ) // SW4
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN ) // SW5
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN ) // SW6
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) // SW7
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN ) // SW8

	PORT_START_TAG("IN2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("EF")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SPECIAL ) // _PRD from CDP1869
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SERVICE ) // ST
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN2 ) // M2
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN1 ) // M1
INPUT_PORTS_END

/* CDP1802 Interface */

// Destroyer

static void destryer_out_q(int level)
{
	// write to 1024x1bit PagememoryColorBit RAM
}

static int destryer_in_ef(void)
{
	/*
        EF1     ?
        EF2     ?
        EF3     ?
        EF4     ?
    */

	return readinputportbytag("EF") & 0x0f;
}

static CDP1802_CONFIG destryer_cdp1802_config =
{
	NULL,
	destryer_out_q,
	destryer_in_ef
};

// Altair

static void altair_out_q(int level)
{
	// write to 1024x1bit PagememoryColorBit RAM

	/*

        Character RAM mapping

        bit     description

        0       CCB0
        1       CCB1
        2       CDB0
        3       CDB1
        4       CDB2
        5       CDB3
        6       CDB4
        7       CDB5
        8       PCB

    */

//  UINT16 address = activecpu_get_reg(activecpu_get_reg(CDP1802_X) + CDP1802_R0) - 1; // TODO: why -1?
}

static int altair_in_ef(void)
{
	/*
        EF1     CDP1869 _PRD
        EF2     Test
        EF3     Coin 2
        EF4     Coin 1
    */

	return readinputportbytag("EF") & 0x0f;
}

static CDP1802_CONFIG altair_cdp1802_config =
{
	NULL,
	altair_out_q,
	altair_in_ef
};

// Draco

static void draco_out_q(int level)
{
	// write to 1024x1bit PagememoryColorBit RAM
}

static int draco_in_ef(void)
{
	/*
        EF1     CDP1869 _PRD
        EF2     Test
        EF3     Coin 2
        EF4     Coin 1
    */

	return readinputportbytag("EF") & 0x0f;
}

static CDP1802_CONFIG draco_cdp1802_config =
{
	NULL,
	draco_out_q,
	draco_in_ef
};

/* Interrupt Generators */

static INTERRUPT_GEN( cidelsa_frame_int )
{
}

/* Machine Driver */

static MACHINE_DRIVER_START( destryer )

	// basic system hardware

	MDRV_CPU_ADD(CDP1802, 3579000) // ???
	MDRV_CPU_PROGRAM_MAP(destryer_map, 0)
	MDRV_CPU_IO_MAP(destryer_io_map, 0)
	MDRV_CPU_VBLANK_INT(cidelsa_frame_int, 1)
	MDRV_CPU_CONFIG(destryer_cdp1802_config)

	MDRV_FRAMES_PER_SECOND(CDP1870_FPS_PAL)	// 50.09 Hz
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	// video hardware

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(CDP1870_SCREEN_WIDTH, CDP1870_SCANLINES_PAL)
	MDRV_VISIBLE_AREA(0, CDP1870_SCREEN_WIDTH-1, 0, CDP1870_SCANLINES_PAL-1)
	MDRV_PALETTE_LENGTH(8)
	MDRV_COLORTABLE_LENGTH(2*8)

	MDRV_PALETTE_INIT(cdp1869)
	MDRV_VIDEO_START(cdp1869)
	MDRV_VIDEO_UPDATE(cdp1869)

	// sound hardware

	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(CDP1869, 3579000) // ???
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( altair )

	// basic system hardware

	MDRV_CPU_ADD(CDP1802, 3579000)
	MDRV_CPU_PROGRAM_MAP(altair_map, 0)
	MDRV_CPU_IO_MAP(altair_io_map, 0)
	MDRV_CPU_VBLANK_INT(cidelsa_frame_int, 1)
	MDRV_CPU_CONFIG(altair_cdp1802_config)

	MDRV_FRAMES_PER_SECOND(CDP1870_FPS_PAL)	// 50.09 Hz
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	// video hardware

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(CDP1870_SCREEN_WIDTH, CDP1870_SCANLINES_PAL)
	MDRV_VISIBLE_AREA(0, CDP1870_SCREEN_WIDTH-1, 0, CDP1870_SCANLINES_PAL-1)
	MDRV_PALETTE_LENGTH(8)
	MDRV_COLORTABLE_LENGTH(2*8)

	MDRV_PALETTE_INIT(cdp1869)
	MDRV_VIDEO_START(cdp1869)
	MDRV_VIDEO_UPDATE(cdp1869)

	// sound hardware

	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(CDP1869, 3579000)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( draco )

	// basic system hardware

	MDRV_CPU_ADD(CDP1802, 3579000)
	MDRV_CPU_PROGRAM_MAP(draco_map, 0)
	MDRV_CPU_IO_MAP(draco_io_map, 0)
	MDRV_CPU_VBLANK_INT(cidelsa_frame_int, 1)
	MDRV_CPU_CONFIG(draco_cdp1802_config)

//  MDRV_CPU_ADD(COP402, 2012160)
//  MDRV_CPU_PROGRAM_MAP(draco_sound_map, 0)
//  MDRV_CPU_IO_MAP(draco_sound_io_map, 0)

	MDRV_FRAMES_PER_SECOND(CDP1870_FPS_PAL)	// 50.09 Hz
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	// video hardware

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(CDP1870_SCREEN_WIDTH, CDP1870_SCANLINES_PAL)
	MDRV_VISIBLE_AREA(0, CDP1870_SCREEN_WIDTH-1, 0, CDP1870_SCANLINES_PAL-1)
	MDRV_PALETTE_LENGTH(8)
	MDRV_COLORTABLE_LENGTH(2*8)

	MDRV_PALETTE_INIT(cdp1869)
	MDRV_VIDEO_START(cdp1869)
	MDRV_VIDEO_UPDATE(cdp1869)

	// sound hardware

	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(CDP1869, 3579000)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	MDRV_SOUND_ADD(AY8910, 2012160)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
MACHINE_DRIVER_END

/* ROMs */

ROM_START( destryer )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "des a 2.ic4", 0x0000, 0x0800, CRC(63749870) SHA1(a8eee4509d7a52dcf33049de221d928da3632174) )
	ROM_LOAD( "des b 2.ic5", 0x0800, 0x0800, CRC(60604f40) SHA1(32ca95c5b38b0f4992e04d77123d217f143ae084) )
	ROM_LOAD( "des c 2.ic6", 0x1000, 0x0800, CRC(a7cdeb7b) SHA1(a5a7748967d4ca89fb09632e1f0130ef050dbd68) )
	ROM_LOAD( "des d 2.ic7", 0x1800, 0x0800, CRC(dbec0aea) SHA1(1d9d49009a45612ee79763781a004499313b823b) )
ROM_END

ROM_START( altair )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "alt a 1.ic7",  0x0000, 0x0800, CRC(37c26c4e) SHA1(30df7efcf5bd12dafc1cb6e894fc18e7b76d3e61) )
	ROM_LOAD( "alt b 1.ic8",  0x0800, 0x0800, CRC(76b814a4) SHA1(e8ab1d1cbcef974d929ef8edd10008f60052a607) )
	ROM_LOAD( "alt c 1.ic9",  0x1000, 0x0800, CRC(2569ce44) SHA1(a09597d2f8f50fab9a09ed9a59c50a2bdcba47bb) )
	ROM_LOAD( "alt d 1.ic10", 0x1800, 0x0800, CRC(a25e6d11) SHA1(c197ff91bb9bdd04e88908259e4cde11b990e31d) )
	ROM_LOAD( "alt e 1.ic11", 0x2000, 0x0800, CRC(e497f23b) SHA1(6094e9873df7bd88c521ddc3fd63961024687243) )
	ROM_LOAD( "alt f 1.ic12", 0x2800, 0x0800, CRC(a06dd905) SHA1(c24ad9ff6d4e3b4e57fd75f946e8832fa00c2ea0) )
ROM_END

ROM_START( draco )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "dra a 1.ic10", 0x0000, 0x0800, CRC(ca127984) SHA1(46721cf42b1c891f7c88bc063a2149dd3cefea74) )
	ROM_LOAD( "dra b 1.ic11", 0x0800, 0x0800, CRC(e4936e28) SHA1(ddbbf769994d32a6bce75312306468a89033f0aa) )
	ROM_LOAD( "dra c 1.ic12", 0x1000, 0x0800, CRC(94480f5d) SHA1(8f49ce0f086259371e999d097a502482c83c6e9e) )
	ROM_LOAD( "dra d 1.ic13", 0x1800, 0x0800, CRC(32075277) SHA1(2afaa92c91f554e3bdcfec6d94ef82df63032afb) )
	ROM_LOAD( "dra e 1.ic14", 0x2000, 0x0800, CRC(cce7872e) SHA1(c956eb994452bd8a27bbc6d0e6d103e87a4a3e6e) )
	ROM_LOAD( "dra f 1.ic15", 0x2800, 0x0800, CRC(e5927ec7) SHA1(42e0aabb6187bbb189648859fd5dddda43814526) )
	ROM_LOAD( "dra g 1.ic16", 0x3000, 0x0800, CRC(f28546c0) SHA1(daedf1d64f94358b15580d697dd77d3c977aa22c) )
	ROM_LOAD( "dra h 1.ic17", 0x3800, 0x0800, CRC(dce782ea) SHA1(f558096f43fb30337bc4a527169718326c265c2c) )

	ROM_REGION( 0x800, REGION_CPU2, 0 )
	ROM_LOAD( "dra s 1.ic4",  0x0000, 0x0800, CRC(292a57f8) SHA1(b34a189394746d77c3ee669db24109ee945c3be7) )
ROM_END

/* Game Drivers */

GAME( 1980, destryer, 0, destryer, destryer, 0, ROT0, "Cidelsa", "Destroyer (Cidelsa)", GAME_NOT_WORKING )
GAME( 1981, altair,   0, altair,   altair,   0, ROT0, "Cidelsa", "Altair", GAME_NOT_WORKING )
GAME( 1981, draco,    0, draco,    draco,    0, ROT0, "Cidelsa", "Draco", GAME_NOT_WORKING )