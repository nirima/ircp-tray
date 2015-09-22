/*
 *  GNOME IrDA Monitor (girda)
 *  Copyright (C) 2001 Ragnar Henriksen <ragnar@juniks.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <glib-2.0/glib.h>
#include <glib/gi18n-lib.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <linux/types.h>
#include <linux/irda.h>
#include <unistd.h>

#include "irda_misc.h"

/* Open the IrDA socket */
static int irda_open_socket()
{
    int fd;

    fd = socket(AF_IRDA, SOCK_STREAM, 0);
	if ( fd == -1 ) {
		return FALSE;
	}

	return fd;
}


/* Return a list of discovered devices on IrDA socket */
GList* irda_discover_devices()
{
    struct irda_device_list *dev_list;
    guint len, i;
    gchar *buf;
    GList *list = NULL;
    dev_info *device;
    int fd;

    len = sizeof(struct irda_device_list) + sizeof(struct irda_device_info) * MAX_DEVICES;
	buf = g_new0(gchar, len);

	dev_list = (struct irda_device_list *) buf;

	fd = irda_open_socket();

	if ( getsockopt(fd, SOL_IRLMP, IRLMP_ENUMDEVICES, dev_list, &len) == 0 )
	{

	    for(i = 0; i < dev_list->len; i++)
	    {
            device = g_malloc(sizeof(dev_info));
            device->name = g_strdup(dev_list->dev[i].info);
            device->daddr = dev_list->dev[i].daddr;
            device->saddr = dev_list->dev[i].saddr;
            device->hints[0] = dev_list->dev[i].hints[0];
            device->hints[1] = dev_list->dev[i].hints[1];
            device->icon = get_icon_from_irda_hints(device->hints);

	        list = g_list_prepend(list, device);
	    }
	    g_free(dev_list);

	}
	else
		list = NULL;

	return list;
}

/* Free the device list returned by IrDA discover */
void free_devices_list(GList *dev_list)
{
	while (dev_list)
	{
        g_free( ((dev_info*)dev_list->data)->name);
        g_free( ((dev_info*)dev_list->data)->icon);

        g_free(dev_list->data);
        dev_list = g_list_delete_link (dev_list, dev_list);
    }
    g_list_free(dev_list);
}

/* Get the icon name for various irda hints */
gchar* get_icon_from_irda_hints( guint8 hints[2] )
{
    gchar *icon_name;

    if(hints[0] & HINT_PDA)
        icon_name = g_strdup("pda");

    else if(hints[0] & HINT_COMPUTER)
        icon_name = g_strdup("computer");

    else if(hints[0] & HINT_PRINTER)
        icon_name = g_strdup("printer");

    else if(hints[1] & HINT_TELEPHONY)
        icon_name = g_strdup("phone");
    else
        icon_name = g_strdup("ircp-tray");

    return icon_name;
}


gchar* get_device_name_by_daddr (GList* list, guint32 daddr)
{
    GList *li;

    li = list;

    while (li)
    {
        if( ((dev_info*)li->data)->daddr == daddr )
            return g_strdup( ((dev_info*)li->data)->name );

        li = g_list_next(li);
    }

    /* Nothing found */
    return 0;
}

int irda_check()
{
	int sock;
	sock = socket(AF_IRDA, SOCK_STREAM, 0);
	if(sock>=0) {
		close(sock);
		return 1;
	} else
		return 0;
}
