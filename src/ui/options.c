/***************************************************************************

  M.A.M.E.32  -  Multiple Arcade Machine Emulator for Win32
  Win32 Portions Copyright (C) 1997-2003 Michael Soderstrom and Chris Kirmse

  This file is part of MAME32, and may only be used, modified and
  distributed under the terms of the MAME license, in "readme.txt".
  By continuing to use, modify or distribute this file you indicate
  that you have read the license and understand and accept it fully.

 ***************************************************************************/
 
 /***************************************************************************

  options.c

  Stores global options and per-game options;

***************************************************************************/

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <windowsx.h>
#include <winreg.h>
#include <commctrl.h>
#include <assert.h>
#include <stdio.h>
#include <sys/stat.h>
#include <malloc.h>
#include <math.h>
#include <driver.h>
#include "mame32.h"
#include "m32util.h"
#include "resource.h"
#include "file.h"
#include "splitters.h"

/***************************************************************************
    Internal function prototypes
 ***************************************************************************/

static void  SaveOptions(void);
static void  LoadOptions(void);
static void  SaveGlobalOptions(BOOL bBackup);

static void  LoadRegGameOptions(HKEY hKey, options_type *o, int driver_index);
static DWORD GetRegOption(HKEY hKey, const char *name);
static void  GetRegBoolOption(HKEY hKey, const char *name, BOOL *value);
static char  *GetRegStringOption(HKEY hKey, const char *name);

static BOOL  SaveRegGameOptions(HKEY hKey, options_type *o);
static void  PutRegOption(HKEY hKey, const char *name, DWORD value);
static void  PutRegBoolOption(HKEY hKey, const char *name, BOOL value);
static void  PutRegStringOption(HKEY hKey, const char *name, char *option);

static void WriteStringOptionToFile(FILE *fptr,const char *key,const char *value);
static void WriteIntOptionToFile(FILE *fptr,const char *key,int value);
static void WriteBoolOptionToFile(FILE *fptr,const char *key,BOOL value);

static void WriteOptionToFile(FILE *fptr,REG_OPTION *regOpt);

static void  ColumnEncodeString(void* data, char* str);
static void  ColumnDecodeString(const char* str, void* data);

static void  ColumnDecodeWidths(const char *ptr, void* data);

static void  SplitterEncodeString(void* data, char* str);
static void  SplitterDecodeString(const char* str, void* data);

static void  ListEncodeString(void* data, char* str);
static void  ListDecodeString(const char* str, void* data);

static void  FontEncodeString(void* data, char* str);
static void  FontDecodeString(const char* str, void* data);

static void  SavePlayCount(int game_index);
static void  SaveFolderFlags(const char *folderName, DWORD dwFlags);

static void  PutRegObj(HKEY hKey, REG_OPTION *regOpt);
static void  GetRegObj(HKEY hKey, REG_OPTION *regOpt);

/***************************************************************************
    Internal defines
 ***************************************************************************/

/* Used to create/retrieve Registry database */
/* #define KEY_BASE "Software\\Freeware\\TestMame32" */

#define DOTBACKUP       ".Backup"
#define DOTDEFAULTS     ".Defaults"
#define DOTFOLDERS      ".Folders"

#define KEY_BASE        "Software\\Freeware\\" MAME32NAME

#define KEY_BASE_FMT         KEY_BASE "\\"
#define KEY_BASE_FMT_CCH     sizeof(KEY_BASE_FMT)-1

#define KEY_BACKUP      KEY_BASE "\\" DOTBACKUP "\\"
#define KEY_BACKUP_CCH  sizeof(KEY_BACKUP)-1

#define KEY_BASE_DOTBACKUP      KEY_BASE_FMT DOTBACKUP
#define KEY_BASE_DOTDEFAULTS    KEY_BASE_FMT DOTDEFAULTS
#define KEY_BASE_DOTFOLDERS     KEY_BASE_FMT DOTFOLDERS
#define KEY_BACKUP_DOTDEFAULTS      KEY_BACKUP DOTDEFAULTS

// #define MAME32UI_INI

/***************************************************************************
    Internal structures
 ***************************************************************************/

/***************************************************************************
    Internal variables
 ***************************************************************************/

static settings_type settings;

static options_type gOpts;  // Used when saving/loading from Registry
static options_type global; // Global 'default' options
static options_type *game_options;  // Array of Game specific options
static game_variables_type *game_variables;  // Array of game specific extra data

/* Global UI options */
static REG_OPTION regSettings[] =
{
	{"DefaultGame",        RO_STRING,  &settings.default_game,      0, 0},
#ifdef MESS
	{"DefaultSoftware", RO_PSTRING, &settings.default_software, 0, 0},
#endif
	{"FolderID",           RO_INT,     &settings.folder_id,        0, 0},
	{"ShowScreenShot",     RO_BOOL,    &settings.show_screenshot,  0, 0},
	{"ShowFlyer",          RO_INT,     &settings.show_pict_type,   0, 0},
	{"ShowToolBar",        RO_BOOL,    &settings.show_toolbar,     0, 0},
	{"ShowStatusBar",      RO_BOOL,    &settings.show_statusbar,   0, 0},
	{"ShowFolderList",     RO_BOOL,    &settings.show_folderlist,  0, 0},
	{"ShowTabCtrl",        RO_BOOL,    &settings.show_tabctrl,     0, 0},
	{"GameCheck",          RO_BOOL,    &settings.game_check,       0, 0},
	{"VersionCheck",       RO_BOOL,    &settings.version_check,    0, 0},
	{"JoyGUI",             RO_BOOL,    &settings.use_joygui,       0, 0},
	{"Broadcast",          RO_BOOL,    &settings.broadcast,        0, 0},
	{"Random_Bg",          RO_BOOL,    &settings.random_bg,        0, 0},

	{"SortColumn",         RO_INT,     &settings.sort_column,      0, 0},
	{"SortReverse",        RO_BOOL,    &settings.sort_reverse,     0, 0},
	{"X",                  RO_INT,     &settings.area.x,           0, 0},
	{"Y",                  RO_INT,     &settings.area.y,           0, 0},
	{"Width",              RO_INT,     &settings.area.width,       0, 0},
	{"Height",             RO_INT,     &settings.area.height,      0, 0},
	{"State",              RO_INT,     &settings.windowstate,      0, 0},

	{"ShowDisclaimer",     RO_BOOL,    &settings.show_disclaimer,  0, 0},
	{"ShowGameInfo",       RO_BOOL,    &settings.show_gameinfo,    0, 0},

	{"Language",           RO_PSTRING, &settings.language,         0, 0},
	{"FlyerDir",           RO_PSTRING, &settings.flyerdir,         0, 0},
	{"CabinetDir",         RO_PSTRING, &settings.cabinetdir,       0, 0},
	{"MarqueeDir",         RO_PSTRING, &settings.marqueedir,       0, 0},
	{"TitlesDir",          RO_PSTRING, &settings.titlesdir,        0, 0},
	{"BkgroundDir",        RO_PSTRING, &settings.bgdir,            0, 0},

#ifdef MESS
	{"SoftwareDirs",       RO_PSTRING, &settings.softwaredirs,     0, 0},
	{"crc_directory",      RO_PSTRING, &settings.crcdir,           0, 0},
#endif

	{"rompath",            RO_PSTRING, &settings.romdirs,          0, 0},
	{"samplepath",         RO_PSTRING, &settings.sampledirs,       0, 0},
	{"inipath",			   RO_PSTRING, &settings.inidirs,          0, 0},
	{"cfg_directory",      RO_PSTRING, &settings.cfgdir,           0, 0},
	{"nvram_directory",    RO_PSTRING, &settings.nvramdir,         0, 0},
	{"memcard_directory",  RO_PSTRING, &settings.memcarddir,       0, 0},
	{"input_directory",    RO_PSTRING, &settings.inpdir,           0, 0},
	{"hiscore_directory",  RO_PSTRING, &settings.hidir,            0, 0},
	{"state_directory",    RO_PSTRING, &settings.statedir,         0, 0},
	{"artwork_directory",  RO_PSTRING, &settings.artdir,           0, 0},
	{"snapshot_directory", RO_PSTRING, &settings.imgdir,           0, 0},
	{"diff_directory",     RO_PSTRING, &settings.diffdir,          0, 0},
	{"icons_directory",    RO_PSTRING, &settings.iconsdir,         0, 0},
	{"cheat_file",         RO_PSTRING, &settings.cheat_filename,   0, 0},
	{"history_file",       RO_PSTRING, &settings.history_filename, 0, 0},
	{"mameinfo_file",      RO_PSTRING, &settings.mameinfo_filename,0, 0},
	{"ctrlr_directory",    RO_PSTRING, &settings.ctrlrdir,         0, 0},
	{"folder_directory",   RO_PSTRING, &settings.folderdir,        0, 0},

	/* ListMode needs to be before ColumnWidths settings */
	{"ListMode",           RO_ENCODE,  &settings.view,             ListEncodeString,     ListDecodeString},
	{"Splitters",          RO_ENCODE,  settings.splitter,          SplitterEncodeString, SplitterDecodeString},
	{"ListFont",           RO_ENCODE,  &settings.list_font,        FontEncodeString,     FontDecodeString},
	{"ColumnWidths",       RO_ENCODE,  &settings.column_width,     ColumnEncodeString,   ColumnDecodeWidths},
	{"ColumnOrder",        RO_ENCODE,  &settings.column_order,     ColumnEncodeString,   ColumnDecodeString},
	{"ColumnShown",        RO_ENCODE,  &settings.column_shown,     ColumnEncodeString,   ColumnDecodeString},
#ifdef MESS
	{"MessColumnWidths",RO_ENCODE,  &settings.mess_column_width,MessColumnEncodeString, MessColumnDecodeWidths},
	{"MessColumnOrder", RO_ENCODE,  &settings.mess_column_order,MessColumnEncodeString, MessColumnDecodeString},
	{"MessColumnShown", RO_ENCODE,  &settings.mess_column_shown,MessColumnEncodeString, MessColumnDecodeString}
#endif
};

