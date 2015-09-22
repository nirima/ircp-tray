/* ircp.h
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

#ifndef IRCP_H
#define IRCP_H

#define ircp_return_if_fail(test)       do { if (!(test)) return; } while(0);
#define ircp_return_val_if_fail(test, val)      do { if (!(test)) return val; } while(0);

typedef void (*ircp_info_cb_t)(int event, char *param);

enum {
	IRCP_EV_ERRMSG,

	IRCP_EV_OK,
	IRCP_EV_ERR,

	IRCP_EV_CONNECTING,
	IRCP_EV_DISCONNECTING,
	IRCP_EV_SENDING,

	IRCP_EV_LISTENING,
	IRCP_EV_CONNECTIND,
	IRCP_EV_DISCONNECTIND,
	IRCP_EV_RECEIVING,
};

/* Number of bytes passed at one time to OBEX */
#define STREAM_CHUNK 1024 * 2

#endif
