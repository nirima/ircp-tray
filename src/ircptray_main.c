/* ircptray_main.c
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

#include <config.h>
#include "ircptray_main.h"
#include "debug.h"

#include <libnotify/notification.h>
#include <libnotify/notify.h>

#include <gtk/gtk.h>
#include <glib/gi18n-lib.h>
#include "ircptray_trayicon.h"
#include "resources.h"
#include "sendfile.h"
#include "recvfile.h"
#include "irda_misc.h"

/* Discover interval in seconds */
#define DISCOVERER_POLL_INTERVAL 3
/* user config path */
#define CONFIG_FILENAME "/ircp-tray.conf"

GtkStatusIcon* AppTrayIcon;

static gboolean check_irda_config()
{

	if (g_file_test("/var/run/irattach.pid", G_FILE_TEST_EXISTS))
	    return TRUE;
	else {
	    g_printerr("WW: /var/run/irattach.pid not found, assumed irattach not running");
	    return FALSE;
	}

	/*
	char* data;
	FILE* fp;
	char** v1, ** v2;
	short int result;

	data = g_malloc(15);
	fp = fopen("/etc/default/irda-utils", "r");

	if(fp)
	{
		while(fgets(data, 15, fp))
			if(g_str_has_prefix (data, "ENABLE") == TRUE) break;

		data = g_strstrip(data);
		v1 = g_strsplit(data, "=", 2);
		v2 = g_strsplit(v1[1], "\"", 3);
		result = strncmp("false", v2[1], 5 );
		fclose(fp);
	}

	printf("IrDA enabled: %s\n", v2[1]);

	g_strfreev(v2);
	g_strfreev(v1);
	g_free(data);

	if(result == 0) return FALSE;
	else return TRUE;
	*/

}

void version_onclick(GtkMenuItem *menuitem, gpointer user_data)
{

	char* authors[] = {
		"Xin Zhen <xinzhen@pub.minidns.net>",
		"Daniele Napolitano <dnax88@gmail.com>",
		"",
		"Marcel Holtmann",
		"Pontus Fuchs",
		"Ragnar Henriksen",
		NULL
	};

	char* artists[] = {
		"Icon design: Jakub Steiner <jimmac@ximian.com>",
		"Animation: Dieter Vanderfaeillie <dieter.vanderfaeillie@gmail.com>",
		NULL
	};

	/* Feel free to put your names here translators :-) */
	char* translators = _("translator-credits");

	gtk_show_about_dialog(NULL,
                          "authors", authors,
			              "artists", artists,
			              "translator-credits", strcmp("translator-credits", translators) ? translators : NULL,
                          "comments", _("A utility for IrDA OBEX beaming and receiving"),
                          "copyright", "Copyright © 2004-2007 Xin Zhen \"LoneStar\"\nCopyright © 2008-2010 Daniele Napolitano \"DnaX\"",
                          "version", VERSION,
                          "website", "http://launchpad.net/ircp-tray",
			              "logo-icon-name", MAIN_ICON,
                           NULL);

}

/* Get last save directory from user config file */
char* config_get_lastsavedir()
{
	char* lastsavedir = NULL;
	char* configfilename;
	GKeyFile* config;

	configfilename = g_strconcat (g_get_user_config_dir(),
	                              CONFIG_FILENAME,
	                              NULL);

	config = g_key_file_new();
	g_key_file_load_from_file(config, configfilename,
		G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS, NULL);
	lastsavedir = g_key_file_get_string(config, "General", "LastSaveDir",
						NULL);
	g_key_file_free(config);
	g_free(configfilename);

	return lastsavedir;
}

/* Save last save directory from user config file */
void config_set_lastsavedir(char* dir)
{
	char* configfilename;
	GKeyFile* config;
	char* configstr;
	FILE *configfile;

	configfilename = g_strconcat (g_get_user_config_dir(),
	                              CONFIG_FILENAME,
	                              NULL);
    g_print("%s\n", configfilename);
	config = g_key_file_new();
	g_key_file_load_from_file(config, configfilename,
		G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS, NULL);
	g_key_file_set_string(config, "General", "LastSaveDir", dir);

	configstr = g_key_file_to_data(config, NULL, NULL);

	configfile = fopen(configfilename, "w");
	fprintf(configfile, configstr, NULL);
	fclose(configfile);

	g_free(configstr);
	g_free(configfilename);
	g_key_file_free(config);

}

