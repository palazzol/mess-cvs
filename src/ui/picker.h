/***************************************************************************

  M.A.M.E.32  -  Multiple Arcade Machine Emulator for Win32
  Win32 Portions Copyright (C) 1997-2003 Michael Soderstrom and Chris Kirmse

  This file is part of MAME32, and may only be used, modified and
  distributed under the terms of the MAME license, in "readme.txt".
  By continuing to use, modify or distribute this file you indicate
  that you have read the license and understand and accept it fully.

 ***************************************************************************/

#ifndef PICKER_H
#define PICKER_H

struct PickerCallbacks
{
	PFNLVCOMPARE pfnCompare;

	// Options retrieval
	void (*pfnSetSortColumn)(int column);
	int (*pfnGetSortColumn)(void);
	void (*pfnSetSortReverse)(BOOL reverse);
	BOOL (*pfnGetSortReverse)(void);
	void (*pfnSetViewMode)(int val);
	int (*pfnGetViewMode)(void);
	void (*pfnSetColumnWidths)(int widths[]);
	void (*pfnGetColumnWidths)(int widths[]);
	void (*pfnSetColumnOrder)(int order[]);
	void (*pfnGetColumnOrder)(int order[]);
	void (*pfnSetColumnShown)(int shown[]);
	void (*pfnGetColumnShown)(int shown[]);
	BOOL (*pfnGetOffsetChildren)(void);

	void (*pfnDoubleClick)(void);
	const TCHAR *(*pfnGetItemString)(int nItem, int nColumn,
		TCHAR *pszBuffer, UINT nBufferLength);
	int (*pfnGetItemImage)(int nItem);
	void (*pfnLeavingItem)(int nItem);
	void (*pfnEnteringItem)(int nItem);
	void (*pfnBeginListViewDrag)(NM_LISTVIEW *pnlv);
	int (*pfnFindItemParent)(int nItem);
	BOOL (*pfnOnIdle)(HWND hwndPicker);
};

struct PickerOptions
{
	const struct PickerCallbacks *pCallbacks;
	BOOL bOldControl;
	BOOL bXPControl;
	int nColumnCount;
	LPCTSTR *ppszColumnNames;
};

enum
{
	VIEW_LARGE_ICONS = 0,
	VIEW_SMALL_ICONS,
	VIEW_INLIST,
	VIEW_REPORT,
	VIEW_GROUPED,
	VIEW_MAX
};



BOOL SetupPicker(HWND hwndPicker, const struct PickerOptions *pOptions);

int Picker_GetViewID(HWND hwndPicker);
void Picker_SetViewID(HWND hwndPicker, int nViewID);
int Picker_GetRealColumnFromViewColumn(HWND hwndPicker, int nViewColumn);
int Picker_GetViewColumnFromRealColumn(HWND hwndPicker, int nRealColumn);
void Picker_Sort(HWND hwndPicker);
void Picker_ResetColumnDisplay(HWND hwndPicker);
int Picker_GetSelectedItem(HWND hwndPicker);
void Picker_SetSelectedItem(HWND hwndPicker, int nItem);
void Picker_SetSelectedPick(HWND hwndPicker, int nIndex);
int Picker_GetNumColumns(HWND hWnd);
void Picker_ResetIdle(HWND hwndPicker);
BOOL Picker_IsIdling(HWND hwndPicker);

// These are used to handle events received by the parent regarding
// picker controls
BOOL Picker_HandleNotify(LPNMHDR lpNmHdr);
void Picker_HandleDrawItem(HWND hwndPicker, LPDRAWITEMSTRUCT lpDrawItemStruct);

#endif // PICKER_H
