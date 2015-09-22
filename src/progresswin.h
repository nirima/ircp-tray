/* progresswin.h
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

#ifndef _PROGRESS_WIN
#define _PROGRESS_WIN

#include <gtk/gtk.h>
#include <glib.h>

struct ProgressWindow {
	GtkWidget* handle;
	
	GtkWidget* label;
	GtkWidget* label2;
	GtkWidget* progressbar;
	
	char* filename;
	
	void(*callback)(void*);
	void* callback_context;
};

struct ProgressWindow* progress_window_new(char* filename, void* abortfunc, void* userdata);

void progress_window_destroy(struct ProgressWindow* window);
void progress_window_update(struct ProgressWindow* window, int current, int total, double rate);

#endif
