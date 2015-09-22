/* ircp_client.h
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

#ifndef IRCP_CLIENT_H
#define IRCP_CLIENT_H

#include <openobex/obex.h>
#include "ircp.h"

typedef struct ircp_client
{
	obex_t *obexhandle;
	int finished;
	int success;
	int obex_rsp;
	ircp_info_cb_t infocb;
	int fd;
	uint8_t *buf;
	
	long long totalsize;
	long long finishedsize;
} ircp_client_t;


ircp_client_t *ircp_cli_open(ircp_info_cb_t infocb);
void ircp_cli_close(ircp_client_t *cli);
int ircp_cli_connect(ircp_client_t *cli);
int ircp_cli_disconnect(ircp_client_t *cli);
int ircp_put(ircp_client_t *cli, char *name);
	
#endif
