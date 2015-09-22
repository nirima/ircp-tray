INCLUDES = -I$(top_srcdir) \
	   -DLOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\"	\
	   -DDATADIR=\""$(datadir)"\"					        \
	   -DG_DISABLE_DEPRECATED -DGTK_DISABLE_DEPRECATED		\
	   -DGDK_DISABLE_DEPRECATED

AM_CFLAGS = 		  \
    @GTK_CFLAGS@      \
    @OPENOBEX_CFLAGS@ \
    @LIBNOTIFY_CFLAGS@

bin_PROGRAMS = ircp-tray

ircp_tray_SOURCES =  \
    ircptray.c \
    ircptray_main.c ircptray_main.h \
    ircptray_trayicon.c ircptray_trayicon.h	\
	ircp_io.c ircp_io.h	\
	ircp_client.c ircp_client.h	\
	resources.h	\
	dirtraverse.c dirtraverse.h	\
	sendfile.c sendfile.h	\
	recvfile.c recvfile.h	\
	debug.h		\
	ircp.h		\
	ircp_server.c ircp_server.h	\
	progresswin.c progresswin.h	\
	savefiledlg.c savefiledlg.h	\
	irda_misc.c irda_misc.h

ircp_tray_LDADD = @GTK_LIBS@ @OPENOBEX_LIBS@ @LIBNOTIFY_LIBS@

