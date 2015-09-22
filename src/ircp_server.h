/* ircp_server.h
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

#ifndef IRCP_SERVER_H
#define IRCP_SERVER_H

#include <openobex/obex.h>
#include "ircp.h"

typedef struct ircp_server
{
	obex_t *obexhandle;
	int finished;
	int success;
	char *inbox;
	ircp_info_cb_t infocb;
	int fd;
	int dirdepth;

	char* filename;
	char* localfilename;
	int filelength;
	int finishedsize;
} ircp_server_t;

int ircp_srv_receive(ircp_server_t *srv, obex_object_t *object, int finished);
int ircp_srv_setpath(ircp_server_t *srv, obex_object_t *object);

ircp_server_t *ircp_srv_open(ircp_info_cb_t infocb);
void ircp_srv_close(ircp_server_t *srv);
int ircp_srv_recv(ircp_server_t *srv, char *inbox);


#endif
