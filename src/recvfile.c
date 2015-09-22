/* recvfile.c
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

#include "config.h"
#include <debug.h>
#include <string.h>
#include <glib/gi18n-lib.h>

#include <fcntl.h>
#include <sys/stat.h>

#include <libnotify/notification.h>
#include <libnotify/notify.h>

#include <ircp_server.h>
#include <recvfile.h>
#include <savefiledlg.h>
#include <errno.h>
#include "resources.h"
#include "irda_misc.h"

enum recvfile_status {
	RCV_IDLE,
	RCV_LISTENING,
	RCV_RECEIVING
} RecvStatus;

static char *RemoteName;

extern GtkStatusIcon* AppTrayIcon;

void ircp_info_cb(int event, char *param);

static ircp_server_t* IrcpServer = NULL;
static GIOChannel* IrcpServerChan = NULL;
static struct ProgressWindow* RecvFilePgswin = NULL;
static GTimer* timer = NULL;

static gboolean recvfile_onlisten(GIOChannel *source,
                                             GIOCondition condition,
                                             gpointer data);
static gboolean recvfile_onrecv(GIOChannel *source,
                                             GIOCondition condition,
                                             gpointer data);
void recvfile_launch_dialog(char* filename, char** localfilename, int* fd);

void log_file(const char*);

void recvfile_listen(char enabled)
{
	if(IrcpServer)
	{
		char* reason, *utf8, *logtext;
		if(IrcpServerChan) {
			g_source_remove_by_user_data(IrcpServer);
			g_io_channel_unref(IrcpServerChan);
			IrcpServerChan = NULL;
		}

		if(IrcpServer->finished)
			if(IrcpServer->success)
				reason = _("succeeded");
			else
				reason = _("failed");
		else
			reason = _("aborted");

		if(IrcpServer->localfilename) {
			utf8 = (char*) filename_to_utf8(IrcpServer->localfilename);
			logtext = g_malloc(strlen(utf8)+30);
			g_sprintf(logtext, _("Receive %s %s"), utf8, reason);
			log_file(logtext);
			g_free(logtext);
			g_free(utf8);
		}

		ircp_srv_close(IrcpServer);
		IrcpServer = NULL;
		RecvStatus = RCV_IDLE;
	}

	if(enabled) {
		int obexfd;

		IrcpServer = ircp_srv_open(ircp_info_cb);
		ircp_srv_listen(IrcpServer, ".");

		//create asynchronous i/o channel
		obexfd = OBEX_GetFD(IrcpServer->obexhandle);
		IrcpServerChan = g_io_channel_unix_new(obexfd);
		g_io_add_watch(IrcpServerChan, G_IO_IN, recvfile_onlisten, IrcpServer);
		RecvStatus = RCV_LISTENING;
	}
}

static void recvfile_abort()
{
	printf("Receiving abort\n");

	if(RecvFilePgswin) {
		progress_window_destroy(RecvFilePgswin);
		trayicon_animation_stop();
	}

	RecvFilePgswin = NULL;

	recvfile_listen(1);
}

static gboolean recvfile_onlisten(GIOChannel *source,
                                             GIOCondition condition,
                                             gpointer data)
{
	int ret, obexfd;
	struct _dev_info *devices;
   	struct _dev_info *p;
	int numdev;

	ret = OBEX_HandleInput(IrcpServer->obexhandle, 1);

	//re-register callback
	g_source_remove_by_user_data(IrcpServer);
	g_io_channel_unref(IrcpServerChan);
	//create asynchronous i/o channel
	obexfd = OBEX_GetFD(IrcpServer->obexhandle);
	IrcpServerChan = g_io_channel_unix_new(obexfd);
	g_io_add_watch(IrcpServerChan, G_IO_IN, recvfile_onrecv, IrcpServer);

	RecvStatus = RCV_RECEIVING;

	/*devices = (struct _dev_info *) irda_get_hints_fd( &numdev, obexfd );
	if ( numdev <= 0 )
		printf("none remote device found\n");
	p = devices;
	while ( p ) {
		printf("name: %s, hint0 %x, hint1 %x\n",
				p->name, p->hints[0], p->hints[1]);
		strncpy(RemoteName, p->name, 255);
		RemoteName[255] = 0;
		p = p->next;
	}

	free_dev_info(devices);*/

	RemoteName = g_strdup(discoverer_get_name());

	return FALSE;
}

