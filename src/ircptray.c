/* ircptray.c
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


#include <config.h>

#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "ircptray_main.h"
#include "resources.h"

#define MAXLINE 128

/*
static void
getparam(char *s, char *argv[], char *errmes)
{
    if (*argv != NULL && argv[0][0] != '-') {
        strncpy(s, *argv, MAXLINE - 1);
    } else {
        /* Translators: the following string contains two strings that
         * are passed to it: the first is the gcalctool program name and
         * the second is an error message (see the last parameter in the
         * getparam() call in the get_options() routine below.

        fprintf(stderr, _("%s: %s as next argument.\n"), argv[0], errmes);
        exit(1);
    }
}*/

static void
usage()
{
    /* Translators: the following string contains one strings that
     * is passed to it: is the ircp-tray program name.
     */
    g_printerr(_("Usage:\n  %s: [-v] [-h]\n"), PACKAGE);
}

static void
version()
{
    /* Translators: the following string contains two strings that
     * are passed to it: the first is the ircp-tray program name and
     * the second is the program version number.
     */
    g_printerr(_("%s version %s\n\n"), PACKAGE, VERSION);
}

#define INC { argc--; argv++; }

static unsigned char
get_options(int argc, char *argv[])      /* Extract command line options. */
{

    INC;
    while (argc > 0) {
        if (argv[0][0] == '-') {
            switch (argv[0][1]) {
                case 'v' :
                    version();
                    return TRUE;
                    break;
                case 'h' :
                case '?' :
                    version();
                    usage();
                    return TRUE;
                    break;
            }
            INC;
        } else {
            INC;
        }
    }
    return FALSE;
}

int
main(int argc, char** argv)
{

    bindtextdomain(GETTEXT_PACKAGE, LOCALE_DIR);
    bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
    textdomain(GETTEXT_PACKAGE);

    if (!get_options(argc, argv))
    {

        gtk_init(&argc, &argv);

        gtk_icon_theme_append_search_path(gtk_icon_theme_get_default (),
			DATADIR G_DIR_SEPARATOR_S "icons");

        gtk_window_set_default_icon_name(MAIN_ICON);

        g_set_application_name("Ircp Tray");

        if(ircptray_app_new() == TRUE)
            gtk_main();
        else
            return 1;
    }
    return 0;

}