/* Game Options */
static REG_OPTION regGameOpts[] =
{
	/* video */
	{ "autoframeskip",          RO_BOOL,    &gOpts.autoframeskip,     0, 0},
	{ "frameskip",              RO_INT,     &gOpts.frameskip,         0, 0},
	{ "waitvsync",              RO_BOOL,    &gOpts.wait_vsync,        0, 0},
	{ "triplebuffer",           RO_BOOL,    &gOpts.use_triplebuf,     0, 0},
	{ "window",                 RO_BOOL,    &gOpts.window_mode,       0, 0},
	{ "ddraw",                  RO_BOOL,    &gOpts.use_ddraw,         0, 0},
	{ "hwstretch",              RO_BOOL,    &gOpts.ddraw_stretch,     0, 0},
	{ "resolution",             RO_STRING,  &gOpts.resolution,        0, 0},
	{ "refresh",                RO_INT,     &gOpts.gfx_refresh,       0, 0},
	{ "scanlines",              RO_BOOL,    &gOpts.scanlines,         0, 0},
	{ "switchres",              RO_BOOL,    &gOpts.switchres,         0, 0},
	{ "switchbpp",              RO_BOOL,    &gOpts.switchbpp,         0, 0},
	{ "maximize",               RO_BOOL,    &gOpts.maximize,          0, 0},
	{ "keepaspect",             RO_BOOL,    &gOpts.keepaspect,        0, 0},
	{ "matchrefresh",           RO_BOOL,    &gOpts.matchrefresh,      0, 0},
	{ "syncrefresh",            RO_BOOL,    &gOpts.syncrefresh,       0, 0},
	{ "throttle",               RO_BOOL,    &gOpts.throttle,          0, 0},
	{ "full_screen_brightness", RO_DOUBLE,  &gOpts.gfx_brightness,    0, 0},
	{ "frames_to_run",          RO_INT,     &gOpts.frames_to_display, 0, 0},
	{ "effect",                 RO_STRING,  &gOpts.effect,            0, 0},
	{ "screen_aspect",          RO_STRING,  &gOpts.aspect,            0, 0},

	/* input */
	{ "hotrod",                 RO_BOOL,    &gOpts.hotrod,            0, 0},
	{ "hotrodse",               RO_BOOL,    &gOpts.hotrodse,          0, 0},
	{ "mouse",                  RO_BOOL,    &gOpts.use_mouse,         0, 0},
	{ "joystick",               RO_BOOL,    &gOpts.use_joystick,      0, 0},
	{ "a2d",                    RO_DOUBLE,  &gOpts.f_a2d,             0, 0},
	{ "steadykey",              RO_BOOL,    &gOpts.steadykey,         0, 0},
	{ "lightgun",               RO_BOOL,    &gOpts.lightgun,          0, 0},
	{ "ctrlr",                  RO_STRING,  &gOpts.ctrlr,             0, 0},

	/* core video */
	{ "brightness",             RO_DOUBLE,  &gOpts.f_bright_correct,  0, 0}, 
	{ "pause_brightness",       RO_DOUBLE,  &gOpts.f_pause_bright    ,0, 0}, 
	{ "norotate",               RO_BOOL,    &gOpts.norotate,          0, 0},
	{ "ror",                    RO_BOOL,    &gOpts.ror,               0, 0},
	{ "rol",                    RO_BOOL,    &gOpts.rol,               0, 0},
	{ "flipx",                  RO_BOOL,    &gOpts.flipx,             0, 0},
	{ "flipy",                  RO_BOOL,    &gOpts.flipy,             0, 0},
	{ "debug_resolution",       RO_STRING,  &gOpts.debugres,          0, 0}, 
	{ "gamma",                  RO_DOUBLE,  &gOpts.f_gamma_correct,   0, 0},

	/* vector */
	{ "antialias",              RO_BOOL,    &gOpts.antialias,         0, 0},
	{ "translucency",           RO_BOOL,    &gOpts.translucency,      0, 0},
	{ "beam",                   RO_DOUBLE,  &gOpts.f_beam,            0, 0},
	{ "flicker",                RO_DOUBLE,  &gOpts.f_flicker,         0, 0},
	{ "intensity",              RO_DOUBLE,  &gOpts.f_intensity,       0, 0},

	/* sound */
	{ "samplerate",             RO_INT,     &gOpts.samplerate,        0, 0},
	{ "use_samples",            RO_BOOL,    &gOpts.use_samples,       0, 0},
	{ "resamplefilter",         RO_BOOL,    &gOpts.use_filter,        0, 0},
	{ "sound",                  RO_BOOL,    &gOpts.enable_sound,      0, 0},
	{ "volume",                 RO_INT,     &gOpts.attenuation,       0, 0},

	/* misc artwork options */
	{ "artwork",                RO_BOOL,    &gOpts.use_artwork,       0, 0},
	{ "backdrops",              RO_BOOL,    &gOpts.backdrops,         0, 0},
	{ "overlays",               RO_BOOL,    &gOpts.overlays,          0, 0},
	{ "bezels",                 RO_BOOL,    &gOpts.bezels,            0, 0},
	{ "artwork_crop",           RO_BOOL,    &gOpts.artwork_crop,      0, 0},
	{ "artres",                 RO_INT,     &gOpts.artres,            0, 0},

	/* misc */
	{ "cheat",                  RO_BOOL,    &gOpts.cheat,             0, 0},
	{ "debug",                  RO_BOOL,    &gOpts.mame_debug,        0, 0},
/*	{ "playback",               RO_STRING,  &gOpts.playbackname,      0, 0},*/
/*	{ "record",                 RO_STRING,  &gOpts.recordname,        0, 0},*/
	{ "log",                    RO_BOOL,    &gOpts.errorlog,          0, 0},
	{ "sleep",                  RO_BOOL,    &gOpts.sleep,             0, 0},
	{ "old_timing",             RO_BOOL,    &gOpts.old_timing,        0, 0},
	{ "leds",                   RO_BOOL,    &gOpts.leds,              0, 0}

#ifdef MESS
	/* mess options */
	,
	{ "extra_software",			RO_STRING,	&gOpts.extra_software_paths,	0, 0},
	{ "printer",				RO_STRING,	&gOpts.printer,					0, 0},
	{ "use_new_ui",				RO_BOOL,	&gOpts.use_new_ui,				0, 0},
	{ "ram_size",				RO_INT,		&gOpts.ram_size,				0, 0}
#endif
};

#define NUM_SETTINGS (sizeof(regSettings) / sizeof(regSettings[0]))
#define NUM_GAMEOPTS (sizeof(regGameOpts) / sizeof(regGameOpts[0]))

static int  num_games = 0;
static BOOL bResetGUI      = FALSE;
static BOOL bResetGameDefs = FALSE;

/* Default sizes based on 8pt font w/sort arrow in that column */
static int default_column_width[] = { 187, 68, 84, 84, 64, 88, 74,108, 60,144, 84 };
static int default_column_shown[] = {   1,  0,  1,  1,  1,  1,  1,  1,  1,  1,  0 };
/* Hidden columns need to go at the end of the order array */
static int default_column_order[] = {   0,  2,  3,  4,  5,  6,  7,  8,  9,  1, 10 };

