icons = \
    apps_22x22_ircp-tray.png \
    apps_24x24_ircp-tray.png \
	apps_48x48_ircp-tray.png


service_icondir = $(datadir)/icons/hicolor/22x22/apps
service_icon_DATA = ircp-tray-active.png ircp-tray-progress1.png \
                    ircp-tray-progress4.png ircp-tray-inactive.png \
                    ircp-tray-progress2.png ircp-tray-progress3.png

@INTLTOOL_DESKTOP_RULE@
desktopdir = $(datadir)/applications
desktop_in_files = ircp-tray.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

EXTRA_DIST = \
	$(service_icon_DATA)    \
	$(desktop_DATA)

install-icons:
	for icon in $(icons); do \
		CONTEXT=`echo $$icon | cut -d_ -f1`; \
		SIZE=`echo $$icon | cut -d_ -f2`; \
		ICONFILE=`echo $$icon | cut -d_ -f3`; \
		mkdir -p $(DESTDIR)$(datadir)/icons/hicolor/$$SIZE/$$CONTEXT; \
		$(INSTALL_DATA) $(srcdir)/$$icon $(DESTDIR)$(datadir)/icons/hicolor/$$SIZE/$$CONTEXT/$$ICONFILE; \
	done;

uninstall-icons:
	-for icon in $(icons); do \
		CONTEXT=`echo $$icon | cut -d_ -f1`; \
		SIZE=`echo $$icon | cut -d_ -f2`; \
		ICONFILE=`echo $$icon | cut -d_ -f3`; \
		rm -f $(DESTDIR)$(datadir)/icons/hicolor/$$SIZE/$$CONTEXT/$$ICONFILE; \
	done;

install-data-local: install-icons
uninstall-local: uninstall-icons


