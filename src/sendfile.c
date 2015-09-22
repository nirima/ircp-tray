/* sendfile.c
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
#include <errno.h>
#include <glib/gi18n.h>
#include <string.h>

#include "sendfile.h"
#include "ircp_client.h"
#include "debug.h"

char* filename_to_utf8(const char* filename);
void log_file(const char*);

// must be in a app.h header
char* config_get_lastsavedir(void);

static GTimer* timer = NULL;

void ircp_info_cb(int event, char *param)
{
	switch (event) {

	case IRCP_EV_ERRMSG:
		printf("Error: %s\n", param);
		break;

	case IRCP_EV_ERR:
		printf("Failed. Error %d: %s\n", errno, strerror(errno));
		break;
	case IRCP_EV_OK:
		printf("Done\n");
		break;

	case IRCP_EV_CONNECTING:
		printf("Connecting...\n");
		break;
	case IRCP_EV_DISCONNECTING:
		printf("Disconnecting...\n");
		break;
	case IRCP_EV_SENDING:
		printf("Sending %s...\n", param);
		break;
	case IRCP_EV_RECEIVING:
		printf("Receiving %s...\n", param);
		break;

	case IRCP_EV_LISTENING:
		printf("Waiting for incoming connection\n");
		break;

	case IRCP_EV_CONNECTIND:
		printf("Incoming connection\n");
		break;
	case IRCP_EV_DISCONNECTIND:
		printf("Disconnecting\n");
		break;
	default:
		DEBUG(4, "Unknown Event ID 0x%x\n", event);
	}
}

/*
send_session::
new()
destroy()
*/

struct SendSession {
	struct ProgressWindow* window;
	ircp_client_t* ircp_client;
	char* filename_u;

	GIOChannel* obexchan;
};

void send_session_destroy(struct SendSession* sess);
static gboolean send_session_onrecv(GIOChannel *source,
                                             GIOCondition condition,
                                             gpointer data);

//called from tray icon menu click
struct SendSession* send_session_new(char* filename)
{
	struct SendSession* sendsess;
	int ret;
	char* remotename, *remote_u;
	int obexfd;

	//create the context
	sendsess = (struct SendSession*)g_malloc(sizeof(struct SendSession));

	printf("Inizializing to beam file %s\n", filename);

	//create socket
	sendsess->ircp_client = ircp_cli_open(ircp_info_cb);
	if(sendsess->ircp_client == NULL) {
		printf("Error opening ircp-client\n");
		g_free(sendsess);
		return;
	}

	//connect to remote
	if(ircp_cli_connect(sendsess->ircp_client) < 0)
	{
		//error, clean and exit
		char msgbuf[1024];
		char* unistr;
		GtkWidget* dialog;

		unistr = g_locale_to_utf8((char*)strerror(errno), -1, NULL, NULL, NULL);
		snprintf(msgbuf, 1024, "%s, %s", _("Unable to connect remote"), unistr);
		g_free(unistr);
		dialog = gtk_message_dialog_new(NULL, 0, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, msgbuf);
		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);

		ircp_cli_close(sendsess->ircp_client);
		g_free(sendsess);
		return;
	}

	remote_u = filename_to_utf8(filename);
	DEBUG(4,"UTF-8 name: %s\n", remote_u);

	//create progress window and show it
	sendsess->window = progress_window_new(remote_u, send_session_destroy, sendsess);

	//create asynchronous i/o channel
	obexfd = OBEX_GetFD(sendsess->ircp_client->obexhandle);
	sendsess->obexchan = g_io_channel_unix_new(obexfd);
	g_io_add_watch(sendsess->obexchan, G_IO_IN, send_session_onrecv, sendsess);

	// Strip /'s before sending file
	remotename = strrchr(remote_u, '/');
	if(remotename == NULL)
		remotename = remote_u;
	else
		remotename++;

	sendsess->filename_u = g_strdup(remotename);
	ret = ircp_put_file_a(sendsess->ircp_client, filename, remotename);
	g_free(remote_u);

	DEBUG(4,"ircp_put_file_a returns %d\n", ret);
	// start tray icon animation
	trayicon_animation_start();
	// start timer for tranfer rate
	timer = g_timer_new ();
	// failure
	if(ret != 0)
		send_session_destroy(sendsess);
}

