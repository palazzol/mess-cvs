/***************************************************************************

  layoutms.c

  MESS specific TreeView definitions (and maybe more in the future)

***************************************************************************/

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <shellapi.h>
#include <commctrl.h>
#include <stdio.h>  /* for sprintf */
#include <stdlib.h> /* For malloc and free */
#include <string.h>

#include "ui/treeview.h"
#include "ui/m32util.h"
#include "ui/resource.h"
#include "ui/resourcems.h"
#include "ui/ms32util.h"
#include "ui/directories.h"
#include "ui/options.h"
#include "ui/splitters.h"
#include "ui/help.h"
#include "ui/properties.h"
#include "ui/audit32.h"
#include "ui/propertiesms.h"

static BOOL FilterAvailable(int driver_index);

FOLDERDATA g_folderData[] =
{
	{"All Systems",     FOLDER_ALLGAMES,     IDI_FOLDER,				0,             0,            NULL,                       NULL,              TRUE },
	{"Available",       FOLDER_AVAILABLE,    IDI_FOLDER_AVAILABLE,     F_AVAILABLE,   0,            NULL,                       FilterAvailable,              TRUE },
#ifdef SHOW_UNAVAILABLE_FOLDER
	{"Unavailable",     FOLDER_UNAVAILABLE,  IDI_FOLDER_UNAVAILABLE,	0,             F_AVAILABLE,  NULL,                       FilterAvailable,              FALSE },
#endif
	{"Console",         FOLDER_CONSOLE,      IDI_FOLDER,				F_CONSOLE,     F_COMPUTER,   NULL,                       DriverIsComputer,  FALSE },
	{"Computer",        FOLDER_COMPUTER,     IDI_FOLDER,				F_COMPUTER,    F_CONSOLE,    NULL,                       DriverIsComputer,  TRUE },
	{"Modified/Hacked", FOLDER_MODIFIED,     IDI_FOLDER,				0,             0,            NULL,                       DriverIsModified,  TRUE },
	{"Manufacturer",    FOLDER_MANUFACTURER, IDI_FOLDER_MANUFACTURER,  0,             0,            CreateManufacturerFolders },
	{"Year",            FOLDER_YEAR,         IDI_FOLDER_YEAR,          0,             0,            CreateYearFolders },
	{"Source",          FOLDER_SOURCE,       IDI_FOLDER_SOURCE,        0,             0,            CreateSourceFolders },
	{"CPU",             FOLDER_CPU,          IDI_FOLDER,               0,             0,            CreateCPUFolders },
	{"SND",             FOLDER_SND,          IDI_FOLDER,               0,             0,            CreateSoundFolders },
	{"Working",         FOLDER_WORKING,      IDI_WORKING,				F_WORKING,     F_NONWORKING, NULL,                       DriverIsBroken,    FALSE },
	{"Non-Working",     FOLDER_NONWORKING,   IDI_NONWORKING,			F_NONWORKING,  F_WORKING,    NULL,                       DriverIsBroken,    TRUE },
	{"Originals",       FOLDER_ORIGINAL,     IDI_FOLDER,				F_ORIGINALS,   F_CLONES,     NULL,                       DriverIsClone,     FALSE },
	{"Clones",          FOLDER_CLONES,       IDI_FOLDER,				F_CLONES,      F_ORIGINALS,  NULL,                       DriverIsClone,     TRUE },
	{"Raster",          FOLDER_RASTER,       IDI_FOLDER,				F_RASTER,      F_VECTOR,     NULL,                       DriverIsVector,    FALSE },
	{"Vector",          FOLDER_VECTOR,       IDI_FOLDER,				F_VECTOR,      F_RASTER,     NULL,                       DriverIsVector,    TRUE },
	{"Trackball",       FOLDER_TRACKBALL,    IDI_FOLDER,				0,             0,            NULL,                       DriverUsesTrackball,	TRUE },
	{"Stereo",          FOLDER_STEREO,       IDI_SOUND,	                0,             0,            NULL,                       DriverIsStereo,    TRUE },
	{ NULL }
};

/* list of filter/control Id pairs */
FILTER_ITEM g_filterList[] =
{
	{ F_COMPUTER,     IDC_FILTER_COMPUTER,    DriverIsComputer, TRUE },
	{ F_CONSOLE,      IDC_FILTER_CONSOLE,     DriverIsComputer, FALSE },
	{ F_MODIFIED,     IDC_FILTER_MODIFIED,    DriverIsModified, TRUE },
	{ F_CLONES,       IDC_FILTER_CLONES,      DriverIsClone, TRUE },
	{ F_NONWORKING,   IDC_FILTER_NONWORKING,  DriverIsBroken, TRUE },
	{ F_UNAVAILABLE,  IDC_FILTER_UNAVAILABLE, FilterAvailable, FALSE },
	{ F_RASTER,       IDC_FILTER_RASTER,      DriverIsVector, FALSE },
	{ F_VECTOR,       IDC_FILTER_VECTOR,      DriverIsVector, TRUE },
	{ F_ORIGINALS,    IDC_FILTER_ORIGINALS,   DriverIsClone, FALSE },
	{ F_WORKING,      IDC_FILTER_WORKING,     DriverIsBroken, FALSE },
	{ F_AVAILABLE,    IDC_FILTER_AVAILABLE,   FilterAvailable, TRUE },
	{ 0 }
};

