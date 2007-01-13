/*********************************************************************

	osd_mess.h

	OS dependent calls for MESS

*********************************************************************/

#ifndef OSD_MESS_H
#define OSD_MESS_H

struct _mame_file;
typedef struct _mess_image mess_image;

/* called by the filemanager code to allow the OS to override the file		*/
/* selecting with it's own code. Return 0 if the MESS core should handle	*/
/* file selection, -1 if the OS code does nothing or 1 if the OS code		*/
/* changed a file.															*/
int osd_select_file(mess_image *img, char *filename);

/* returns 1 if input of type IPT_KEYBOARD should be supressed */
int osd_keyboard_disabled(void);



/******************************************************************************

  Parallel processing (for SMP)

******************************************************************************/

/* 
  Called by core to distribute tasks across multiple processors.  When this is
  called, there will be 1 to max_tasks invocations of task(); where task_count
  specifies the number of calls, and task_num is a number from zero to
  task_count-1 to specify which call was made.  This can be used to subdivide
  tasks across mulitple processors.

  If max_tasks<1, then it should be treated as if it was 1

  A bogus implementation would look like this:

	void osd_parallelize(void (*task)(void *param, int task_num, int
		task_count), void *param, int max_tasks)
	{
		task(param, 0, 1);
	}
*/

void osd_parallelize(void (*task)(void *param, int task_num, int task_count), void *param, int max_tasks);

/******************************************************************************

  Device and file browsing

******************************************************************************/

int osd_num_devices(void);
const char *osd_get_device_name(int i);
void osd_change_device(const char *vol);
const char *osd_get_cwd(void);

void osd_begin_final_unloading(void);

/* used to notify osd code of the load status of an image */
void osd_image_load_status_changed(mess_image *img, int is_final_unload);

#endif /* OSD_MESS_H */

