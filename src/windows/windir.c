/***************************************************************************

    windir.c - Win32 OSD core directory access functions

    Copyright (c) 1996-2007, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

***************************************************************************/

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <ctype.h>

#include "osdcore.h"
#include "strconv.h"


//============================================================
//  FUNCTION PROTOTYPES
//============================================================

extern mame_file_error win_error_to_mame_file_error(DWORD error);


//============================================================
//  TYPE DEFINITIONS
//============================================================

struct _osd_directory
{
	HANDLE				find;					// handle to the finder
	int					is_first;				// TRUE if this is the first entry
	osd_directory_entry	entry;					// current entry's data
	WIN32_FIND_DATA		data;					// current raw data
};



//============================================================
//  attributes_to_entry_type
//============================================================

INLINE osd_dir_entry_type attributes_to_entry_type(DWORD attributes)
{
	if (attributes == 0xFFFFFFFF)
		return ENTTYPE_NONE;
	else if (attributes & FILE_ATTRIBUTE_DIRECTORY)
		return ENTTYPE_DIR;
	else
		return ENTTYPE_FILE;
}



//============================================================
//  osd_opendir
//============================================================

osd_directory *osd_opendir(const char *dirname)
{
	osd_directory *dir = NULL;
	TCHAR *t_dirname = NULL;
	TCHAR *dirfilter = NULL;
	size_t dirfilter_size;

	// allocate memory to hold the osd_tool_directory structure
	dir = malloc(sizeof(*dir));
	if (dir == NULL)
		goto error;
	memset(dir, 0, sizeof(*dir));

	// initialize the structure
	dir->find = INVALID_HANDLE_VALUE;
	dir->is_first = TRUE;

	// convert the path to TCHARs
	t_dirname = tstring_from_utf8(dirname);
	if (t_dirname == NULL)
		goto error;

	// append \*.* to the directory name
	dirfilter_size = _tcslen(t_dirname) + 5;
	dirfilter = (TCHAR *)malloc(dirfilter_size * sizeof(*dirfilter));
	if (dirfilter == NULL)
		goto error;
	_sntprintf(dirfilter, dirfilter_size, TEXT("%s\\*.*"), t_dirname);

	// attempt to find the first file
	dir->find = FindFirstFile(dirfilter, &dir->data);

error:
	// cleanup
	if (t_dirname != NULL)
		free(t_dirname);
	if (dirfilter != NULL)
		free(dirfilter);
	if (dir != NULL && dir->find == INVALID_HANDLE_VALUE)
	{
		free(dir);
		dir = NULL;
	}
	return dir;
}


//============================================================
//  osd_readdir
//============================================================

const osd_directory_entry *osd_readdir(osd_directory *dir)
{
	// if we've previously allocated a name, free it now
	if (dir->entry.name != NULL)
	{
		free((void *)dir->entry.name);
		dir->entry.name = NULL;
	}

	// if this isn't the first file, do a find next
	if (!dir->is_first)
	{
		if (!FindNextFile(dir->find, &dir->data))
			return NULL;
	}

	// otherwise, just use the data we already had
	else
		dir->is_first = FALSE;

	// extract the data
	dir->entry.name = utf8_from_tstring(dir->data.cFileName);
	dir->entry.type = attributes_to_entry_type(dir->data.dwFileAttributes);
	dir->entry.size = dir->data.nFileSizeLow | ((UINT64) dir->data.nFileSizeHigh << 32);
	return (dir->entry.name != NULL) ? &dir->entry : NULL;
}


//============================================================
//  osd_closedir
//============================================================

void osd_closedir(osd_directory *dir)
{
	// free any data associated
	if (dir->entry.name != NULL)
		free((void *)dir->entry.name);
	if (dir->find != INVALID_HANDLE_VALUE)
		CloseHandle(dir->find);
	free(dir);
}



//============================================================
//  osd_stat
//============================================================

osd_directory_entry *osd_stat(const char *path)
{
	osd_directory_entry *result = NULL;
	TCHAR *t_path;
	HANDLE find = INVALID_HANDLE_VALUE;
	WIN32_FIND_DATA find_data;

	// convert the path to TCHARs
	t_path = tstring_from_utf8(path);
	if (t_path == NULL)
		goto done;

	// attempt to find the first file
	find = FindFirstFile(t_path, &find_data);
	if (find == INVALID_HANDLE_VALUE)
		goto done;

	// create an osd_directory_entry; be sure to make sure that the caller can
	// free all resources by just freeing the resulting osd_directory_entry
	result = (osd_directory_entry *) malloc(sizeof(*result) + strlen(path) + 1);
	if (!result)
		goto done;
	strcpy(((char *) result) + sizeof(*result), path);
	result->name = ((char *) result) + sizeof(*result);
	result->type = attributes_to_entry_type(find_data.dwFileAttributes);
	result->size = find_data.nFileSizeLow | ((UINT64) find_data.nFileSizeHigh << 32);

done:
	if (t_path)
		free(t_path);
	return result;
}



