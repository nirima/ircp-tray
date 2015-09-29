/* ircp_server.c
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

#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <unistd.h>

#include <openobex/obex.h>

#include "ircp.h"
#include "ircp_io.h"
#include "ircp_server.h"
#include "debug.h"

#define TRUE  1
#define FALSE 0

static void get_incoming_file_info(obex_t* handle, obex_object_t* object, ircp_server_t* srv);

//
// Incoming event from OpenOBEX
//
void srv_obex_event(obex_t *handle, obex_object_t *object, int mode, int event, int obex_cmd, int obex_rsp)
{
	ircp_server_t *srv;
	int ret;
		
	srv = OBEX_GetUserData(handle);

	if(!srv->localfilename && !srv->finished && object)
		get_incoming_file_info(handle, object, srv);
	
	DEBUG(4,"OBEX Event: 0x%x Command: 0x%x\n", event, obex_cmd);

	if(srv->finished) {
		printf("Refuse remote offer\n");
		OBEX_ObjectSetRsp(object, 
					OBEX_RSP_FORBIDDEN,OBEX_RSP_FORBIDDEN);
		return;
	}

	DEBUG(5, "OBEX OBJECT\n");
	if( object == NULL )
		DEBUG(5,"(null)\n");
	else
	{

	}


	switch (event)	{
	
	/* Time to pick up data when receiving a stream */
	case OBEX_EV_STREAMAVAIL:
		DEBUG(4, "Time to read some data from stream\n");
		ret = ircp_srv_receive(srv, object, FALSE);
		break;
		
	/* Progress has been made */
	case OBEX_EV_PROGRESS:
		break;
		
	/* An incoming request has arrived */
	case OBEX_EV_REQ:
		DEBUG(4, "OBEX_EV_REQ: Incoming request %02x, %d\n", obex_cmd, OBEX_CMD_SETPATH);

		switch(obex_cmd) {
		case OBEX_CMD_CONNECT:
			DEBUG(4, "OBEX_CMD_CONNECT\n");
			srv->infocb(IRCP_EV_CONNECTIND, "");
			ret = 1;
			break;
		case OBEX_CMD_DISCONNECT:
			DEBUG(4, "OBEX_CMD_DISCONNECT\n");
			srv->infocb(IRCP_EV_DISCONNECTIND, "");	
			ret = 1;
			break;

		case OBEX_CMD_PUT:
			DEBUG(4, "OBEX_CMD_PUT\n");
			ret = ircp_srv_receive(srv, object, TRUE);
			break;

		case OBEX_CMD_SETPATH:
			DEBUG(4, "OBEX_CMD_SETPATH\n");
			ret = ircp_srv_setpath(srv, object);
			break;
		default:
			DEBUG(4,"Unknown %d\n", obex_cmd);
			ret = 1;
			break;
		}

		if(ret < 0) {
			srv->finished = TRUE;
			srv->success = FALSE;
		}
			
		break;
		
	/* An incoming request is about to come */
	case OBEX_EV_REQHINT:
	DEBUG(4, "OBEX_EV_REQHINT\n");
		/* An incoming request is about to come. Accept it! */
		switch(obex_cmd) {
		case OBEX_CMD_PUT:
			DEBUG(4, "Going to turn streaming on!\n");
			/* Set response to ok! */
			OBEX_ObjectSetRsp(object, OBEX_RSP_CONTINUE, OBEX_RSP_SUCCESS);
			/* Turn streaming on */
			OBEX_ObjectReadStream(handle, object, NULL);
			break;
		
		case OBEX_CMD_SETPATH:
		case OBEX_CMD_CONNECT:
		case OBEX_CMD_DISCONNECT:
			/* Set response to ok! */
			OBEX_ObjectSetRsp(object, OBEX_RSP_CONTINUE, OBEX_RSP_SUCCESS);
			break;
		
		default:
			DEBUG(0, "Skipping unsupported command:%02x\n", obex_cmd);
			OBEX_ObjectSetRsp(object, OBEX_RSP_NOT_IMPLEMENTED, OBEX_RSP_NOT_IMPLEMENTED);
			break;
		
		}
		break;
		
	/* First packet of an incoming request has been parsed */
	case OBEX_EV_REQCHECK:
	    DEBUG(4, "OBEX_EV_REQCHECK\n");
		srv->infocb(IRCP_EV_RECEIVING, srv->filename);	
		break;
		
	/* Request has finished */
	case OBEX_EV_REQDONE: // 3
		DEBUG(4, "OBEX_EV_REQDONE\n");
		if(obex_cmd == OBEX_CMD_DISCONNECT) {
			DEBUG(4," -- OBEX_CMD_DISCONNECT\n");
			srv->finished = TRUE;
			srv->success = TRUE;
		}
		break;
		
	/* Link has been disconnected */
	case OBEX_EV_LINKERR:
		DEBUG(0, "Link error\n");
		srv->finished = TRUE;
		srv->success = FALSE;
		break;
	/* Request was aborted */
	case OBEX_EV_ABORT:
		DEBUG(3, "Request aborted\n");
		srv->finished = TRUE;
		srv->success = FALSE;
		break;
	default:
		printf("Unmanaged OBEX Event 0x%x\n",event);
		break;
	}
}

