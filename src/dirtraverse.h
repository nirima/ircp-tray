/* dirtraverse.c
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

#ifndef DIRTRAVERSE_H
#define DIRTRAVERSE_H

typedef int (*visit_cb)(int action, char *name, char *path, void *userdata);
#define VISIT_FILE 1
#define VISIT_GOING_DEEPER 2
#define VISIT_GOING_UP 3

int visit_all_files(char *name, visit_cb cb, void *userdata);

#endif