char* get_log_path()
{
	char* logfile;
	char filename[] = "/.ircp-tray_log";

	logfile = g_malloc(strlen(g_get_home_dir())+strlen(filename)+1);
	g_sprintf(logfile, "%s%s", g_get_home_dir(), filename);

	return logfile;
}
void log_file(const char* message)
{
	char* logfile;
	FILE* fp;
        char tmstr[21];
        time_t tm = time(NULL);

	printf("%s\n", message);
        strftime(tmstr, 20, "%x %X", localtime(&tm));

	logfile = get_log_path();

	fp = fopen(logfile, "a");
	if(fp) {
		fprintf(fp, "[%s] %s\n", tmstr, message);
		fclose(fp);
	}
	g_free(logfile);
}

static char *discoverer_name;
static int discoverer_hints[2];

/* Do some things on device discovered */
static void
gui_update_on_discovery (gchar *device_name)
{
    /* Refresh tray icon */
	trayicon_set_icon(ICON_TRAY);

	/* Update icon tray tooltip */
	gchar *tooltip_msg;

	tooltip_msg = g_strdup_printf (_("Infrared File Transfer: %s"), device_name);
	trayicon_set_tooltip_text(tooltip_msg);
	g_free(tooltip_msg);

	/* Activate device info menù item */
	trayicon_set_info_menuitem_sensitive(TRUE);
}

/* Do some things on device disappeared */
static void
gui_update_on_disappearing (guint num_devices)
{
    /* Refresh tray icon */
	trayicon_set_icon(ICON_TRAY_INACTIVE);

	/* Update icon tray tooltip */
	trayicon_set_tooltip_text(_("Infrared File Transfer") );

	/* Disable device info menù item on zero devices */
	if ( num_devices <= 0 )
	    trayicon_set_info_menuitem_sensitive(FALSE);

}

/* Send notify on device discovered */
static void
notify_on_discovering (gchar *device_name, gchar *icon )
{
    NotifyNotification *notification = NULL;
	gchar *notify_msg;

	notify_msg = g_strdup_printf( _("Remote device discovered: %s"),
					 		      device_name);

	notification = notify_notification_new (g_get_application_name(),
                                            notify_msg,
                                            icon,
                                            NULL);
    if (notification == NULL) {
        g_warning("failed to setup notification");
        return;
    }

	notify_notification_attach_to_status_icon (notification, AppTrayIcon);

	if (!notify_notification_show (notification, NULL))
	    g_warning("failed to send notification");

	/* destroy intantly if there isn't actions */
	g_object_unref(G_OBJECT(notification));

}

/* Send notify on device disappeared */
static void
notify_on_disappearing (gchar *device_name, gchar *icon )
{
    NotifyNotification *notification = NULL;
    gchar *notify_msg;

    notify_msg = g_strdup_printf( _("Remote device %s disappeared"),
					 		      device_name);

	notification = notify_notification_new (g_get_application_name(),
                                       notify_msg,
                                       icon,
                                       NULL);

	if (notification != NULL) {
	    notify_notification_attach_to_status_icon (notification, AppTrayIcon);

		if (!notify_notification_show (notification, NULL))
			g_warning("failed to send notification\n");
		g_object_unref(G_OBJECT(notification));
	}
	else
	    g_warning("failed to setup notification");

}

GList *DevicesList;
guint NumDevices;

/* Function called each X seconds that detect device discovered
 * and disappeared. Then send notification and update GUI. */