//
//
//
int ircp_srv_sync_wait(ircp_server_t *srv)
{
	int ret;
	while(srv->finished == FALSE) {
		ret = OBEX_HandleInput(srv->obexhandle, 1);
		if (ret < 0)
			return -1;
	}
	if(srv->success)
		return 1;
	else
		return -1;
}

//
// Change current dir after some sanity-checking
//
int ircp_srv_setpath(ircp_server_t *srv, obex_object_t *object)
{
	obex_headerdata_t hv;
	uint8_t hi;
	int hlen;

	uint8_t *nonhdr_data = NULL;
	int nonhdr_data_len;
	char *name = NULL;
	int ret = -1;

	DEBUG(4, "\n");
	nonhdr_data_len = OBEX_ObjectGetNonHdrData(object, &nonhdr_data);
	if(nonhdr_data_len != 2) {
		DEBUG(0, "Error parsing SetPath-command\n");
		return -1;
	}

	while(OBEX_ObjectGetNextHeader(srv->obexhandle, object, &hi, &hv, &hlen))	{
		switch(hi)	{
		case OBEX_HDR_NAME:
			if( (name = malloc(hlen / 2)))	{
				OBEX_UnicodeToChar(name, hv.bs, hlen);
			}
			break;
		default:
			DEBUG(2, "Skipped header %02x\n", hi);
		}
	}

	DEBUG(2, "Got setpath flags=%d, name=%s\n", nonhdr_data[0], name);

	// If bit 0 is set we shall go up
	if(nonhdr_data[0] & 1) {
		/* Cannot cd above inbox */
		if(srv->dirdepth == 0)
			goto out;
		
		if(chdir("..") == -1)
			goto out;

		srv->dirdepth--;
	}
	else {
		if(name == NULL) {
			DEBUG(4, "NULL Name\n");
			return 1;
			
			//goto out;
		}
		
		// A setpath with empty name meens "goto root"
		if(strcmp(name, "") == 0) {
			if(chdir(srv->inbox) == -1)
				goto out;
			srv->dirdepth = 0;
		}
		else {
			DEBUG(4, "Going down to %s\n", name);
			if(ircp_checkdir("", name, CD_CREATE) < 0)
				goto out;
			if(chdir(name) == -1)
				goto out;
			srv->dirdepth++;		
		}
	}
				
	ret = 1;

out:
	if(ret < 0)
		OBEX_ObjectSetRsp(object, OBEX_RSP_FORBIDDEN, OBEX_RSP_FORBIDDEN);
	free(name);
	return ret;
}


//
// Open a file for receivning
//
static int new_file(ircp_server_t *srv, obex_object_t *object)
{
	obex_headerdata_t hv;
	uint8_t hi;
	int hlen;
	char *name = NULL;
	int ret = -1;

	DEBUG(0, "new_file\n");
	/* First iterate through recieved header to find name */
	while(OBEX_ObjectGetNextHeader(srv->obexhandle, object, &hi, &hv, &hlen))	{
		switch(hi)	{
		case OBEX_HDR_NAME:
			DEBUG(0, "hlen = %d\n", hlen);
			if( (name = malloc(hlen / 2)))	{
				OBEX_UnicodeToChar(name, hv.bs, hlen);
			}
			break;
		case OBEX_HDR_LENGTH:
			printf("LENGTH: %d\n", hv.bq4);	
		default:
			DEBUG(4, "Skipped header %02x\n", hi);
		}
	}
	if(name == NULL)	{
		DEBUG(0, "Got a PUT without a name. Refusing\n");
		/* Send back error */
		OBEX_ObjectSetRsp(object, OBEX_RSP_BAD_REQUEST, OBEX_RSP_BAD_REQUEST);
		srv->infocb(IRCP_EV_ERR, "");
		goto out;
	}

	srv->infocb(IRCP_EV_RECEIVING, name);	
	srv->fd = ircp_open_safe(srv->inbox, name);
	
	ret = srv->fd;

out:	g_free(name);
	return ret;
}