static gboolean recvfile_onrecv(GIOChannel *source,
                                             GIOCondition condition,
                                             gpointer data)
{
	int ret;
	ircp_server_t* srv = IrcpServer;
	int obexfd;
	double elapsed;
	double transfer_rate;

	ret = OBEX_HandleInput(srv->obexhandle, 1);
	obexfd = OBEX_GetFD(IrcpServer->obexhandle);
	//printf("obexfd %d\n", obexfd);

	DEBUG(4, "HandleInput %d\n", ret);
	if (ret < 0) {
		srv->finished = TRUE;
		srv->success = FALSE;
	}

	if(RecvFilePgswin)
	{
		elapsed = g_timer_elapsed (timer, NULL);
		transfer_rate = srv->finishedsize / elapsed;

		progress_window_update(RecvFilePgswin, srv->finishedsize, srv->filelength, transfer_rate);
	}

	if(srv->fd >= 0 && !RecvFilePgswin) {
		char* utf8;
		utf8 = (char*) filename_to_utf8(srv->filename);
		RecvFilePgswin = progress_window_new(utf8,
					recvfile_abort, NULL);
		g_free(utf8);
		trayicon_animation_start();
		timer = g_timer_new ();
	}

	/*if(srv->object) {
		recvfile_launch_dialog(&srv->fd);
		if(srv->fd<0) {
			perror("Target file open failed");
			OBEX_ObjectSetRsp(srv->object,
					OBEX_RSP_FORBIDDEN,OBEX_RSP_FORBIDDEN);
			srv->finished = TRUE;
			srv->success = FALSE;
		} else
			OBEX_ObjectSetRsp(srv->object, OBEX_RSP_CONTINUE, OBEX_RSP_SUCCESS);

		OBEX_ObjectReadStream(srv->obexhandle,
				srv->object, NULL);
		srv->object = NULL;
	}*/

	if(srv->finished)
	{
		GtkWidget* dialog;
		GtkMessageType type;
		GtkWidget* parent = NULL;
		char* utf8;

		if (srv->localfilename)
		{
			//show transfer result dialog
			utf8 = (char*) filename_to_utf8(srv->localfilename);

			if (srv->success)
				type = GTK_MESSAGE_INFO;
			else
				type = GTK_MESSAGE_WARNING;

			if(RecvFilePgswin)
				parent = RecvFilePgswin->handle;

			dialog = gtk_message_dialog_new(GTK_WINDOW (parent),
							GTK_DIALOG_MODAL, type,
							GTK_BUTTONS_OK,
							"%s", utf8);

			if (srv->success) {
				gchar *title = NULL;
				gchar *msg = NULL;

				NotifyNotification *n = NULL;
				title = g_strdup (_("Infrared file received"));
				msg = g_strdup_printf (_("Saved to: %s\nSize: %d bytes"), utf8, srv->finishedsize);

				notify_init(PACKAGE);



#ifdef NOTIFY_CHECK_VERSION
#if NOTIFY_CHECK_VERSION(0,7,0)
                                n = notify_notification_new (title,
                                       msg,
                                       MAIN_ICON
                                       );
#else
                                n = notify_notification_new (title,
                                       msg,
                                       MAIN_ICON,
                                       GTK_WIDGET(AppTrayIcon));
#endif
#else
                                n = notify_notification_new (title,
                                       msg,
                                       MAIN_ICON,
                                       GTK_WIDGET(AppTrayIcon));
#endif


				if (n != NULL) {
					notify_notification_set_urgency(n, NOTIFY_URGENCY_NORMAL);
					notify_notification_set_timeout (n, NOTIFY_EXPIRES_DEFAULT);

					if (!notify_notification_show (n, NULL))
						g_warning("failed to send notification\n");
					g_object_unref(G_OBJECT(n));
				}

			}
			else {
				gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG (dialog),
                                                         _("File receiving was aborted by remote"));
				gtk_dialog_run(GTK_DIALOG (dialog));

				// delete incomplete file
				unlink(srv->localfilename);
			}

			gtk_widget_destroy (dialog);
			g_free(utf8);
			g_free(RemoteName);
		}

		DEBUG(4,"srv finished, success %d\n", srv->success);
		if(srv->fd>=0) {
			printf("Close FD %d\n", srv->fd);
			close(srv->fd);
			srv->fd = -1;
		}
		if(RecvFilePgswin) {
			progress_window_destroy(RecvFilePgswin);
			trayicon_animation_stop();
			g_timer_destroy (timer);
		}
		RecvFilePgswin = NULL;

		recvfile_listen(1);

		return FALSE;
	}
	else
		return TRUE;

}

void recvfile_launch_dialog(char* filename_u, char** localfilename, int* fd)
{

	struct SaveFileDialog* dialog;

	*fd = -1;

	dialog = savefile_dialog_new(_("Incoming File Transfer"), RemoteName);

	savefile_dialog_set_current_name(dialog, filename_u);

	if (savefile_dialog_run (dialog) == GTK_RESPONSE_YES)
	{
		char *filename;

		filename = savefile_dialog_get_filename(dialog);

		if( access(filename, F_OK) >= 0)
		{
			// Asks the user whether or not overwrite.
			GtkWidget *message_dialog;

			message_dialog = gtk_message_dialog_new(NULL,
			 				GTK_DIALOG_MODAL,
			 				GTK_MESSAGE_QUESTION,
			 				GTK_BUTTONS_CANCEL,
			 				_("File already exists"));

			gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG(message_dialog),
				_("The file \"%s\" already exists. Would you like to replace it?"), filename);

			gtk_dialog_add_button (GTK_DIALOG(message_dialog),
						_("_Replace"),
						GTK_RESPONSE_OK);
			gtk_dialog_set_default_response (GTK_DIALOG(message_dialog), GTK_RESPONSE_OK);

			if( gtk_dialog_run(GTK_DIALOG(message_dialog)) == GTK_RESPONSE_CANCEL)
			{
				gtk_widget_destroy(message_dialog);
				savefile_dialog_destroy (dialog);
				return;
			}

			gtk_widget_destroy(message_dialog);
		}

		*fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, DEFFILEMODE);

		if(*fd<0) {
			char msgbuf[1024];
			char* unistr;
			GtkWidget* dialog;

			unistr = g_locale_to_utf8(g_strerror(errno), -1, NULL, NULL, NULL);
			snprintf(msgbuf, 1024, "%s, %s", _("Unable to open file for saving"), unistr);
			g_free(unistr);
			dialog = gtk_message_dialog_new(NULL, 0, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, msgbuf);
			gtk_dialog_run(GTK_DIALOG(dialog));
			gtk_widget_destroy(dialog);
		}


		*localfilename = filename;
	}else
		DEBUG(5,"Recvfile dialog quit\n");

	savefile_dialog_destroy (dialog);
}