static const char *view_modes[VIEW_MAX] = { "Large Icons", "Small Icons", "List", "Details", "Grouped" };

static char oldInfoMsg[400] = 
MAME32NAME " has detected outdated configuration data.\n\n\
The detected configuration data is from Version %s of " MAME32NAME ".\n\
The current version is %s. It is recommended that the\n\
configuration is set to the new defaults.\n\n\
Would you like to use the new configuration?";

extern const char g_szDefaultGame[];

/***************************************************************************
    External functions  
 ***************************************************************************/

void OptionsInit()
{
	int i;

	num_games = GetNumGames();

	strcpy(settings.default_game, g_szDefaultGame);
#ifdef MESS
	settings.default_software = NULL;
#endif
	settings.folder_id       = 0;
	settings.view            = VIEW_GROUPED;
	settings.show_folderlist = TRUE;
	settings.show_toolbar    = TRUE;
	settings.show_statusbar  = TRUE;
	settings.show_screenshot = TRUE;
	settings.show_tabctrl    = TRUE;
	settings.game_check      = TRUE;
	settings.version_check   = TRUE;
	settings.use_joygui      = FALSE;
	settings.broadcast       = FALSE;
	settings.random_bg       = FALSE;

	for (i = 0; i < COLUMN_MAX; i++)
	{
		settings.column_width[i] = default_column_width[i];
		settings.column_order[i] = default_column_order[i];
		settings.column_shown[i] = default_column_shown[i];
	}

#ifdef MESS
	for (i = 0; i < MESS_COLUMN_MAX; i++)
	{
		settings.mess_column_width[i] = default_mess_column_width[i];
		settings.mess_column_order[i] = default_mess_column_order[i];
		settings.mess_column_shown[i] = default_mess_column_shown[i];
	}
#endif
	settings.sort_column = 0;
	settings.sort_reverse= FALSE;
	settings.area.x      = 0;
	settings.area.y      = 0;
	settings.area.width  = 640;
	settings.area.height = 400;
	settings.windowstate = 1;
	settings.splitter[0] = 150;
	settings.splitter[1] = 362;
#ifdef MESS
	/* an algorithm to adjust for the fact that we need a larger window for the
	 * software picker
	 */
	settings.splitter[1] -= (settings.splitter[1] - settings.splitter[0]) / 4;
	settings.area.width += settings.splitter[1] - settings.splitter[0];
	settings.splitter[2] = settings.splitter[1] + (settings.splitter[1] - settings.splitter[0]);
#endif

	settings.language          = strdup("english");
	settings.flyerdir          = strdup("flyers");
	settings.cabinetdir        = strdup("cabinets");
	settings.marqueedir        = strdup("marquees");
	settings.titlesdir         = strdup("titles");

#ifdef MESS
	settings.romdirs           = strdup("bios");
#else
	settings.romdirs           = strdup("roms");
#endif
	settings.sampledirs        = strdup("samples");
#ifdef MESS
	settings.softwaredirs      = strdup("software");
	settings.crcdir            = strdup("crc");
#endif
	settings.inidirs		   = strdup("ini");
	settings.cfgdir            = strdup("cfg");
	settings.nvramdir          = strdup("nvram");
	settings.memcarddir        = strdup("memcard");
	settings.inpdir            = strdup("inp");
	settings.hidir             = strdup("hi");
	settings.statedir          = strdup("sta");
	settings.artdir            = strdup("artwork");
	settings.imgdir            = strdup("snap");
	settings.diffdir           = strdup("diff");
	settings.iconsdir          = strdup("icons");
	settings.bgdir             = strdup("bkground");
	settings.cheat_filename    = strdup("cheat.dat");
#ifdef MESS
	settings.history_filename  = strdup("sysinfo.dat");
	settings.mameinfo_filename = strdup("messinfo.dat");
#else
	settings.history_filename  = strdup("history.dat");
	settings.mameinfo_filename = strdup("mameinfo.dat");
#endif
	settings.ctrlrdir          = strdup("ctrlr");
	settings.folderdir         = strdup("folders");

	settings.list_font.lfHeight         = -8;
	settings.list_font.lfWidth          = 0;
	settings.list_font.lfEscapement     = 0;
	settings.list_font.lfOrientation    = 0;
	settings.list_font.lfWeight         = FW_NORMAL;
	settings.list_font.lfItalic         = FALSE;
	settings.list_font.lfUnderline      = FALSE;
	settings.list_font.lfStrikeOut      = FALSE;
	settings.list_font.lfCharSet        = ANSI_CHARSET;
	settings.list_font.lfOutPrecision   = OUT_DEFAULT_PRECIS;
	settings.list_font.lfClipPrecision  = CLIP_DEFAULT_PRECIS;
	settings.list_font.lfQuality        = DEFAULT_QUALITY;
	settings.list_font.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;

	strcpy(settings.list_font.lfFaceName, "MS Sans Serif");

	settings.list_font_color = (COLORREF)-1;
	settings.list_clone_color = (COLORREF)-1;

	settings.show_disclaimer = TRUE;
	settings.show_gameinfo = TRUE;

	global.use_default = FALSE;

	/* video */
	global.autoframeskip     = TRUE;
	global.frameskip         = 0;
	global.wait_vsync        = FALSE;
	global.use_triplebuf     = FALSE;
	global.window_mode       = FALSE;
	global.use_ddraw         = TRUE;
	global.ddraw_stretch     = TRUE;
	strcpy(global.resolution, "auto");
	global.gfx_refresh       = 0;
	global.scanlines         = FALSE;
	global.switchres         = TRUE;
	global.switchbpp         = TRUE;
	global.maximize          = TRUE;
	global.keepaspect        = TRUE;
	global.matchrefresh      = FALSE;
	global.syncrefresh       = FALSE;
	global.throttle          = TRUE;
	global.gfx_brightness    = 1.0;
	global.frames_to_display = 0;
	strcpy(global.effect,    "none");
	strcpy(global.aspect,    "4:3");

	/* input */
	global.hotrod            = FALSE;
	global.hotrodse          = FALSE;
	global.use_mouse         = FALSE;
	global.use_joystick      = FALSE;
	global.f_a2d             = 0.3;
	global.steadykey         = FALSE;
	global.lightgun          = FALSE;
	strcpy(global.ctrlr,     "Standard");

	/* Core video */
	global.f_bright_correct  = 1.0;
	global.f_pause_bright    = 0.65;
	global.norotate          = FALSE;
	global.ror               = FALSE;
	global.rol               = FALSE;
	global.flipx             = FALSE;
	global.flipy             = FALSE;
	strcpy(global.debugres, "auto");
	global.f_gamma_correct   = 1.0;

	/* Core vector */
	global.antialias         = TRUE;
	global.translucency      = TRUE;
	global.f_beam            = 1.0;
	global.f_flicker         = 0.0;
	global.f_intensity		 = 1.5;

	/* Sound */
	global.samplerate        = 44100;
	global.use_samples       = TRUE;
	global.use_filter        = TRUE;
	global.enable_sound      = TRUE;
	global.attenuation       = 0;

	/* misc artwork options */
	global.use_artwork       = TRUE;
	global.backdrops         = TRUE;
	global.overlays          = TRUE;
	global.bezels            = TRUE;
	global.artwork_crop      = FALSE;
	global.artres            = 0; /* auto */

	/* misc */
	global.cheat             = FALSE;
	global.mame_debug        = FALSE;
	global.playbackname      = NULL;
	global.recordname        = NULL;
	global.errorlog          = FALSE;
	global.sleep             = FALSE;
	global.old_timing        = FALSE;
	global.leds				 = TRUE;


#ifdef MESS
	memset(global.extra_software_paths, '\0', sizeof(global.extra_software_paths));
	global.use_new_ui = TRUE;
#endif

	game_options = (options_type *)malloc(num_games * sizeof(options_type));
	game_variables = (game_variables_type *)malloc(num_games * sizeof(game_variables_type));

	for (i = 0; i < num_games; i++)
	{
		game_options[i] = global;
		game_options[i].use_default = TRUE;

		game_variables[i].play_count = 0;
		game_variables[i].has_roms = UNKNOWN;
		game_variables[i].has_samples = UNKNOWN;
	}

	SaveGlobalOptions(TRUE);
	LoadOptions();

	// have our mame core (file code) know about our rom path
	// this leaks a little, but the win32 file core writes to this string
	set_pathlist(FILETYPE_ROM,strdup(settings.romdirs));
	set_pathlist(FILETYPE_SAMPLE,strdup(settings.sampledirs));
#ifdef MESS
	set_pathlist(FILETYPE_CRC,strdup(settings.crcdir));
#endif
}

