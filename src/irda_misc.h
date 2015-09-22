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

#ifndef __IRDA_MISC_H__
#define __IRDA_MISC_H__

#include <glib-2.0/glib.h>

struct _dev_info
{
        gchar *name;
        gchar *icon;
        guint32 daddr;
        guint32 saddr;
        guint8 hints[2];
};

typedef struct _dev_info dev_info;

#define MAX_DEVICES 10

GList*
irda_discover_devices ();

gchar*
get_icon_from_irda_hints (guint8 hints[2]);

void
free_devices_list (GList *dev_list);

//int
//irda_check();


#endif
