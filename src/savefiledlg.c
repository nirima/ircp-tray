/* savefiledlg.c
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

#include "savefiledlg.h"
#include <glib/gi18n.h>
#include <string.h>

struct SaveFileDialog* savefile_dialog_new(char* titlebar, char* devicename)
{
	struct SaveFileDialog* dialog = g_malloc(sizeof(struct SaveFileDialog));
	GtkWidget *widget, *box;
	char* path;

	dialog->dialogbox = gtk_message_dialog_new(NULL,
					GTK_DIALOG_MODAL,
					GTK_MESSAGE_QUESTION,
					GTK_BUTTONS_YES_NO,
					_("Incoming File Transfer Request"));
					
	gtk_window_set_resizable(GTK_WINDOW (dialog->dialogbox), FALSE );
	gtk_window_activate_focus(GTK_WINDOW (dialog->dialogbox) );
	
	gtk_window_set_title(GTK_WINDOW (dialog->dialogbox), titlebar);

	gtk_dialog_set_default_response(GTK_DIALOG (dialog->dialogbox),
						GTK_RESPONSE_YES);
	gtk_message_dialog_format_secondary_text(
			GTK_MESSAGE_DIALOG (dialog->dialogbox),
			_("A remote IrDA device (%s) is attempting to send you a file. Do you want to accept it?"), devicename);

	//box 1
	box = gtk_hbox_new(FALSE, 12);

	//label
	widget = gtk_label_new(_("File name:"));
	gtk_box_pack_start(GTK_BOX (box), widget, FALSE, FALSE, 10);

	//entry
	dialog->currentname = widget = gtk_entry_new();
	gtk_entry_set_activates_default(GTK_ENTRY (widget), TRUE);
	gtk_box_pack_start(GTK_BOX (box), widget, FALSE, FALSE, 0);

	//save to button
	dialog->filepath = widget = gtk_file_chooser_button_new(_("Save to"),
				GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
	gtk_box_pack_start(GTK_BOX (box), widget, TRUE, FALSE, 0);

	gtk_box_pack_start(GTK_BOX ( gtk_dialog_get_content_area (GTK_DIALOG (dialog->dialogbox))),
			box, TRUE, FALSE, 10);

	path = (char*)config_get_lastsavedir();
	if(path) {
		savefile_dialog_set_current_folder(dialog, path);
		g_free(path);
	}
	return dialog;
}

void savefile_dialog_destroy(struct SaveFileDialog* dialog)
{
	char* path;
	path = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(dialog->filepath));
	if (dialog->respcode == GTK_RESPONSE_OK)
		config_set_lastsavedir(path);
	g_free(path);
	gtk_widget_destroy(dialog->dialogbox);

	g_free(dialog);
}

int savefile_dialog_run(struct SaveFileDialog* dialog)
{
	gtk_widget_show_all (dialog->dialogbox);
	return dialog->respcode = gtk_dialog_run(GTK_DIALOG(dialog->dialogbox));
}

//filename in utf8
void savefile_dialog_set_current_name(struct SaveFileDialog* dialog,
					char* filename_u)
{
	gtk_entry_set_text(GTK_ENTRY(dialog->currentname), filename_u);
}

gboolean savefile_dialog_set_current_folder(struct SaveFileDialog* dialog,
					char* filepath)
{
	return gtk_file_chooser_set_current_folder (
		GTK_FILE_CHOOSER(dialog->filepath), filepath);
}

char* savefile_dialog_get_filename(struct SaveFileDialog* dialog)
{
	char* pathname;
	const char*filename_u;
	char* filename;
	pathname = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog->filepath));
	filename_u = gtk_entry_get_text(GTK_ENTRY(dialog->currentname));
	filename = g_filename_from_utf8(filename_u, -1, NULL, NULL, NULL);
	pathname = g_realloc(pathname, strlen(pathname)+strlen(filename)+2);
	strcat(pathname, "/");
	strcat(pathname, filename);
	g_free(filename);
	return pathname;
}