//============================================================
//  osd_is_path_separator
//============================================================

int osd_is_path_separator(char c)
{
	return (c == '/') || (c == '\\');
}



//============================================================
//	osd_is_absolute_path
//============================================================

int osd_is_absolute_path(const char *path)
{
	int result;

	if (osd_is_path_separator(path[0]))
		result = TRUE;
#ifndef UNDER_CE
	else if (isalpha(path[0]))
		result = (path[1] == ':');
#endif
	else
		result = FALSE;
	return result;
}


//============================================================
//	osd_dirname
//============================================================

char *osd_dirname(const char *filename)
{
	char *dirname;
	char *c;

	// NULL begets NULL
	if (!filename)
		return NULL;

	// allocate space for it
	dirname = malloc(strlen(filename) + 1);
	if (!dirname)
		return NULL;

	// copy in the name
	strcpy(dirname, filename);

	// search backward for a slash or a colon
	for (c = dirname + strlen(dirname) - 1; c >= dirname; c--)
		if (*c == '\\' || *c == '/' || *c == ':')
		{
			// found it: NULL terminate and return
			*(c + 1) = 0;
			return dirname;
		}

	// otherwise, return an empty string
	dirname[0] = 0;
	return dirname;
}


//============================================================
//	osd_basename
//============================================================

char *osd_basename(char *filename)
{
	char *c;

	// NULL begets NULL
	if (!filename)
		return NULL;

	// start at the end and return when we hit a slash or colon
	for (c = filename + strlen(filename) - 1; c >= filename; c--)
		if (*c == '\\' || *c == '/' || *c == ':')
			return c + 1;

	// otherwise, return the whole thing
	return filename;
}


//============================================================
//	osd_mkdir
//============================================================

mame_file_error osd_mkdir(const char *dir)
{
	mame_file_error filerr = FILERR_NONE;

	TCHAR *tempstr = tstring_from_utf8(dir);
	if (!tempstr)
	{
		filerr = FILERR_OUT_OF_MEMORY;
		goto done;
	}

	if (!CreateDirectory(tempstr, NULL))
	{
		filerr = win_error_to_mame_file_error(GetLastError());
		goto done;
	}

done:
	if (tempstr)
		free(tempstr);
	return filerr;
}


//============================================================
//	osd_rmdir
//============================================================

mame_file_error osd_rmdir(const char *dir)
{
	mame_file_error filerr = FILERR_NONE;

	TCHAR *tempstr = tstring_from_utf8(dir);
	if (!tempstr)
	{
		filerr = FILERR_OUT_OF_MEMORY;
		goto done;
	}

	if (!RemoveDirectory(tempstr))
	{
		filerr = win_error_to_mame_file_error(GetLastError());
		goto done;
	}

done:
	if (tempstr)
		free(tempstr);
	return filerr;
}


//============================================================
//	osd_getcurdir
//============================================================

mame_file_error osd_getcurdir(char *buffer, size_t buffer_len)
{
	mame_file_error filerr = FILERR_NONE;
	TCHAR *tempstr = NULL;
	TCHAR path[_MAX_PATH];

	if (!GetCurrentDirectory(ARRAY_LENGTH(path), path))
	{
		filerr = win_error_to_mame_file_error(GetLastError());
		goto done;
	}

	tempstr = utf8_from_tstring(path);
	if (!tempstr)
	{
		filerr = FILERR_OUT_OF_MEMORY;
		goto done;
	}
	snprintf(buffer, buffer_len, "%s", tempstr);

done:
	if (tempstr)
		free(tempstr);
	return filerr;
}


//============================================================
//	osd_setcurdir
//============================================================

mame_file_error osd_setcurdir(const char *dir)
{
	mame_file_error filerr = FILERR_NONE;

	TCHAR *tempstr = tstring_from_utf8(dir);
	if (!tempstr)
	{
		filerr = FILERR_OUT_OF_MEMORY;
		goto done;
	}

	if (!SetCurrentDirectory(tempstr))
	{
		filerr = win_error_to_mame_file_error(GetLastError());
		goto done;
	}

done:
	if (tempstr)
		free(tempstr);
	return filerr;
}