static gboolean discover_do(gpointer data)
{
	GList *list;
    int i, c;
    dev_info *device, *device2;
    guint8 list_length;
    gboolean found;

	list = irda_discover_devices();
    list_length = g_list_length(list);

    /* Check for added devices */
    for(i = 0; i < list_length; i++)
    {
        device = g_list_nth_data(list, i);

        if(DevicesList == NULL)
        {
            /* Device discovered */
            g_print("* Device discovered: %s [0x%08x]\n",
                      device->name, device->daddr);

            notify_on_discovering (device->name, device->icon);
            gui_update_on_discovery (device->name);

            if(device->hints[1] & 0x20) {
                if(discoverer_name)
                    g_free(discoverer_name);
                discoverer_name = g_strdup(device->name);
                discoverer_hints[0] = device->hints[0];
                discoverer_hints[1] = device->hints[1];
            }

            continue;
        }
        for(c = 0; c < NumDevices; c++)
        {
            device2 = g_list_nth_data(DevicesList, c);

            if( device->daddr == device2->daddr ) {

                found = TRUE;
                break;
            }
            else
                found = FALSE;
        }
        /* Device discovered */
        if(found == FALSE)
        {
            g_print("* Device discovered: %s [0x%08x]\n",
                      device->name, device->daddr);

            notify_on_discovering (device->name, device->icon);
            gui_update_on_discovery (device->name);

            if(device->hints[1] & 0x20) {
                if(discoverer_name)
                    g_free(discoverer_name);
                discoverer_name = g_strdup(device->name);
                discoverer_hints[0] = device->hints[0];
                discoverer_hints[1] = device->hints[1];
            }

        }
    }

    /* Check for removed devices */
    for(i = 0; i < NumDevices; i++)
    {
        device = g_list_nth_data(DevicesList, i);
        found = FALSE;

        if(list != NULL)
            for(c = 0; c < list_length; c++)
            {
	            device2 = g_list_nth_data(list, c);

	            if(device->daddr == device2->daddr)
	            {
	                found = TRUE;
	                break;
	            }
	        }

        /* Device disappeared */
	    if(found == FALSE)
        {
            g_print("* Device disappeared: %s [0x%08x]\n",
                      device->name, device->daddr);

            notify_on_disappearing (device->name, device->icon);
            gui_update_on_disappearing (list_length);

            if(g_strcmp0(discoverer_name, device->name) == 0) {
                //discoverer_name = NULL;
                discoverer_hints[0] = 0;
                discoverer_hints[1] = 0;
            }

        }
    }

    free_devices_list (DevicesList);
    DevicesList = list;
    NumDevices = list_length;

    g_timeout_add_seconds(DISCOVERER_POLL_INTERVAL,
	                      discover_do, NULL);

    return FALSE;
}


static void device_discovery_start()
{
    g_print("* Start polling of devices in range, interval %d seconds\n",
				        DISCOVERER_POLL_INTERVAL);
	discover_do(NULL);
}

char* discoverer_get_name()
{
	return discoverer_name;
}

int* discoverer_get_hints()
{
	return discoverer_hints;
}

int discoverer_is_in_range()
{
	return NumDevices > 0;
}

static void
on_dialog_response (GtkDialog *dialog, gint response, gpointer ok_handler_data)
{
	if (response == GTK_RESPONSE_CLOSE) {
		gtk_widget_destroy (GTK_WIDGET (dialog));

		DEBUG(5,"Dialog destroyed\n");
	}
}