DIRECTORYINFO g_directoryInfo[] =
{
	{ "BIOSes",                GetRomDirs,      SetRomDirs,      TRUE,  DIRDLG_ROMS },
	{ "Software",              GetSoftwareDirs, SetSoftwareDirs, TRUE,  DIRDLG_SOFTWARE },
	{ "Ini Files",             GetIniDir,       SetIniDir,       FALSE, DIRDLG_INI },
	{ "CRC",                   GetCrcDir,       SetCrcDir,       FALSE, 0 },
	{ "Config",                GetCfgDir,       SetCfgDir,       FALSE, DIRDLG_CFG },
	{ "High Scores",           GetHiDir,        SetHiDir,        FALSE, DIRDLG_HI },
	{ "Snapshots",             GetImgDir,       SetImgDir,       FALSE, DIRDLG_IMG },
	{ "Input files (*.inp)",   GetInpDir,       SetInpDir,       FALSE, DIRDLG_INP },
	{ "State",                 GetStateDir,     SetStateDir,     FALSE, 0 },
	{ "Artwork",               GetArtDir,       SetArtDir,       FALSE, 0 },
	{ "Memory Card",           GetMemcardDir,   SetMemcardDir,   FALSE, 0 },
	{ "Flyers",                GetFlyerDir,     SetFlyerDir,     FALSE, 0 },
	{ "Cabinets",              GetCabinetDir,   SetCabinetDir,   FALSE, 0 },
	{ "Marquees",              GetMarqueeDir,   SetMarqueeDir,   FALSE, 0 },
	{ "Titles",                GetTitlesDir,    SetTitlesDir,    FALSE, 0 },
	{ "NVRAM",                 GetNvramDir,     SetNvramDir,     FALSE, 0 },
	{ "Controller Files",      GetCtrlrDir,     SetCtrlrDir,     FALSE, DIRDLG_CTRLR },
	{ "Hard Drive Difference", GetDiffDir,      SetDiffDir,      FALSE, 0 },
	{ "Icons",                 GetIconsDir,     SetIconsDir,     FALSE, 0 },
	{ "Background Images",     GetBgDir,        SetBgDir,        FALSE, 0 },
	{ NULL }
};

const SPLITTERINFO g_splitterInfo[] =
{
	{ 0.2,	IDC_SPLITTER,	IDC_TREE,	IDC_LIST,		AdjustSplitter1Rect },
	{ 0.4,	IDC_SPLITTER2,	IDC_LIST,	IDC_LIST2,		AdjustSplitter1Rect },
	{ 0.6,	IDC_SPLITTER3,	IDC_LIST2,	IDC_SSFRAME,	AdjustSplitter2Rect },
	{ -1 }
};

const MAMEHELPINFO g_helpInfo[] =
{
	{ ID_HELP_CONTENTS,		TRUE,	MAME32HELP "::/html/mess_overview.htm" },
	{ ID_HELP_RELEASE,		FALSE,	"docs\\Mess.txt" },
	{ ID_HELP_WHATS_NEW,	TRUE,	MAME32HELP "::/messnew.txt" },
	{ -1 }
};

PROPERTYSHEETINFO g_propSheets[] =
{
	{ FALSE,	NULL,					IDD_PROP_GAME,			GamePropertiesDialogProc },
	{ FALSE,	NULL,					IDD_PROP_AUDIT,			GameAuditDialogProc },
	{ TRUE,		NULL,					IDD_PROP_DISPLAY,		GameOptionsProc },
	{ TRUE,		NULL,					IDD_PROP_ADVANCED,		GameOptionsProc },
	{ TRUE,		NULL,					IDD_PROP_SOUND,			GameOptionsProc },
	{ TRUE,		NULL,					IDD_PROP_INPUT,			GameOptionsProc },
	{ TRUE,		NULL,					IDD_PROP_MISC,			GameOptionsProc },
	{ TRUE,		NULL,					IDD_PROP_SOFTWARE,		GameMessOptionsProc },
	{ FALSE,	PropSheetFilter_Config,	IDD_PROP_CONFIGURATION,	GameMessOptionsProc },
	{ TRUE,		PropSheetFilter_Vector,	IDD_PROP_VECTOR,		GameOptionsProc },
	{ FALSE }
};

const char g_szDefaultGame[] = "nes";

static BOOL FilterAvailable(int driver_index)
{
	// GetHasRoms() returns FALSE, TRUE, UNKNOWN... sigh.
	return GetHasRoms(driver_index) == TRUE;
}