void OptionsExit(void)
{
    SaveOptions();

    free(game_options);

    FreeIfAllocated(&settings.language);
    FreeIfAllocated(&settings.romdirs);
    FreeIfAllocated(&settings.sampledirs);
    FreeIfAllocated(&settings.inidirs);
    FreeIfAllocated(&settings.cfgdir);
    FreeIfAllocated(&settings.hidir);
    FreeIfAllocated(&settings.inpdir);
    FreeIfAllocated(&settings.imgdir);
    FreeIfAllocated(&settings.statedir);
    FreeIfAllocated(&settings.artdir);
    FreeIfAllocated(&settings.memcarddir);
    FreeIfAllocated(&settings.flyerdir);
    FreeIfAllocated(&settings.cabinetdir);
    FreeIfAllocated(&settings.marqueedir);
    FreeIfAllocated(&settings.titlesdir);
    FreeIfAllocated(&settings.nvramdir);
    FreeIfAllocated(&settings.diffdir);
    FreeIfAllocated(&settings.iconsdir);
    FreeIfAllocated(&settings.bgdir);
	FreeIfAllocated(&settings.cheat_filename);
	FreeIfAllocated(&settings.history_filename);
	FreeIfAllocated(&settings.mameinfo_filename);
    FreeIfAllocated(&settings.ctrlrdir);
	FreeIfAllocated(&settings.folderdir);
}

options_type * GetDefaultOptions(void)
{
	return &global;
}

options_type * GetGameOptions(int driver_index)
{
	assert(0 <= driver_index && driver_index < num_games);

	if (game_options[driver_index].use_default)
	{
		game_options[driver_index] = global;
		game_options[driver_index].use_default	= TRUE;
	}
	return &game_options[driver_index];
}

void ResetGUI(void)
{
	bResetGUI = TRUE;
}

void SetViewMode(int val)
{
	settings.view = val;
}

int GetViewMode(void)
{
	return settings.view;
}

void SetGameCheck(BOOL game_check)
{
	settings.game_check = game_check;
}

BOOL GetGameCheck(void)
{
	return settings.game_check;
}

void SetVersionCheck(BOOL version_check)
{
	settings.version_check = version_check;
}

BOOL GetVersionCheck(void)
{
	return settings.version_check;
}

void SetJoyGUI(BOOL use_joygui)
{
	settings.use_joygui = use_joygui;
}

BOOL GetJoyGUI(void)
{
	return settings.use_joygui;
}

void SetBroadcast(BOOL broadcast)
{
	settings.broadcast = broadcast;
}

BOOL GetBroadcast(void)
{
	return settings.broadcast;
}

void SetShowDisclaimer(BOOL show_disclaimer)
{
	settings.show_disclaimer = show_disclaimer;
}

BOOL GetShowDisclaimer(void)
{
	return settings.show_disclaimer;
}

void SetShowGameInfo(BOOL show_gameinfo)
{
	settings.show_gameinfo = show_gameinfo;
}

BOOL GetShowGameInfo(void)
{
	return settings.show_gameinfo;
}

void SetRandomBackground(BOOL random_bg)
{
	settings.random_bg = random_bg;
}

BOOL GetRandomBackground(void)
{
	return settings.random_bg;
}

void SetSavedFolderID(UINT val)
{
	settings.folder_id = val;
}

UINT GetSavedFolderID(void)
{
	return settings.folder_id;
}

void SetShowScreenShot(BOOL val)
{
	settings.show_screenshot = val;
}

BOOL GetShowScreenShot(void)
{
	return settings.show_screenshot;
}

void SetShowFolderList(BOOL val)
{
	settings.show_folderlist = val;
}

BOOL GetShowFolderList(void)
{
	return settings.show_folderlist;
}

void SetShowStatusBar(BOOL val)
{
	settings.show_statusbar = val;
}

BOOL GetShowStatusBar(void)
{
	return settings.show_statusbar;
}

void SetShowTabCtrl (BOOL val)
{
	settings.show_tabctrl = val;
}

BOOL GetShowTabCtrl (void)
{
	return settings.show_tabctrl;
}

void SetShowToolBar(BOOL val)
{
	settings.show_toolbar = val;
}

BOOL GetShowToolBar(void)
{
	return settings.show_toolbar;
}

void SetShowPictType(int val)
{
	settings.show_pict_type = val;
}

int GetShowPictType(void)
{
	return settings.show_pict_type;
}

void SetDefaultGame(const char *name)
{
	strcpy(settings.default_game,name);
}

const char *GetDefaultGame(void)
{
	return settings.default_game;
}

void SetWindowArea(AREA *area)
{
	memcpy(&settings.area, area, sizeof(AREA));
}

void GetWindowArea(AREA *area)
{
	memcpy(area, &settings.area, sizeof(AREA));
}

void SetWindowState(UINT state)
{
	settings.windowstate = state;
}

UINT GetWindowState(void)
{
	return settings.windowstate;
}

void SetListFont(LOGFONT *font)
{
	memcpy(&settings.list_font, font, sizeof(LOGFONT));
}

void GetListFont(LOGFONT *font)
{
	memcpy(font, &settings.list_font, sizeof(LOGFONT));
}

void SetListFontColor(COLORREF uColor)
{
	if (settings.list_font_color == GetSysColor(COLOR_WINDOWTEXT))
		settings.list_font_color = (COLORREF)-1;
	else
		settings.list_font_color = uColor;
}

COLORREF GetListFontColor(void)
{
	if (settings.list_font_color == (COLORREF)-1)
		return (GetSysColor(COLOR_WINDOWTEXT));

	return settings.list_font_color;
}

void SetListCloneColor(COLORREF uColor)
{
	if (settings.list_clone_color == GetSysColor(COLOR_WINDOWTEXT))
		settings.list_clone_color = (COLORREF)-1;
	else
		settings.list_clone_color = uColor;
}

COLORREF GetListCloneColor(void)
{
	if (settings.list_clone_color == (COLORREF)-1)
		return (GetSysColor(COLOR_WINDOWTEXT));

	return settings.list_clone_color;

}

void SetColumnWidths(int width[])
{
	int i;

	for (i = 0; i < COLUMN_MAX; i++)
		settings.column_width[i] = width[i];
}

void GetColumnWidths(int width[])
{
	int i;

	for (i = 0; i < COLUMN_MAX; i++)
		width[i] = settings.column_width[i];
}

void SetSplitterPos(int splitterId, int pos)
{
	if (splitterId < GetSplitterCount())
		settings.splitter[splitterId] = pos;
}

int  GetSplitterPos(int splitterId)
{
	if (splitterId < GetSplitterCount())
		return settings.splitter[splitterId];

	return -1; /* Error */
}

void SetColumnOrder(int order[])
{
	int i;

	for (i = 0; i < COLUMN_MAX; i++)
		settings.column_order[i] = order[i];
}

void GetColumnOrder(int order[])
{
	int i;

	for (i = 0; i < COLUMN_MAX; i++)
		order[i] = settings.column_order[i];
}

void SetColumnShown(int shown[])
{
	int i;

	for (i = 0; i < COLUMN_MAX; i++)
		settings.column_shown[i] = shown[i];
}

void GetColumnShown(int shown[])
{
	int i;

	for (i = 0; i < COLUMN_MAX; i++)
		shown[i] = settings.column_shown[i];
}

void SetSortColumn(int column)
{
	settings.sort_column = column;
}

int GetSortColumn(void)
{
	return settings.sort_column;
}

void SetSortReverse(BOOL reverse)
{
	settings.sort_reverse = reverse;
}

BOOL GetSortReverse(void)
{
	return settings.sort_reverse;
}

const char* GetLanguage(void)
{
	return settings.language;
}

void SetLanguage(const char* lang)
{
	FreeIfAllocated(&settings.language);

	if (lang != NULL)
		settings.language = strdup(lang);
}

const char* GetRomDirs(void)
{
	return settings.romdirs;
}

void SetRomDirs(const char* paths)
{
	FreeIfAllocated(&settings.romdirs);

	if (paths != NULL)
	{
		settings.romdirs = strdup(paths);

		// have our mame core (file code) know about it
		// this leaks a little, but the win32 file core writes to this string
		set_pathlist(FILETYPE_ROM,strdup(settings.romdirs));
	}
}

const char* GetSampleDirs(void)
{
	return settings.sampledirs;
}