void send_session_destroy(struct SendSession* sess)
{
	char* logtext;
	char* reason;

	DEBUG(5,"\n");

	progress_window_destroy(sess->window);

	while (g_source_remove_by_user_data(sess));

	g_io_channel_unref(sess->obexchan);

	// destroy timer for transfer rate
	g_timer_destroy (timer);
	// stato tray icon animation
	trayicon_animation_stop();

	if (sess->ircp_client->finished)
		if (sess->ircp_client->success)
			reason = _("succeeded");
		else
			reason = _("failed");
	else
		reason = _("aborted");

        // TEST IT
	ircp_cli_disconnect(sess->ircp_client);
	close(sess->ircp_client->fd);
	ircp_cli_close(sess->ircp_client);
	logtext = g_malloc(strlen(sess->filename_u)+30);
	sprintf(logtext, _("Sending %s %s"), sess->filename_u,
			reason);
	log_file(logtext);
	g_free(logtext);
	g_free(sess->filename_u);
	g_free(sess);
}

static gboolean send_session_onrecv(GIOChannel *source,
                                    GIOCondition condition,
                                    gpointer data)
{
	int ret;
	double elapsed;
	double transfer_rate;

	struct SendSession* sess = (struct SendSession*)data;

	ret = cli_sync_request_continue(sess->ircp_client);

	DEBUG(4,"request return %d\n", ret);
	if(ret != 0)
	{
		GtkWidget* dialog;
		GtkMessageType type;
		DEBUG(4,"send session quit. finished %d, success %d\n",
				sess->ircp_client->finished,
				sess->ircp_client->success);

		if (sess->ircp_client->success)
			type = GTK_MESSAGE_INFO;
		else
			type = GTK_MESSAGE_WARNING;

		dialog = gtk_message_dialog_new(GTK_WINDOW(sess->window->handle),
						GTK_DIALOG_MODAL, type,
						GTK_BUTTONS_OK,
						"%s", sess->filename_u);

		if (sess->ircp_client->success)
			gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog),
                                                     _("File was sent successfully"));
		else
			gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog),
                                                     _("File beaming was aborted by remote"));

		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy (dialog);
		send_session_destroy(sess);
	}
	else
	{
		elapsed = g_timer_elapsed (timer, NULL);
		transfer_rate = sess->ircp_client->finishedsize / elapsed;
		progress_window_update(sess->window, sess->ircp_client->finishedsize,
					sess->ircp_client->totalsize, transfer_rate);
	}
	return TRUE;
}

/* Pop up dialog asking "OK to proceed" */
static gboolean sendfile_launch_dialog_confirm()
{
	GtkWidget *dialog;
	gboolean result;
	dialog = gtk_message_dialog_new(NULL,
                                    GTK_DIALOG_MODAL,
                                    GTK_MESSAGE_QUESTION,
                                    GTK_BUTTONS_OK_CANCEL,
                                    _("It seems no active device around here, Do you still want to continue?\n"));
	result = (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_OK);
	gtk_widget_destroy (dialog);
	return result;
}

void sendfile_launch_dialog()
{
	GtkWidget *dialog;
	char* path;

	/* look if any device in range */
	if (!discoverer_is_in_range()) {
		if (!sendfile_launch_dialog_confirm())
			return;
	}

	dialog = gtk_file_chooser_dialog_new (_("Select File to Beam"),
                                          NULL,
                                          GTK_FILE_CHOOSER_ACTION_OPEN,
                                          GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                          GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
                                          NULL);

	path = config_get_lastsavedir();
	if(path) {
		gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), path);
		g_free(path);
	}

	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
	{
		char *filename;

		filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
		send_session_new(filename);
		g_free (filename);
	}else
		DEBUG(5,"sendfile dialog destroyed\n");

	  gtk_widget_destroy (dialog);
}
