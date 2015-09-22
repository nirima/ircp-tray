/* ircp_io.h
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

#ifndef IRCP_IO_H
#define IRCP_IO_H

typedef enum {
	CD_CREATE=1,
	CD_ALLOWABS=2
} cd_flags;

obex_object_t *build_object_from_file(obex_t *handle, const char *localname, const char *remotename);
int ircp_open_safe(const char *path, const char *name);
int ircp_checkdir(const char *path, const char *dir, cd_flags flags);

#endif /* IRCP_IO_H */