void SetSampleDirs(const char* paths)
{
	FreeIfAllocated(&settings.sampledirs);

	if (paths != NULL)
	{
		settings.sampledirs = strdup(paths);
		
		// have our mame core (file code) know about it
		// this leaks a little, but the win32 file core writes to this string
		set_pathlist(FILETYPE_SAMPLE,strdup(settings.sampledirs));
	}

}

const char* GetIniDirs(void)
{
	return settings.inidirs;
}

void SetIniDirs(const char* paths)
{
	FreeIfAllocated(&settings.inidirs);

	if (paths != NULL)
		settings.inidirs = strdup(paths);
}

const char* GetCtrlrDir(void)
{
	return settings.ctrlrdir;
}

void SetCtrlrDir(const char* path)
{
	FreeIfAllocated(&settings.ctrlrdir);

	if (path != NULL)
		settings.ctrlrdir = strdup(path);
}

const char* GetCfgDir(void)
{
	return settings.cfgdir;
}

void SetCfgDir(const char* path)
{
	FreeIfAllocated(&settings.cfgdir);

	if (path != NULL)
		settings.cfgdir = strdup(path);
}

const char* GetHiDir(void)
{
	return settings.hidir;
}

void SetHiDir(const char* path)
{
	FreeIfAllocated(&settings.hidir);

	if (path != NULL)
		settings.hidir = strdup(path);
}

const char* GetNvramDir(void)
{
	return settings.nvramdir;
}

void SetNvramDir(const char* path)
{
	FreeIfAllocated(&settings.nvramdir);

	if (path != NULL)
		settings.nvramdir = strdup(path);
}

const char* GetInpDir(void)
{
	return settings.inpdir;
}

void SetInpDir(const char* path)
{
	FreeIfAllocated(&settings.inpdir);

	if (path != NULL)
		settings.inpdir = strdup(path);
}

const char* GetImgDir(void)
{
	return settings.imgdir;
}

void SetImgDir(const char* path)
{
	FreeIfAllocated(&settings.imgdir);

	if (path != NULL)
		settings.imgdir = strdup(path);
}

const char* GetStateDir(void)
{
	return settings.statedir;
}

void SetStateDir(const char* path)
{
	FreeIfAllocated(&settings.statedir);

	if (path != NULL)
		settings.statedir = strdup(path);
}

const char* GetArtDir(void)
{
	return settings.artdir;
}

void SetArtDir(const char* path)
{
	FreeIfAllocated(&settings.artdir);

	if (path != NULL)
		settings.artdir = strdup(path);
}

const char* GetMemcardDir(void)
{
	return settings.memcarddir;
}

void SetMemcardDir(const char* path)
{
	FreeIfAllocated(&settings.memcarddir);

	if (path != NULL)
		settings.memcarddir = strdup(path);
}

const char* GetFlyerDir(void)
{
	return settings.flyerdir;
}

void SetFlyerDir(const char* path)
{
	FreeIfAllocated(&settings.flyerdir);

	if (path != NULL)
		settings.flyerdir = strdup(path);
}

const char* GetCabinetDir(void)
{
	return settings.cabinetdir;
}

void SetCabinetDir(const char* path)
{
	FreeIfAllocated(&settings.cabinetdir);

	if (path != NULL)
		settings.cabinetdir = strdup(path);
}

const char* GetMarqueeDir(void)
{
	return settings.marqueedir;
}

void SetMarqueeDir(const char* path)
{
	FreeIfAllocated(&settings.marqueedir);

	if (path != NULL)
		settings.marqueedir = strdup(path);
}

const char* GetTitlesDir(void)
{
	return settings.titlesdir;
}

void SetTitlesDir(const char* path)
{
	FreeIfAllocated(&settings.titlesdir);

	if (path != NULL)
		settings.titlesdir = strdup(path);
}

const char* GetDiffDir(void)
{
	return settings.diffdir;
}

void SetDiffDir(const char* path)
{
	FreeIfAllocated(&settings.diffdir);

	if (path != NULL)
		settings.diffdir = strdup(path);
}

const char* GetIconsDir(void)
{
	return settings.iconsdir;
}

void SetIconsDir(const char* path)
{
	FreeIfAllocated(&settings.iconsdir);

	if (path != NULL)
		settings.iconsdir = strdup(path);
}

const char* GetBgDir (void)
{
	return settings.bgdir;
}

void SetBgDir (const char* path)
{
	FreeIfAllocated(&settings.bgdir);

	if (path != NULL)
		settings.bgdir = strdup (path);
}

const char* GetFolderDir(void)
{
	return settings.folderdir;
}

void SetFolderDir(const char* path)
{
	FreeIfAllocated(&settings.folderdir);

	if (path != NULL)
		settings.folderdir = strdup(path);
}

const char* GetCheatFileName(void)
{
	return settings.cheat_filename;
}

void SetCheatFileName(const char* path)
{
	FreeIfAllocated(&settings.cheat_filename);

	if (path != NULL)
		settings.cheat_filename = strdup(path);
}

const char* GetHistoryFileName(void)
{
	return settings.history_filename;
}

void SetHistoryFileName(const char* path)
{
	FreeIfAllocated(&settings.history_filename);

	if (path != NULL)
		settings.history_filename = strdup(path);
}


const char* GetMAMEInfoFileName(void)
{
	return settings.mameinfo_filename;
}

void SetMAMEInfoFileName(const char* path)
{
	FreeIfAllocated(&settings.mameinfo_filename);

	if (path != NULL)
		settings.mameinfo_filename = strdup(path);
}

void ResetGameOptions(int driver_index)
{
	assert(0 <= driver_index && driver_index < num_games);

	game_options[driver_index] = global;
	game_options[driver_index].use_default = TRUE;
}

void ResetGameDefaults(void)
{
	bResetGameDefs = TRUE;
}

void ResetAllGameOptions(void)
{
	int i;

	for (i = 0; i < num_games; i++)
		ResetGameOptions(i);
}

int GetHasRoms(int driver_index)
{
	assert(0 <= driver_index && driver_index < num_games);

	return game_variables[driver_index].has_roms;
}

void SetHasRoms(int driver_index, int has_roms)
{
	assert(0 <= driver_index && driver_index < num_games);

	game_variables[driver_index].has_roms = has_roms;
}

int  GetHasSamples(int driver_index)
{
	assert(0 <= driver_index && driver_index < num_games);

	return game_variables[driver_index].has_samples;
}

void SetHasSamples(int driver_index, int has_samples)
{
	assert(0 <= driver_index && driver_index < num_games);

	game_variables[driver_index].has_samples = has_samples;
}

void IncrementPlayCount(int driver_index)
{
	assert(0 <= driver_index && driver_index < num_games);

	game_variables[driver_index].play_count++;

	SavePlayCount(driver_index);
}

void SetFolderFlags(const char *folderName, DWORD dwFlags)
{
	SaveFolderFlags(folderName, dwFlags);
}

int GetPlayCount(int driver_index)
{
	assert(0 <= driver_index && driver_index < num_games);

	return game_variables[driver_index].play_count;
}

/***************************************************************************
    Internal functions
 ***************************************************************************/

static void ColumnEncodeStringWithCount(void* data, char *str, int count)
{
	int* value = (int*)data;
	int  i;
	char tmpStr[100];

	sprintf(tmpStr, "%d", value[0]);
	
	strcpy(str, tmpStr);

    for (i = 1; i < count; i++)
	{
		sprintf(tmpStr, ",%d", value[i]);
		strcat(str, tmpStr);
	}
}

static void ColumnDecodeStringWithCount(const char* str, void* data, int count)
{
	int* value = (int*)data;
	int  i;
	char *s, *p;
	char tmpStr[100];

	if (str == NULL)
		return;

	strcpy(tmpStr, str);
	p = tmpStr;
	
    for (i = 0; p && i < count; i++)
	{
		s = p;
		
		if ((p = strchr(s,',')) != NULL && *p == ',')
		{
			*p = '\0';
			p++;
		}
		value[i] = atoi(s);
    }
	}

static void ColumnEncodeString(void* data, char *str)
{
	ColumnEncodeStringWithCount(data, str, COLUMN_MAX);
}

static void ColumnDecodeString(const char* str, void* data)
{
	ColumnDecodeStringWithCount(str, data, COLUMN_MAX);
}

static void ColumnDecodeWidths(const char* str, void* data)
{
	if (settings.view == VIEW_REPORT || settings.view == VIEW_GROUPED)
		ColumnDecodeString(str, data);
}

