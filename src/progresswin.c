/* progresswin.c
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

#include "debug.h"
#include <progresswin.h>
#include <glib/gi18n.h>


static gboolean progress_window_ondelete (GtkWidget *widget,
                                            GdkEvent *event,
                                            gpointer user_data)
{
	struct ProgressWindow* window = (struct ProgressWindow*) user_data;

	DEBUG(5,"Progress window delete...\n");
	if (window->callback) {
		window->callback(window->callback_context);
		return FALSE;
	} else
		return TRUE; //refuse to close
}

static void progress_window_oncancel(GtkWidget* button, void * user_data)
{
	struct ProgressWindow* window = (struct ProgressWindow*) user_data;

	DEBUG(5,"Progress window cancel...\n");
	if (window->callback)
		window->callback(window->callback_context);
}

struct ProgressWindow* progress_window_new(char* filename, void* abortfunc, void* userdata)
{
	struct ProgressWindow* window;
	GtkWidget* frame, * frame2;
	GtkWidget* widget, * prgsbar;

	window = (struct ProgressWindow*) g_malloc( sizeof( struct ProgressWindow ) );
	window->callback = abortfunc;
	window->callback_context = userdata;
	window->filename = g_strdup (filename);
	
	//window
	window->handle = gtk_window_new( GTK_WINDOW_TOPLEVEL );
	
	gtk_window_set_resizable( GTK_WINDOW (window->handle), FALSE );
	gtk_window_activate_focus( GTK_WINDOW (window->handle) );
	gtk_window_set_title( GTK_WINDOW (window->handle), _("Beaming Progress") );
	
	// RGBA support
	GdkScreen *screen = gtk_widget_get_screen(window->handle);
    	GdkColormap *colormap = gdk_screen_get_rgba_colormap (screen);

	if (colormap)
		gtk_widget_set_default_colormap(colormap);
	
	gtk_container_set_border_width (GTK_CONTAINER (window->handle), 10);
	g_signal_connect( G_OBJECT( window->handle ),
                   "delete-event",
                   G_CALLBACK( progress_window_ondelete ),
                   window );

	frame = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(window->handle), frame);	

	//label title
	window->label = gtk_label_new("");
	gtk_box_pack_start(GTK_BOX(frame), window->label, FALSE, FALSE, 10);
	
	//progress bar
	window->progressbar = gtk_progress_bar_new();
	gtk_progress_bar_set_text(GTK_PROGRESS_BAR (window->progressbar), filename);
	gtk_box_pack_start(GTK_BOX (frame), window->progressbar, TRUE, FALSE, 2);
	
	//label progress
	window->label2 = gtk_label_new("");
	gtk_misc_set_alignment(GTK_MISC(window->label2), 0, 0);
	gtk_box_pack_start(GTK_BOX(frame), window->label2, FALSE, FALSE, 0);
		
	//cancel button
	widget = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
	
	g_signal_connect(G_OBJECT(widget),
                   "clicked",
                   G_CALLBACK(progress_window_oncancel),
                   window);

	gtk_box_pack_start(GTK_BOX(frame), widget, TRUE, FALSE, 10);

	gtk_widget_show_all(window->handle);
	
	return window;
}

void progress_window_destroy(struct ProgressWindow* window)
{
	gtk_widget_destroy(window->handle);
	g_free(window);
}

void progress_window_update(struct ProgressWindow* window, int current, int total, double rate)
{
	char *labeltext;
	
	if(total>0)
		gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR (window->progressbar),
						 (double)current/total);
	else
		gtk_progress_bar_pulse(GTK_PROGRESS_BAR (window->progressbar));
	
	labeltext = g_strdup_printf ( _("<b>Transfering</b> %s"), window->filename);
	gtk_label_set_markup (GTK_LABEL (window->label), labeltext);
    g_free(labeltext);
    
	if(total>0)
	{
		labeltext = g_strdup_printf("%d%%", current*100/total);
		gtk_progress_bar_set_text(GTK_PROGRESS_BAR (window->progressbar), labeltext);
		g_free(labeltext);
		
		if(rate>0)
			labeltext = g_strdup_printf( _("<small>%.1f KiB of %.1f KiB (%.1fbps)</small>"), current/1024.0, total/1024.0, rate);
		else
			labeltext = g_strdup_printf( _("<small>%.1f KiB of %.1f KiB</small>"), current/1024.0, total/1024.0);
		
		gtk_label_set_markup (GTK_LABEL (window->label2), labeltext);
		g_free(labeltext);
	}
	else
	{
		labeltext = g_strdup_printf( _("%d bytes"), current);
		gtk_label_set_markup (GTK_LABEL (window->label2), labeltext);
		g_free(labeltext);
		gtk_progress_bar_set_text(GTK_PROGRESS_BAR (window->progressbar), window->filename);
	}
	
}
