/******************************************************************************

  MESS - device.c

  List of all available devices and Device handling interfaces.

******************************************************************************/

#include "driver.h"
#include "device.h"

/* The List of Devices, with Associated Names - Be careful to ensure that 	*/
/* this list matches the ENUM from device.h, so searches can use IO_COUNT	*/
const struct Devices devices[] =
{
	{IO_END,		"NONE",			"NONE"}, /*  0 */
	{IO_CARTSLOT,	"cartridge",	"cart"}, /*  1 */
	{IO_FLOPPY,		"floppydisk",	"flop"}, /*  2 */
	{IO_HARDDISK,	"harddisk",		"hard"}, /*  3 */
	{IO_CYLINDER,	"cylinder",		"cyln"}, /*  4 */
	{IO_CASSETTE,	"cassette",		"cass"}, /*  5 */
	{IO_PUNCHCARD,	"punchcard",	"pcrd"}, /*  6 */
	{IO_PUNCHTAPE,	"punchtape",	"ptap"}, /*  7 */
	{IO_PRINTER,	"printer",		"prin"}, /*  8 */
	{IO_SERIAL,		"serial",		"serl"}, /*  9 */
	{IO_PARALLEL,   "parallel",		"parl"}, /* 10 */
	{IO_SNAPSHOT,	"snapshot",		"dump"}, /* 11 */
	{IO_QUICKLOAD,	"quickload",	"quik"}, /* 12 */
	{IO_COUNT,		NULL,			NULL  }, /* 13 Always at end of this array! */
};


/* register_device() - used to register the device in the options struct...	*/
/* Call this from the CLI or UI to add a DEVICE (with its arg) to the 		*/
/* options struct.  Return 0 for success, -1 for error 						*/
int register_device (const int type, const char *arg)
{
	extern struct GameOptions options;

	/* Check the the device type is valid, otherwise this lookup will be bad*/
	if (type <= IO_END || type >= IO_COUNT || !type)
	{
		mess_printf("register_device() failed! - device type [%d] is not valid\n",type);
		return -1;
	}

	/* Next, check that we havent loaded too many images					*/
	if (options.image_count >= MAX_IMAGES)
	{
		mess_printf("Too many image names specified!\n");
		return -1;
	}

	/* All seems ok to add device and argument to options{} struct			*/
	#ifdef MAME_DEBUG
	mess_printf("Image [%s] Registered for Device [%s]\n", arg, device_typename(type));
	#endif
	/* the user specified a device type */
	options.image_files[options.image_count].type = type;
	options.image_files[options.image_count].name = strdup(arg);
	options.image_count++;
	return 0;

}
