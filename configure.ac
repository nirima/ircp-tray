AC_INIT([ircp-tray], [0.7.6], [https://launchpad.net/ircp-tray])
AC_CONFIG_SRCDIR([src/ircptray.c])
AM_INIT_AUTOMAKE
AM_CONFIG_HEADER(config.h)

dnl Initialize maintainer mode
AM_MAINTAINER_MODE

AC_ISC_POSIX
AC_PROG_CC
AM_PROG_CC_STDC
AC_HEADER_STDC

dnl Initialize intltool
GETTEXT_PACKAGE=ircp-tray
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", [GETTEXT package name])

IT_PROG_INTLTOOL([0.40.0])
AM_GLIB_GNU_GETTEXT

#### Check for GTK+ >= 2.14 ####
PKG_CHECK_MODULES(GTK, gtk+-2.0 >= 2.14,,
	AC_MSG_ERROR([GTK+-2.14 is required to compile ircp-tray]))

AC_SUBST(GTK_CFLAGS)
AC_SUBST(GTK_LIBS)

#### Check for openobex ####
if $(pkg-config --exists openobex); then
        PKG_CHECK_MODULES(OPENOBEX, [openobex])
        AC_SUBST(OPENOBEX_CFLAGS)
        AC_SUBST(OPENOBEX_LIBS)
fi

#### Check for libnotify ####
PKG_CHECK_MODULES(LIBNOTIFY, [libnotify],,
	AC_MSG_ERROR([libnotify is required to compile ircp-tray]))

AC_SUBST(LIBNOTIFY_CFLAGS)
AC_SUBST(LIBNOTIFY_LIBS)


AC_CONFIG_FILES([
Makefile
src/Makefile
data/Makefile
po/Makefile.in
])

AC_OUTPUT

echo
echo -e "Now type 'make' to compile ircp-tray,\nthen type 'sudo make install' to install it."