GtkWidget *
show_device_info_cb( void *data )
{
	GtkWidget *info;
	GtkWidget *vbox1;
	GtkWidget *close;
	GtkWidget *table1;
	GtkWidget *label1;
	GtkWidget *ck1;
	GtkWidget *aa1;
	gint i;
	gint y;
	GList *devs;
	dev_info *dev;
	gchar *labels[] = {_("<b>Hints</b>"),
			   _("PNP"),
			   _("PDA"),
			   _("COMPUTER"),
			   _("PRINTER"),
			   _("MODEM"),
			   _("FAX"),
			   _("LAN"),
			   _("EXTENSION"),
			   _("TELEPHONY"),
			   _("FILESERVER"),
			   _("COMM"),
			   _("MESSAGE"),
			   _("HTTP"),
			   _("OBEX"),
			   NULL
	};

	if ( NumDevices <= 0 )
		return 0;

	info = gtk_dialog_new ();

	// Window title
	gtk_window_set_title (GTK_WINDOW (info), _("Information"));
	gtk_window_set_resizable( GTK_WINDOW (info), FALSE );

	// RGBA patch
	//GdkScreen *screen = gtk_widget_get_screen(info);
    	//GdkColormap *colormap = gdk_screen_get_rgba_colormap (screen);

    	//if (colormap)
	//	gtk_widget_set_default_colormap(colormap);

	gtk_window_set_destroy_with_parent (GTK_WINDOW (info), TRUE);
	gtk_dialog_set_default_response (GTK_DIALOG (info), GTK_RESPONSE_CLOSE);

	vbox1 =  gtk_dialog_get_content_area (GTK_DIALOG (info));
	gtk_widget_show (vbox1);

	table1 = gtk_table_new (15, NumDevices+1, TRUE);
	gtk_widget_show (table1);
	gtk_box_pack_start (GTK_BOX (vbox1), table1, TRUE, TRUE, 0);

	/* Create labels */
	i = 0;
	while ( labels[i] ) {
		label1 = gtk_label_new (_(labels[i]));
		gtk_widget_show (label1);
		gtk_table_attach (GTK_TABLE (table1), label1, 0, 1, i, i+1,
				  (GtkAttachOptions) (GTK_FILL),
				  (GtkAttachOptions) (0), 5, 0);
		gtk_label_set_justify (GTK_LABEL (label1), GTK_JUSTIFY_LEFT);
		gtk_label_set_use_markup (GTK_LABEL (label1), TRUE);
		gtk_misc_set_alignment (GTK_MISC (label1), 0, 0.5);
		i++;
	}

	/* Adding checkboxes */
	devs = DevicesList;
	i = 0;
	while ( devs )
	{
		dev = (dev_info*) devs->data;

		label1 = gtk_label_new( dev->name );
		gtk_widget_show (label1);
		gtk_table_attach (GTK_TABLE (table1), label1, i+1, i+2, 0, 1,
				  (GtkAttachOptions) (GTK_FILL),
				  (GtkAttachOptions) (0), 5, 0);
		gtk_label_set_justify (GTK_LABEL (label1), GTK_JUSTIFY_LEFT);
		gtk_misc_set_alignment (GTK_MISC (label1), 0, 0.5);

		y = 1;
		while ( labels[y] ) {
			ck1 = gtk_check_button_new();
			gtk_widget_set_can_focus (ck1, FALSE);
			gtk_widget_set_sensitive (ck1, FALSE);

			if ( (y <= 8) && (dev->hints[0] & 1<<(y-1)) )
				gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(ck1), 1 );

			if ( (y > 7) && (dev->hints[1] & 1<<(y-9)) )
				gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(ck1), 1 );

			gtk_widget_show (ck1);
			gtk_table_attach (GTK_TABLE (table1), ck1, i+1, i+2, y, y+1,
					  (GtkAttachOptions) (GTK_FILL),
					  (GtkAttachOptions) (0), 5, 0);
			y++;
		}
		devs = devs->next;
		i++;
	}

	/* Create close button */
	aa1 = gtk_dialog_get_action_area (GTK_DIALOG (info));
	gtk_widget_show (aa1);
	gtk_button_box_set_layout (GTK_BUTTON_BOX (aa1), GTK_BUTTONBOX_END);

	close = gtk_button_new_from_stock ("gtk-close");
	gtk_widget_show (close);
	gtk_dialog_add_action_widget (GTK_DIALOG (info), close, GTK_RESPONSE_CLOSE);
	gtk_widget_set_can_default (close, TRUE);

	g_signal_connect (GTK_OBJECT (info), "response",
			    G_CALLBACK (on_dialog_response),
			    NULL);

	gtk_widget_show(info);

	return info;
}

int clean_log_file( GtkWidget* w, gpointer data )
{
	GtkTextBuffer *buffer = data;
	GtkTextIter *start_iter, *end_iter;
	char* logfile;

	logfile = get_log_path();

	if(g_remove (logfile))
	{
		g_print("Deleted log file: %s\n", logfile);
		gtk_text_buffer_set_text (buffer, "", -1);
		g_free(logfile);
		return TRUE;
	}
	else
	{
		g_free(logfile);
		return FALSE;
	}
}

static void
on_dialog_log_response(GtkDialog *dialog, gint response, gpointer ok_handler_data)
{
	if (response == GTK_RESPONSE_CLOSE) {
		gtk_widget_destroy (GTK_WIDGET (dialog));

		DEBUG(5,"Log Dialog destroyed\n");
	}
}

