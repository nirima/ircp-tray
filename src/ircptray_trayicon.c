/* ircptray_trayicon.c
 *
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

#include "config.h"
#include "debug.h"

#include <gtk/gtk.h>
#include <glib/gi18n-lib.h>
#include <string.h>

#include "ircptray_main.h"
#include "resources.h"
#include "sendfile.h"


static gboolean trayicon_animation_ontimer (gpointer data);

static int TrayIconAnimationRefCount=0;
static int TrayIconTimerId = 0;
static char* TrayIconFileName = ICON_TRAY_INACTIVE;

static GtkWidget *DeviceInfoMenuitem;
static GtkStatusIcon *TrayIcon;

static void trayicon_on_click(GtkStatusIcon *trayicon,
                	gpointer       data)
{
	sendfile_launch_dialog();
}

static void trayicon_on_popup (GtkStatusIcon *trayicon,
                               guint          button,
                               guint          activate_time,
                               gpointer       data)
{
  	gtk_menu_popup(GTK_MENU(data), NULL, NULL, gtk_status_icon_position_menu, trayicon,
    			button,
    			activate_time);
}

static void sendfile_onclick(GtkMenuItem *menuitem, gpointer user_data)
{
	sendfile_launch_dialog();
}

GtkStatusIcon* trayicon_init()
{
    GtkWidget* menu, *menuitem;
    GtkWidget* image;

	TrayIcon = gtk_status_icon_new_from_icon_name(TrayIconFileName);

	//building icon popup menu
	menu = gtk_menu_new();

	// Send file item
	menuitem = gtk_image_menu_item_new_with_mnemonic( _("Send Files") );
	image = gtk_image_new_from_stock (GTK_STOCK_GOTO_TOP, GTK_ICON_SIZE_MENU);
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM (menuitem), image);

	gtk_menu_shell_append (GTK_MENU_SHELL(menu), GTK_WIDGET(menuitem));
	g_signal_connect (G_OBJECT(menuitem),
			          "activate",
			          G_CALLBACK(sendfile_onclick),
			           NULL);

	// Device info item
	menuitem = gtk_image_menu_item_new_with_mnemonic( _("Get Devices Info") );
	image = gtk_image_new_from_stock (GTK_STOCK_INFO, GTK_ICON_SIZE_MENU);
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM (menuitem), image);
	gtk_container_add(GTK_CONTAINER(menu), menuitem);
	g_signal_connect (G_OBJECT(menuitem),
                      "activate",
                      G_CALLBACK(show_device_info_cb),
                      NULL);
	gtk_widget_set_sensitive(menuitem, FALSE);

	DeviceInfoMenuitem = menuitem;

	// History item
	menuitem = gtk_image_menu_item_new_with_mnemonic( _("Show History") );
	gtk_container_add(GTK_CONTAINER(menu), menuitem);
        g_signal_connect(G_OBJECT(menuitem),
                        "activate",
                        G_CALLBACK(show_log_file),
                        NULL);

	menuitem = gtk_separator_menu_item_new();
	gtk_container_add(GTK_CONTAINER(menu), menuitem);

	// About menu item
	menuitem = gtk_image_menu_item_new_from_stock(GTK_STOCK_ABOUT, NULL);
	gtk_container_add(GTK_CONTAINER(menu), menuitem);
	g_signal_connect(G_OBJECT(menuitem),
			"activate",
			G_CALLBACK(version_onclick),
			NULL);

	// Exit menu item
	menuitem = gtk_image_menu_item_new_from_stock(GTK_STOCK_QUIT, NULL);
	gtk_container_add(GTK_CONTAINER(menu), menuitem);
	g_signal_connect(G_OBJECT(menuitem),
			"activate",
			G_CALLBACK(gtk_main_quit),
			NULL);

	gtk_widget_show_all(menu);

	g_signal_connect(G_OBJECT(TrayIcon), "activate",
        	G_CALLBACK(trayicon_on_click), NULL);

    g_signal_connect(G_OBJECT(TrayIcon), "popup-menu",
        	G_CALLBACK(trayicon_on_popup), menu);

	gtk_status_icon_set_tooltip (TrayIcon , _("Send files via infrared"));

	gtk_status_icon_set_visible (TrayIcon, TRUE);

	return TrayIcon;

}

void trayicon_set_info_menuitem_sensitive(gboolean value)
{
    gtk_widget_set_sensitive(DeviceInfoMenuitem, value);
}


void trayicon_set_tooltip_text(char* str)
{
	gtk_status_icon_set_tooltip(TrayIcon, str);

}

void trayicon_set_icon(char* icon_name)
{
	TrayIconFileName = icon_name;
	gtk_status_icon_set_from_icon_name(TrayIcon, icon_name);
}

static gboolean trayicon_animation_ontimer (gpointer data)
{

	char* anim_table[] = {
		ICON_TRAY_ANIM1,
		ICON_TRAY_ANIM2,
		ICON_TRAY_ANIM3,
		ICON_TRAY_ANIM4
	};

	static int anim_index = 0;
	int anim_num = sizeof(anim_table)/sizeof(char*);

	gtk_status_icon_set_from_icon_name(TrayIcon, anim_table[anim_index]);

	anim_index = (anim_index+1) % anim_num;

	return TRUE;
}

void trayicon_animation_start()
{
	if(TrayIconAnimationRefCount == 0)
	{
		//setup a timer
		TrayIconTimerId = g_timeout_add_seconds_full(
							G_PRIORITY_LOW,
							1, //1 second
							trayicon_animation_ontimer,
							NULL, NULL);
	}
	TrayIconAnimationRefCount++;
	DEBUG(5,"Animation start %d\n", TrayIconAnimationRefCount);
}

void trayicon_animation_stop()
{
	TrayIconAnimationRefCount--;
	DEBUG(5,"Animation stop %d\n", TrayIconAnimationRefCount);

	if(TrayIconAnimationRefCount == 0) {
		//destroy timer
		g_source_remove(TrayIconTimerId);
		//restore default
		trayicon_set_icon(TrayIconFileName);
	}
}