//
// Extract interesting things from object and save to disk.
//
int ircp_srv_receive(ircp_server_t *srv, obex_object_t *object, int finished)
{
	const uint8_t *body = NULL;
	int body_len = 0;

	if(srv->fd < 0 && finished == FALSE) {
		/* Not receiving a file */
		if(new_file(srv, object) < 0)
			return 1;
	}
	
	if(finished == TRUE) {
        	/* Recieve done! */
		DEBUG(4, "Done!...\n");
		return 1;		
	}
	else if (srv->fd > 0) {
		/* fd is valid. We are currently receiving a file */
		body_len = OBEX_ObjectReadStream(srv->obexhandle, object, &body);
		DEBUG(4, "Got %d bytes of stream-data\n", body_len);
		
		if(body_len < 0) {
			/* Error */
		}
		else if(body_len == 0) {
			/* EOS */
			close(srv->fd);
	        	srv->fd = -1;
			free(srv->filename);
			srv->filename = NULL;
			//free(srv->localfilename);
			//srv->localfilename = NULL;
			srv->infocb(IRCP_EV_OK, "");
		}
		else {
			if(srv->fd > 0) {
				write(srv->fd, body, body_len);
				srv->finishedsize += body_len;
			}
		}
		return 1;
	}
	return -1;
}	

//
// Create an ircp server
//
ircp_server_t *ircp_srv_open(ircp_info_cb_t infocb)
{
	ircp_server_t *srv;

	DEBUG(4, "\n");
	srv = malloc(sizeof(ircp_server_t));
	memset(srv, 0, sizeof(ircp_server_t));
	if(srv == NULL)
		return NULL;

	srv->fd = -1;
	srv->infocb = infocb;

#ifdef DEBUG_TCP
	srv->obexhandle = OBEX_Init(OBEX_TRANS_INET, srv_obex_event, 0);
#else
	srv->obexhandle = OBEX_Init(OBEX_TRANS_IRDA, srv_obex_event, 0);
#endif

	if(srv->obexhandle == NULL) {
		free(srv);
		return NULL;
	}
	OBEX_SetUserData(srv->obexhandle, srv);
	return srv;
}

//
// Close an ircp server
//
void ircp_srv_close(ircp_server_t *srv)
{
	DEBUG(4, "\n");
	ircp_return_if_fail(srv != NULL);

	OBEX_Cleanup(srv->obexhandle);
	if(srv->filename)
		free(srv->filename);
	if(srv->localfilename)
		free(srv->localfilename);
	if(srv->fd>=0)
		close(srv->fd);
	free(srv);
}

//
// Wait for incoming files.
//
int ircp_srv_recv(ircp_server_t *srv, char *inbox)
{
	int ret;
	
	if(ircp_checkdir("", inbox, CD_ALLOWABS) < 0) {
		srv->infocb(IRCP_EV_ERRMSG, "Specified destination directory does not exist.");
		return -1;
	}

	/* Start receiving files in inbox */
	if(chdir(inbox) == -1) {
		perror("chdir failed");
		return -1;
	}
	srv->dirdepth = 0;
			
	if(IrOBEX_ServerRegister(srv->obexhandle, "OBEX") < 0)
		return -1;
	srv->infocb(IRCP_EV_LISTENING, "");
	srv->inbox = inbox;
	
	ret = ircp_srv_sync_wait(srv);
	
	/* Go back to inbox */
	chdir(inbox);
	
	return ret;
}

//
// Wait for incoming files.
//
int ircp_srv_listen(ircp_server_t *srv, char *inbox)
{
	int ret;
	
	if(ircp_checkdir("", inbox, CD_ALLOWABS) < 0) {
		srv->infocb(IRCP_EV_ERRMSG, "Specified desination directory does not exist.");
		return -1;
	}

	/* Start receiving files in inbox */
	if(chdir(inbox) == -1) {
		perror("chdir failed");
		return -1;
	}
	srv->dirdepth = 0;
			
	if(IrOBEX_ServerRegister(srv->obexhandle, "OBEX") < 0)
		return -1;
	srv->infocb(IRCP_EV_LISTENING, "");
	srv->inbox = inbox;
	
	return ret;
}

static void get_incoming_file_info(obex_t* handle, obex_object_t* object, ircp_server_t* srv)
{
	obex_headerdata_t hv;
	uint8_t hi;
	int hlen;
	char* utf8;
		
	DEBUG(4,"get_incoming_file_info");

	while(OBEX_ObjectGetNextHeader(handle, object, &hi, &hv, &hlen))
	{
		switch(hi)	{
		case OBEX_HDR_NAME:
			utf8 = (char*) unicode_to_utf8(hv.bs, hlen);
			srv->filename = (char*) filename_from_utf8(utf8);
			printf("** File Name: %s\n", srv->filename);
			free(utf8);
			break;
		case OBEX_HDR_LENGTH:
			printf("** File Length: %d bytes\n", hv.bq4);
			srv->filelength = hv.bq4;	
		default:
			DEBUG(4, "Skipped header %02x\n", hi);
		}
	}	
	
	//OBEX_ObjectReParseHeaders(handle, object);

	if(srv->filename) {
		utf8 = (char*) filename_to_utf8(srv->filename);
		recvfile_launch_dialog(utf8, &srv->localfilename, 
						&srv->fd);
		free(utf8);
		
		DEBUG(4,"filename %s local %s fd %d\n", srv->filename, srv->localfilename, srv->fd);
		
		if(srv->fd<0) {

			

			srv->finished = TRUE;
			srv->success = FALSE;
		}
	}
}