GtkWidget* show_log_file( void *data )
{
	GtkWidget *dialog, *button_clear, *button_close;
	GtkWidget *view, *scroll, *frame;
  	GtkTextBuffer *buffer;

	char* logfile;
	char* content;
	FILE* fp;
	int len;

        logfile = get_log_path();

	content = g_malloc0(64 * 1024);

	fp = fopen(logfile, "r");
	if (fp)
		len = fread(content, 1, 64 * 1024, fp);
	else
		len = 0;

	// Dialog
	dialog = gtk_dialog_new();
	gtk_window_set_title (GTK_WINDOW (dialog), _("History"));
	gtk_dialog_set_has_separator(GTK_DIALOG (dialog), FALSE);
	gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);
	g_object_set (dialog, "border_width", 5, NULL);

	// RGBA support
    	GdkScreen *screen = gtk_widget_get_screen(dialog);
    	GdkColormap *colormap = gdk_screen_get_rgba_colormap (screen);

    	if (colormap)
		gtk_widget_set_default_colormap(colormap);

	// Frame

	frame = gtk_frame_new( _("<b>Transfers History</b>") );
	gtk_frame_set_shadow_type (GTK_FRAME(frame), GTK_SHADOW_NONE);
	gtk_container_add (GTK_CONTAINER (gtk_dialog_get_content_area (GTK_DIALOG (dialog))), frame);

	// Frame Label
	gtk_label_set_use_markup( GTK_LABEL( gtk_frame_get_label_widget (GTK_FRAME(frame))), TRUE );

	// Scroll
	scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (scroll),
					GTK_POLICY_AUTOMATIC,
					GTK_POLICY_AUTOMATIC);
	gtk_container_add (GTK_CONTAINER (frame), scroll);
	gtk_widget_set_size_request(scroll, 400, 200);

  	// TextView
  	view = gtk_text_view_new ();
  	gtk_text_view_set_editable( GTK_TEXT_VIEW (view), FALSE );
  	gtk_text_view_set_cursor_visible( GTK_TEXT_VIEW (view), FALSE );
  	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW (scroll), view);

	// Text Buffer
  	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));
  	gtk_text_buffer_set_text (buffer, content, len);

    	// Buttons
    	button_clear = gtk_button_new_from_stock( GTK_STOCK_CLEAR );
	gtk_container_add (GTK_CONTAINER (gtk_dialog_get_action_area (GTK_DIALOG (dialog))), button_clear);

	button_close = gtk_button_new_from_stock( GTK_STOCK_CLOSE );
	gtk_dialog_add_action_widget(GTK_DIALOG (dialog), button_close, GTK_RESPONSE_CLOSE);

	g_signal_connect (GTK_OBJECT (dialog), "response",
			    G_CALLBACK (on_dialog_log_response),
			    NULL);

	g_signal_connect(G_OBJECT (button_clear),
         "clicked",
		   G_CALLBACK (clean_log_file),
		   buffer);

	gtk_widget_show_all(dialog);

	g_free(content);
	if (fp)
		fclose(fp);
	g_free(logfile);
}

unsigned char
ircptray_app_new()
{

    //sanity check
    if(!irda_check()) //no irda support
    {
        GtkWidget* dialog;

        g_printerr("EE: irda kernel module not loaded, quit");

	    dialog = gtk_message_dialog_new(NULL,
                                        GTK_DIALOG_MODAL,
                                        GTK_MESSAGE_ERROR,
                                        GTK_BUTTONS_CLOSE,
                                        _("You don't seem to have IrDA enabled in your kernel, quit."),
                                        NULL);

	    gtk_dialog_run(GTK_DIALOG(dialog));

	    return FALSE;
    }

    if( !check_irda_config() )
    {
		GtkWidget* dialog;

		dialog = gtk_message_dialog_new(NULL,
                                        GTK_DIALOG_MODAL,
                                        GTK_MESSAGE_WARNING,
                                        GTK_BUTTONS_CLOSE,
                                        _("IrDA service not started!"),
                                        NULL);

		gtk_message_dialog_format_secondary_markup(GTK_MESSAGE_DIALOG(dialog),
		_("<i>You must edit <b>\"/etc/default/irda-utils\"</b> and set irattach to start it on boot.\n\nOtherwise, try typing in terminal:\n\n\"sudo irattach /dev/ttyS1 -s\"\n\nRead the irattach manual for more info.\n\n<span color=\"#f00000\" style=\"normal\"><b>%s will not be able to work.</b></span></i>"), g_get_application_name());

	    gtk_window_set_title (GTK_WINDOW(dialog), "Ircp Tray");
		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
    }

    notify_init(PACKAGE);

    AppTrayIcon = trayicon_init();

    recvfile_listen(TRUE);

    device_discovery_start();

    return TRUE;
}
