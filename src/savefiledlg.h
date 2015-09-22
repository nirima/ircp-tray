/* savefiledlg.h
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

#include <gtk/gtk.h>

struct SaveFileDialog {
	GtkWidget* dialogbox;
	GtkWidget* currentname;
	GtkWidget* filepath;
	unsigned respcode;
};

struct SaveFileDialog* savefile_dialog_new(char* titlebar, char* devicename);

void savefile_dialog_destroy(struct SaveFileDialog*);

int savefile_dialog_run(struct SaveFileDialog*);

void savefile_dialog_set_current_name(struct SaveFileDialog*, char*utf8);
gboolean savefile_dialog_set_current_folder(struct SaveFileDialog*, char*filename);
char* savefile_dialog_get_filename(struct SaveFileDialog*);
