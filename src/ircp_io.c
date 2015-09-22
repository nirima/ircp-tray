/* ircp_io.c
 *
 * Copyright (C) 2004-2007 Xin Zhen
 * Copyright (C) 2008 Daniele Napolitano
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
 
#include <stdio.h>
#include <malloc.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/param.h>

#include <fcntl.h>
#include <string.h>
#include <time.h>

#include <openobex/obex.h>
#include <glib-2.0/glib.h>

#include "debug.h"
#include "ircp_io.h"

/*#define TRUE  1
#define FALSE 0*/

//
// Get some file-info. (size and lastmod)
//
static int get_fileinfo(const char *name, char *lastmod)
{
	struct stat stats;
	struct tm *tm;
	
	stat(name, &stats);
	tm = gmtime(&stats.st_mtime);
	snprintf(lastmod, 21, "%04d-%02d-%02d %02d:%02d:%02dZ",
			tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday,
			tm->tm_hour, tm->tm_min, tm->tm_sec);
	return (int) stats.st_size;
}

int strcpy_utf8_to_unicode(char* utf16, const char* utf8, int utf16_len_inbyte);
//
// Create an object from a file. Attach some info-headers to it
//
obex_object_t *build_object_from_file(obex_t *handle, const char *localname, const char *remotename)
{
	obex_object_t *object = NULL;
	obex_headerdata_t hdd;
	uint8_t *ucname;
	int ucname_len, size;
	int i;
	char lastmod[21*2] = {"1970-01-01T00:00:00Z"};
		
	/* Get filesize and modification-time */
	size = get_fileinfo(localname, lastmod);

	object = OBEX_ObjectNew(handle, OBEX_CMD_PUT);
	if(object == NULL)
		return NULL;

	ucname_len = strlen(remotename)*2 + 2;
	ucname = malloc(ucname_len);
	if(ucname == NULL)
		goto err;

	ucname_len = strcpy_utf8_to_unicode(ucname, remotename, ucname_len);
	
	//ucname_len = OBEX_CharToUnicode(ucname, remotename, ucname_len);
	//printf("ucname len %d\n", ucname_len);
	//for(i=0; i< ucname_len; i++)
	//	printf("%d ", ucname[i]);
	
	//printf("\n");

	hdd.bs = ucname;
	OBEX_ObjectAddHeader(handle, object, OBEX_HDR_NAME, hdd, ucname_len, 0);
	free(ucname);

	hdd.bq4 = size;
	OBEX_ObjectAddHeader(handle, object, OBEX_HDR_LENGTH, hdd, sizeof(uint32_t), 0);

	hdd.bs = NULL;
	OBEX_ObjectAddHeader(handle, object, OBEX_HDR_BODY,
				hdd, 0, OBEX_FL_STREAM_START);

	DEBUG(4, "Lastmod = %s\n", lastmod);
	return object;

err:
	if(object != NULL)
		OBEX_ObjectDelete(handle, object);
	return NULL;
}

//
// Check for dangerous filenames.
//
static int ircp_nameok(const char *name)
{
	DEBUG(4, "\n");
	
	/* No abs paths */
	if(name[0] == '/')
		return FALSE;

	if(strlen(name) >= 3) {
		/* "../../vmlinuz" */
		if(name[0] == '.' && name[1] == '.' && name[2] == '/')
			return FALSE;
		/* "dir/../../../vmlinuz" */
		if(strstr(name, "/../") != NULL)
			return FALSE;
	}
	return TRUE;
}
	
//
// Open a file, but do some sanity-checking first.
//
int ircp_open_safe(const char *path, const char *name)
{
	char diskname[MAXPATHLEN];
	int fd;

	DEBUG(4, "\n");
	
	/* Check for dangerous filenames */
	if(ircp_nameok(name) == FALSE)
		return -1;

	snprintf(diskname, MAXPATHLEN, "%s/%s", path, name);

	DEBUG(4, "Creating file %s\n", diskname);

	fd = open(diskname, O_RDWR | O_CREAT | O_TRUNC, DEFFILEMODE);
	return fd;
}

