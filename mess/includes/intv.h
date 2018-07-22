#ifndef __INTV_H
#define __INTV_H

/* in vidhrdw/intv.c */
extern VIDEO_START( intv );
extern VIDEO_UPDATE( intv );
extern VIDEO_START( intvkbd );
extern VIDEO_UPDATE( intvkbd );

/* in machine/intv.c */

/*  for the console alone... */
extern UINT8 intv_gramdirty;
extern UINT8 intv_gram[];
extern UINT8 intv_gramdirtybytes[];
extern UINT16 intv_ram16[];

extern DRIVER_INIT( intv );
int intv_load_rom (int id);

extern MACHINE_INIT( intv );
extern INTERRUPT_GEN( intv_interrupt );

READ16_HANDLER( intv_gram_r );
WRITE16_HANDLER( intv_gram_w );
READ16_HANDLER( intv_empty_r );
READ16_HANDLER( intv_ram8_r );
WRITE16_HANDLER( intv_ram8_w );
READ16_HANDLER( intv_ram16_r );
WRITE16_HANDLER( intv_ram16_w );

void stic_screenrefresh(void);

READ_HANDLER( intv_right_control_r );
READ_HANDLER( intv_left_control_r );

/* for the console + keyboard component... */
extern int intvkbd_text_blanked;

extern DRIVER_INIT( intvkbd );
int intvkbd_load_rom (int id);
int intvkbd_tape_init (int id);
void intvkbd_tape_exit (int id);

extern INTERRUPT_GEN( intvkbd_interrupt );
extern INTERRUPT_GEN( intvkbd_interrupt2 );

READ16_HANDLER ( intvkbd_dualport16_r );
WRITE16_HANDLER ( intvkbd_dualport16_w );
READ_HANDLER ( intvkbd_dualport8_lsb_r );
WRITE_HANDLER ( intvkbd_dualport8_lsb_w );
READ_HANDLER ( intvkbd_dualport8_msb_r );
WRITE_HANDLER ( intvkbd_dualport8_msb_w );

READ_HANDLER ( intvkbd_tms9927_r );
WRITE_HANDLER ( intvkbd_tms9927_w );

/* in sndhrdw/intv.c */
READ16_HANDLER( AY8914_directread_port_0_lsb_r );
WRITE16_HANDLER( AY8914_directwrite_port_0_lsb_w );

/* share this stuff for now */
struct tape_drive_state_type
{
	/* read state */
	int read_data;			/* 0x4000 */
	int ready;				/* 0x4001 */
	int leader_detect;		/* 0x4002 */
	int tape_missing;		/* 0x4003 */
	int playing;			/* 0x4004 */
	int no_data;			/* 0x4005 */

	/* write state */
	int motor_state;		/* 0x4020-0x4022 */
	int writing;			/* 0x4023 */
	int audio_b_mute;		/* 0x4024 */
	int audio_a_mute;		/* 0x4025 */
	int channel_select;		/* 0x4026 */
	int erase;				/* 0x4027 */
	int write_data;			/* 0x4040 */

	/* bit_counter */
	int bit_counter;
};

struct tape_drive_state_type *get_tape_drive_state(void);

#endif