static void SplitterEncodeString(void* data, char* str)
{
	int* value = (int*)data;
	int  i;
	char tmpStr[100];

	sprintf(tmpStr, "%d", value[0]);
	
	strcpy(str, tmpStr);

	for (i = 1; i < GetSplitterCount(); i++)
	{
		sprintf(tmpStr, ",%d", value[i]);
		strcat(str, tmpStr);
	}
}

static void SplitterDecodeString(const char* str, void* data)
{
	int* value = (int*)data;
	int  i;
	char *s, *p;
	char tmpStr[100];

	if (str == NULL)
		return;

	strcpy(tmpStr, str);
	p = tmpStr;
	
	for (i = 0; p && i < GetSplitterCount(); i++)
	{
		s = p;
		
		if ((p = strchr(s,',')) != NULL && *p == ',')
		{
			*p = '\0';
			p++;
		}
		value[i] = atoi(s);
	}
}

static void ListDecodeString(const char* str, void* data)
{
	int* value = (int*)data;
	int i;

	*value = VIEW_GROUPED;

	for (i = VIEW_LARGE_ICONS; i < VIEW_MAX; i++)
	{
		if (strcmp(str, view_modes[i]) == 0)
		{
			*value = i;
			return;
		}
	}
}

static void ListEncodeString(void* data, char *str)
{
	int* value = (int*)data;

	strcpy(str, view_modes[*value]);
}

/* Parse the given comma-delimited string into a LOGFONT structure */
static void FontDecodeString(const char* str, void* data)
{
	LOGFONT* f = (LOGFONT*)data;
	char*	 ptr;
	
	sscanf(str, "%li,%li,%li,%li,%li,%i,%i,%i,%i,%i,%i,%i,%i",
		   &f->lfHeight,
		   &f->lfWidth,
		   &f->lfEscapement,
		   &f->lfOrientation,
		   &f->lfWeight,
		   (int*)&f->lfItalic,
		   (int*)&f->lfUnderline,
		   (int*)&f->lfStrikeOut,
		   (int*)&f->lfCharSet,
		   (int*)&f->lfOutPrecision,
		   (int*)&f->lfClipPrecision,
		   (int*)&f->lfQuality,
		   (int*)&f->lfPitchAndFamily);
	ptr = strrchr(str, ',');
	if (ptr != NULL)
		strcpy(f->lfFaceName, ptr + 1);
}

/* Encode the given LOGFONT structure into a comma-delimited string */
static void FontEncodeString(void* data, char *str)
{
	LOGFONT* f = (LOGFONT*)data;

	sprintf(str, "%li,%li,%li,%li,%li,%i,%i,%i,%i,%i,%i,%i,%i,%s",
			f->lfHeight,
			f->lfWidth,
			f->lfEscapement,
			f->lfOrientation,
			f->lfWeight,
			f->lfItalic,
			f->lfUnderline,
			f->lfStrikeOut,
			f->lfCharSet,
			f->lfOutPrecision,
			f->lfClipPrecision,
			f->lfQuality,
			f->lfPitchAndFamily,
			f->lfFaceName);
}

/* Register access functions below */
static void LoadOptions(void)
{
	HKEY    hKey;
	DWORD   value;
	LONG    result;
	int     i;
	char*   ptr;
	BOOL    bResetDefs = FALSE;
	BOOL    bVersionCheck = TRUE;
    char    keyString[80];

	/* Get to HKEY_CURRENT_USER\Software\Freeware\Mame32 */

	result = RegOpenKeyEx( HKEY_CURRENT_USER,
                           KEY_BASE,
                           0, 
                           KEY_QUERY_VALUE, 
                           &hKey );
	if (result == ERROR_SUCCESS)
	{
		BOOL bReset = FALSE;

        /* force reset of configuration? */
		GetRegBoolOption(hKey, "ResetGUI",			&bReset);

        /* reset all games to default? */
		GetRegBoolOption(hKey, "ResetGameDefaults", &bResetDefs);

        /* perform version check? */
		GetRegBoolOption(hKey, "VersionCheck",		&bVersionCheck);

		if (!bReset && bVersionCheck)
		{
			ptr = GetRegStringOption(hKey, "SaveVersion");
			if ( ptr 
                && strcmp(ptr, GetVersionString()) != 0 )
			{
                /* print warning that old MAME configuration detected */

				char msg[400];
				sprintf( msg, oldInfoMsg, ptr, GetVersionString() );
				if ( MessageBox( 0, 
                                 msg, 
                                 "Version Mismatch", 
                                 MB_YESNO | MB_ICONQUESTION) == IDYES )
				{
					bReset = TRUE;
					bResetDefs = TRUE;
				}
			}
		}

		if (bReset)
		{
            /* Reset configuration (read from .Backup if available) */

			RegCloseKey(hKey);

			/* Get to HKEY_CURRENT_USER\Software\Freeware\Mame32\.Backup */

			if ( RegOpenKeyEx( HKEY_CURRENT_USER, 
                               KEY_BASE_DOTBACKUP,
							   0, 
                               KEY_QUERY_VALUE, 
                               &hKey ) != ERROR_SUCCESS )
            {
				return;
            }
		}

		RegDeleteValue(hKey,"ListDetails");
		
        /* read FontColor if it exists */

		if ((value = GetRegOption(hKey, "FontColor")) != -1)
		{
			if (value == GetSysColor(COLOR_WINDOWTEXT))
				settings.list_font_color = (COLORREF)-1;
			else
				settings.list_font_color = value;
		}

		if ((value = GetRegOption(hKey, "CloneColor")) != -1)
		{
			if (value == GetSysColor(COLOR_WINDOWTEXT))
				settings.list_clone_color = (COLORREF)-1;
			else
				settings.list_clone_color = value;
		}

        /* read settings */

		for (i = 0; i < NUM_SETTINGS; i++)
        {
			GetRegObj(hKey, &regSettings[i]);
        }

		RegCloseKey(hKey);
	}

    /* Get to HKEY_CURRENT_USER\Software\Freeware\Mame32\.Defaults */

	result = RegOpenKeyEx( HKEY_CURRENT_USER, 
        bResetDefs ? KEY_BACKUP_DOTDEFAULTS : KEY_BASE_DOTDEFAULTS, 
                           0, 
                           KEY_QUERY_VALUE, 
                           &hKey );
	if (result == ERROR_SUCCESS)
	{
        /* read game default options */

		LoadRegGameOptions(hKey, &global, -1);
		RegCloseKey(hKey);
	}

    strcpy( keyString, KEY_BASE_FMT );

	for (i = 0 ; i < num_games; i++)
	{
        strcpy( keyString + KEY_BASE_FMT_CCH, drivers[i]->name );

        result = RegOpenKeyEx(HKEY_CURRENT_USER, keyString, 0, KEY_QUERY_VALUE, &hKey);
		if (result == ERROR_SUCCESS)
		{
			LoadRegGameOptions(hKey, &game_options[i], i);
			RegCloseKey(hKey);
		}
	}
}

static DWORD GetRegOption(HKEY hKey, const char *name)
{
	DWORD dwType;
	DWORD dwSize;
	DWORD value = -1;

	if (RegQueryValueEx(hKey, name, 0, &dwType, NULL, &dwSize) == ERROR_SUCCESS)
	{
		if (dwType == REG_DWORD)
		{
			dwSize = 4;
			RegQueryValueEx(hKey, name, 0, &dwType, (LPBYTE)&value, &dwSize);
		}
	}
	return value;
}

static void GetRegBoolOption(HKEY hKey, const char *name, BOOL *value)
{
	char *ptr;

	if ((ptr = GetRegStringOption(hKey, name)) != NULL)
	{
		*value = (*ptr == '0') ? FALSE : TRUE;
	}
}

static char *GetRegStringOption(HKEY hKey, const char *name)
{
	DWORD dwType;
	DWORD dwSize;
	static char str[300];

	memset(str, '\0', 300);

	if (RegQueryValueEx(hKey, name, 0, &dwType, NULL, &dwSize) == ERROR_SUCCESS)
	{
		if (dwType == REG_SZ)
		{
			dwSize = 299;
			RegQueryValueEx(hKey, name, 0, &dwType, (unsigned char *)str, &dwSize);
		}
	}
	else
	{
		return NULL;
	}

	return str;
}