//
// Go to a directory. Create if not exists and create is true.
//
int ircp_checkdir(const char *path, const char *dir, cd_flags flags)
{
	char newpath[MAXPATHLEN];
	struct stat statbuf;
	int ret = -1;

	if(!(flags & CD_ALLOWABS))	{
		if(ircp_nameok(dir) == FALSE)
			return -1;
	}

	snprintf(newpath, MAXPATHLEN, "%s/%s", path, dir);

	DEBUG(4, "path = %s dir = %s, flags = %d\n", path, dir, flags);
	if(stat(newpath, &statbuf) == 0) {
		// If this directory aleady exist we are done
		if(S_ISDIR(statbuf.st_mode)) {
			DEBUG(4, "Using existing dir\n");
			ret = 1;
			goto out;
		}
		else  {
			// A non-directory with this name already exist.
			DEBUG(4, "A non-dir called %s already exist\n", newpath);
			ret = -1;
			goto out;
		}
	}
	if(flags & CD_CREATE) {
		DEBUG(4, "Will try to create %s\n", newpath);
		ret = mkdir(newpath, DEFFILEMODE | S_IXGRP | S_IXUSR | S_IXOTH);
	}
	else {
		ret = -1;
	}

out:
	return ret;
}

// return: length written in byte
int strcpy_utf8_to_unicode(char* utf16, const char* utf8, int utf16_len_inbyte)
{
	gunichar2* utf16str;
	int utf16_len;
	int i;
	utf16str = g_utf8_to_utf16(utf8, -1, NULL, (glong*) &utf16_len, NULL);

	if(utf16_len*sizeof(gunichar2)>utf16_len_inbyte)
		utf16_len = utf16_len_inbyte / sizeof(gunichar2);

	//memcpy(utf16, utf16str, utf16_len*sizeof(gunichar2));
	for(i=0; i<utf16_len; i++)
		((short*)utf16)[i] = htons(utf16str[i]);

	((short*)utf16)[i] = 0; //NULL-terminated

	//for(i=0; i< utf16_len*sizeof(gunichar2); i++)
	//	printf("%d ", utf16[i]);
	
	//printf("\n");

	g_free(utf16str);
	return (utf16_len+1)*sizeof(gunichar2);
}

char* unicode_to_utf8(void* unicode, int len_in_byte)
{
	int i;
	char* utf8;
	short* buffer = malloc(len_in_byte+2);
	
	if(!buffer)
		return strdup("untitled");
	
	//buffer[(len_in_byte+2)/2] = 0;
	memcpy((void*)buffer, unicode, len_in_byte);
	
	for(i=0;i<len_in_byte/2; i++)
		buffer[i] = ntohs(buffer[i]);
	
	buffer[i] = 0;
	
	utf8 = g_utf16_to_utf8(buffer, -1, NULL, NULL, NULL);
	free(buffer);
	
	return utf8;
}

char* filename_to_utf8(const char* filename)
{
	char* filename_u;

	//first, lookup G_FILENAME_ENCODING
	if (filename_u = g_filename_to_utf8(filename, -1, NULL, NULL, NULL))
		return filename_u;

	//then treat as locale encoding
	if(filename_u = g_locale_to_utf8(filename, -1, NULL, NULL, NULL))
		return filename_u;

	//whether it's already utf8
	if(g_utf8_validate(filename, -1, NULL))
		return g_strdup(filename);

	//last, treat as ISO-8859-1
	filename_u = g_convert (filename, -1, "UTF-8", "ISO-8859-1", NULL, NULL, NULL);
	return filename_u;
}

char* filename_from_utf8(const char* filename_u)
{
	char* filename;

	//whether it's utf8
	if(g_utf8_validate(filename_u, -1, NULL) == FALSE)
		return NULL;

	//first, lookup G_FILENAME_ENCODING
	if (filename = g_filename_from_utf8(filename_u, -1, NULL, NULL, NULL))
		return filename;

	//then, convert to locale encoding
	if(filename = g_locale_from_utf8(filename_u, -1, NULL, NULL, NULL))
		return filename;

	//last, convert to ISO-8859-1
	if (filename = g_convert (filename_u, -1, 
			"ISO-8859-1", "UTF-8", NULL, NULL, NULL))
		return filename;
	
	return NULL;
}
