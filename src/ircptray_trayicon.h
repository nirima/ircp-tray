/* ircptray_trayicon.h
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

#ifndef IRCPTRAY_TRAYICON_H
#define IRCPTRAY_TRAYICON_H

GtkStatusIcon*
trayicon_init();

void
trayicon_set_info_menuitem_sensitive (gboolean value);

void
trayicon_set_tooltip_text(char* str);

void
trayicon_set_icon(char* icon_name);

void
trayicon_animation_start();

void
trayicon_animation_stop();

#endif /* IRCPTRAY_TRAYICON_H */