static void SavePlayCount(int game_index)
{
	HKEY  hKey, hSubkey;
	DWORD dwDisposition = 0;
	LONG  result;
	char  keyString[300];

	assert(0 <= game_index && game_index < num_games);

	/* Get to HKEY_CURRENT_USER\Software\Freeware\Mame32 */
	result = RegCreateKeyEx(HKEY_CURRENT_USER,KEY_BASE,
							0, (char *)"", REG_OPTION_NON_VOLATILE,
							KEY_ALL_ACCESS, NULL, &hKey, &dwDisposition);
	if (result == ERROR_SUCCESS)
	{
        strcpy( keyString, KEY_BASE_FMT );
        strcpy( keyString + KEY_BASE_FMT_CCH, drivers[game_index]->name );

        result = RegCreateKeyEx( HKEY_CURRENT_USER, 
                                 keyString,
								 0, 
                                 (char *)"", 
                                 REG_OPTION_NON_VOLATILE,
								 KEY_ALL_ACCESS, 
                                 NULL, 
                                 &hSubkey, 
                                 &dwDisposition );
		if (result == ERROR_SUCCESS)
		{
			PutRegOption(hSubkey, "PlayCount", game_variables[game_index].play_count);
			RegCloseKey(hSubkey);
		}
		RegCloseKey(hKey);
	}
}

DWORD GetFolderFlags(const char *folderName)
{
	HKEY  hKey;
	long  value = 0;

	/* Get to HKEY_CURRENT_USER\Software\Freeware\Mame32\.Folders */

    if ( RegOpenKeyEx( HKEY_CURRENT_USER, 
                       KEY_BASE_DOTFOLDERS, 
                       0, 
                       KEY_QUERY_VALUE, 
                       &hKey ) == ERROR_SUCCESS )
	{
		value = GetRegOption(hKey, folderName);
		RegCloseKey(hKey);
	}
	return (value < 0) ? 0 : (DWORD)value;
}

static void SaveFolderFlags(const char *folderName, DWORD dwFlags)
{
	HKEY  hKey;
	DWORD dwDisposition = 0;
	LONG  result;

	/* Get to HKEY_CURRENT_USER\Software\Freeware\Mame32\.Folders */

    result = RegCreateKeyEx( HKEY_CURRENT_USER, 
                             KEY_BASE_DOTFOLDERS,
							 0, 
                             (char *)"", 
                             REG_OPTION_NON_VOLATILE,
							 KEY_ALL_ACCESS, 
                             NULL, 
                             &hKey, 
                             &dwDisposition );
	if (result == ERROR_SUCCESS)
	{
		PutRegOption(hKey, folderName, dwFlags);

		if (dwFlags == 0) /* Delete this reg key */
			RegDeleteValue(hKey, folderName);

		RegCloseKey(hKey);
	}
}

void SaveOptions(void)
{
	HKEY  hKey, hSubkey;
	DWORD dwDisposition = 0;
	LONG  result;
	char  keyString[300];
	int   i;
	BOOL  saved;

	SaveGlobalOptions(FALSE);

	/* Get to HKEY_CURRENT_USER\Software\Freeware\Mame32 */
	result = RegCreateKeyEx(HKEY_CURRENT_USER,KEY_BASE,
							0, (char *)"", REG_OPTION_NON_VOLATILE,
							KEY_ALL_ACCESS, NULL, &hKey, &dwDisposition);

    strcpy( keyString, KEY_BASE_FMT );

	for (i = 0; i < num_games; i++)
	{
		strcpy( keyString + KEY_BASE_FMT_CCH, drivers[i]->name );

		result = RegCreateKeyEx(HKEY_CURRENT_USER, keyString,
								0, (char *)"", REG_OPTION_NON_VOLATILE,
								KEY_ALL_ACCESS, NULL, &hSubkey, &dwDisposition);

		if (result == ERROR_SUCCESS)
		{
			PutRegOption(hSubkey,"PlayCount",game_variables[i].play_count);
			PutRegOption(hSubkey,"ROMS",game_variables[i].has_roms);
			PutRegOption(hSubkey,"Samples",game_variables[i].has_samples);

			saved = SaveRegGameOptions(hSubkey, &game_options[i]);
			RegCloseKey(hSubkey);
			if (saved == FALSE && game_variables[i].has_roms == UNKNOWN)
				RegDeleteKey(hKey,drivers[i]->name);
		}
	}
	RegCloseKey(hKey);
}


void SaveGameOptions(int driver_index)
{
	HKEY  hKey, hSubkey;
	DWORD dwDisposition = 0;
	LONG  result;
	char  keyString[300];
	BOOL  saved;

	/* Get to HKEY_CURRENT_USER\Software\Freeware\Mame32 */
	result = RegCreateKeyEx(HKEY_CURRENT_USER,KEY_BASE,
							0, (char *)"", REG_OPTION_NON_VOLATILE,
							KEY_ALL_ACCESS, NULL, &hKey, &dwDisposition);

    strcpy( keyString, KEY_BASE_FMT );
    strcpy( keyString + KEY_BASE_FMT_CCH, drivers[driver_index]->name);

	result = RegCreateKeyEx(HKEY_CURRENT_USER, keyString,
							0, (char *)"", REG_OPTION_NON_VOLATILE,
							KEY_ALL_ACCESS, NULL, &hSubkey, &dwDisposition);

	if (result == ERROR_SUCCESS)
	{
		PutRegOption(hSubkey,"PlayCount",game_variables[driver_index].play_count);
		PutRegOption(hSubkey,"ROMS",game_variables[driver_index].has_roms);
		PutRegOption(hSubkey,"Samples",game_variables[driver_index].has_samples);

		saved = SaveRegGameOptions(hSubkey,&game_options[driver_index]);
		RegCloseKey(hSubkey);
		if (saved == FALSE && game_variables[driver_index].has_roms == UNKNOWN)
			RegDeleteKey(hKey,drivers[driver_index]->name);
	}
	RegCloseKey(hKey);
}

void SaveDefaultOptions(void)
{
	HKEY  hKey, hSubkey;
	DWORD dwDisposition = 0;
	LONG  result;

	/* Get to HKEY_CURRENT_USER\Software\Freeware\Mame32 */
	result = RegCreateKeyEx(HKEY_CURRENT_USER,KEY_BASE,
							0, (char *)"", REG_OPTION_NON_VOLATILE,
							KEY_ALL_ACCESS, NULL, &hKey, &dwDisposition);

	result = RegCreateKeyEx( HKEY_CURRENT_USER, 
                             KEY_BASE_DOTDEFAULTS,
							 0, 
                             (char *)"", 
                             REG_OPTION_NON_VOLATILE,
							 KEY_ALL_ACCESS, 
                             NULL, 
                             &hSubkey, 
                             &dwDisposition);
	if (result == ERROR_SUCCESS)
	{
		global.use_default = FALSE;
		SaveRegGameOptions(hSubkey, &global);
		RegCloseKey(hSubkey);
	}
	RegCloseKey(hKey);
}

static void PutRegOption(HKEY hKey, const char *name, DWORD value)
{
	RegSetValueEx(hKey, name, 0, REG_DWORD, (void *)&value, sizeof(DWORD));
}

static void PutRegBoolOption(HKEY hKey, const char *name, BOOL value)
{
	char str[2];

	str[0] = (value) ? '1' : '0';
	str[1] = '\0';

	RegSetValueEx(hKey, name, 0, REG_SZ, (void *)str, 2);
}

static void PutRegStringOption(HKEY hKey, const char *name, char *option)
{
	RegSetValueEx(hKey, name, 0, REG_SZ, (void *)option, strlen(option) + 1);
}

static void WriteStringOptionToFile(FILE *fptr,const char *key,const char *value)
{
	fprintf(fptr,"%s \"%s\"\n",key,value);
}

static void WriteIntOptionToFile(FILE *fptr,const char *key,int value)
{
	fprintf(fptr,"%s %i\n",key,value);
}

static void WriteBoolOptionToFile(FILE *fptr,const char *key,BOOL value)
{
	fprintf(fptr,"%s %i\n",key,value ? 1 : 0);
}

static void WriteOptionToFile(FILE *fptr,REG_OPTION *regOpt)
{
	BOOL*	pBool;
	int*	pInt;
	char*	pString;
	double* pDouble;
	char*	cName = regOpt->m_cName;
	char	cTemp[80];
	
	switch (regOpt->m_iType)
	{
	case RO_DOUBLE:
		pDouble = (double*)regOpt->m_vpData;
        _gcvt( *pDouble, 10, cTemp );
		WriteStringOptionToFile(fptr, cName, cTemp);
		break;

	case RO_STRING:
		pString = (char*)regOpt->m_vpData;
		if (pString)
			WriteStringOptionToFile(fptr, cName, pString);
		break;

	case RO_PSTRING:
		pString = *(char**)regOpt->m_vpData;
		if (pString)
		    WriteStringOptionToFile(fptr, cName, pString);
		break;

	case RO_BOOL:
		pBool = (BOOL*)regOpt->m_vpData;
		WriteBoolOptionToFile(fptr, cName, *pBool);
		break;

	case RO_INT:
		pInt = (int*)regOpt->m_vpData;
		WriteIntOptionToFile(fptr, cName, *pInt);
		break;

	case RO_ENCODE:
		regOpt->encode(regOpt->m_vpData, cTemp);
		WriteStringOptionToFile(fptr, cName, cTemp);
		break;

	default:
		break;
	}

}

static void SaveGlobalOptions(BOOL bBackup)
{
	HKEY  hKey, hSubkey;
	DWORD dwDisposition = 0;
	LONG  result;

	int i;

#ifdef MAME32UI_INI
	FILE *fptr;

	fptr = fopen("mame32ui.ini","wt");
	fprintf(fptr,"### mame32ui.ini ###\n\n");
	fprintf(fptr,"%s\n",GetVersionString());
	// do something for fontcolor, clonecolor
	fprintf(fptr,"\n");

	fprintf(fptr,"### interface ###\n\n");
	for (i=0;i<NUM_SETTINGS;i++)
	{
		WriteOptionToFile(fptr,&regSettings[i]);
	}

	fprintf(fptr,"### game variables ###\n\n");
	for (i=0;i<num_games;i++)
	{
		// need to improve this to not save too many
		if (game_variables[i].play_count != 0 ||
			game_variables[i].has_roms != UNKNOWN ||
			game_variables[i].has_samples != UNKNOWN)
		{
			fprintf(fptr,"%s_playcount %i\n",drivers[i]->name,game_variables[i].play_count);
			fprintf(fptr,"%s_have_roms %i\n",drivers[i]->name,game_variables[i].has_roms);
			fprintf(fptr,"%s_have_samples %i\n",drivers[i]->name,game_variables[i].has_samples);
		}
	}
	fclose(fptr);
#endif

    /* Get to HKEY_CURRENT_USER\Software\Freeware\Mame32\.Backup or */
	/* Get to HKEY_CURRENT_USER\Software\Freeware\Mame32 */

	result = RegCreateKeyEx( HKEY_CURRENT_USER, 
        bBackup ? KEY_BASE_DOTBACKUP : KEY_BASE,
							 0, 
                             (char *)"", 
                             REG_OPTION_NON_VOLATILE,
							 KEY_ALL_ACCESS, 
                             NULL, 
                             &hKey, 
                             &dwDisposition );
	if (result == ERROR_SUCCESS)
	{
		PutRegStringOption(hKey, "SaveVersion", GetVersionString());

		if (settings.list_font_color != (COLORREF)-1 )
			PutRegOption(hKey, "FontColor", settings.list_font_color);
		else
			RegDeleteValue(hKey, "FontColor");

		if (settings.list_clone_color != (COLORREF)-1 )
			PutRegOption(hKey, "CloneColor", settings.list_clone_color);
		else
			RegDeleteValue(hKey, "CloneColor");

		for (i = 0; i < NUM_SETTINGS; i++)
			PutRegObj(hKey, &regSettings[i]);
	}
	
	if (! bBackup)
	{
		/* Delete old reg key if it exists */
		RegDeleteKey(hKey,	 "Defaults");
		RegDeleteValue(hKey, "LargeScreenShot");
		RegDeleteValue(hKey, "LoadIcons");

		/* Save ResetGUI flag */
		PutRegBoolOption(hKey, "ResetGUI",			bResetGUI);
		PutRegBoolOption(hKey, "ResetGameDefaults", bResetGameDefs);
	}

	result = RegCreateKeyEx( HKEY_CURRENT_USER, 
        bBackup ? KEY_BACKUP_DOTDEFAULTS : KEY_BASE_DOTDEFAULTS,
							 0, 
                             (char *)"", 
                             REG_OPTION_NON_VOLATILE,
							 KEY_ALL_ACCESS, 
                             NULL, 
                             &hSubkey, 
                             &dwDisposition );
	if (result == ERROR_SUCCESS)
	{
		global.use_default = FALSE;
		SaveRegGameOptions(hSubkey, &global);
		RegCloseKey(hSubkey);
	}

	RegCloseKey(hKey);
}

// returns whether there's any saved data at all
static BOOL SaveRegGameOptions(HKEY hKey, options_type *o)
{
	int   i;

	if (o->use_default == TRUE)
	{
		for (i = 0; i < NUM_GAMEOPTS; i++)
		{
			RegDeleteValue(hKey, regGameOpts[i].m_cName);
		}

		return FALSE;
	}

	/* copy passed in options to our struct */
	gOpts = *o;

	for (i = 0; i < NUM_GAMEOPTS; i++)
    {
		PutRegObj(hKey, &regGameOpts[i]);
    }

	return TRUE;
}

static void LoadRegGameOptions(HKEY hKey, options_type *o, int driver_index)
{
	int 	i;
	DWORD	value;
	DWORD	size;

	if (driver_index >= 0)
	{
		value = GetRegOption(hKey, "PlayCount");
		if (value != -1)
			game_variables[driver_index].play_count = value;
		
		value = GetRegOption(hKey, "ROMS");
		if (value != -1)
			game_variables[driver_index].has_roms = value;
		
		value = GetRegOption(hKey, "Samples");
		if (value != -1)
			game_variables[driver_index].has_samples = value;
	}


	/* look for window.  If it's not there, then use default options for this game */
	if (RegQueryValueEx(hKey, "window", 0, &value, NULL, &size) != ERROR_SUCCESS)
	   return;

	o->use_default = FALSE;

	/* copy passed in options to our struct */
	gOpts = *o;
	
	for (i = 0; i < NUM_GAMEOPTS; i++)
		GetRegObj(hKey, &regGameOpts[i]);

	/* copy options back out */
	*o = gOpts;
}

static void PutRegObj(HKEY hKey, REG_OPTION * regOpt)
{
	BOOL*	pBool;
	int*	pInt;
	char*	pString;
	double* pDouble;
	char*	cName = regOpt->m_cName;
	char	cTemp[80];
	
	switch (regOpt->m_iType)
	{
	case RO_DOUBLE:
		pDouble = (double*)regOpt->m_vpData;
        _gcvt( *pDouble, 10, cTemp );
		PutRegStringOption(hKey, cName, cTemp);
		break;

	case RO_STRING:
		pString = (char*)regOpt->m_vpData;
		if (pString)
			PutRegStringOption(hKey, cName, pString);
		break;

	case RO_PSTRING:
		pString = *(char**)regOpt->m_vpData;
		if (pString)
			PutRegStringOption(hKey, cName, pString);
		break;

	case RO_BOOL:
		pBool = (BOOL*)regOpt->m_vpData;
		PutRegBoolOption(hKey, cName, *pBool);
		break;

	case RO_INT:
		pInt = (int*)regOpt->m_vpData;
		PutRegOption(hKey, cName, *pInt);
		break;

	case RO_ENCODE:
		regOpt->encode(regOpt->m_vpData, cTemp);
		PutRegStringOption(hKey, cName, cTemp);
		break;

	default:
		break;
	}
}

static void GetRegObj(HKEY hKey, REG_OPTION * regOpts)
{
	char*	cName = regOpts->m_cName;
	char*	pString;
	int*	pInt;
	double* pDouble;
	int 	value;
	
	switch (regOpts->m_iType)
	{
	case RO_DOUBLE:
		pDouble = (double*)regOpts->m_vpData;
		pString = GetRegStringOption(hKey, cName);
		if (pString != NULL)
        {
            *pDouble = atof( pString );
        }
		break;

	case RO_STRING:
		pString = GetRegStringOption(hKey, cName);
		if (pString != NULL)
			strcpy((char*)regOpts->m_vpData, pString);
		break;

	case RO_PSTRING:
		pString = GetRegStringOption(hKey, cName);
		if (pString != NULL)
		{
			if (*(char**)regOpts->m_vpData != NULL)
				free(*(char**)regOpts->m_vpData);
			*(char**)regOpts->m_vpData = strdup(pString);
		}
		break;

	case RO_BOOL:
		GetRegBoolOption(hKey, cName, (BOOL*)regOpts->m_vpData);
		break;

	case RO_INT:
		pInt = (BOOL*)regOpts->m_vpData;
		value = GetRegOption(hKey, cName);
		if (value != -1)
			*pInt = value;
		break;

	case RO_ENCODE:
		pString = GetRegStringOption(hKey, cName);
		if (pString != NULL)
			regOpts->decode(pString, regOpts->m_vpData);
		break;

	default:
		break;
	}
	
}

char* GetVersionString(void)
{
	return build_version;
}

/* End of options.c */
