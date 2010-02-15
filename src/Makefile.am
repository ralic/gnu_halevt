bin_PROGRAMS = halevt

halevt_SOURCES = common.h parse_config.c parse_config.h \
 debug.c match.c match.h hal_interface.h hal_interface.c \
 manager.c devices_list.c devices_list.h
nodist_halevt_SOURCES = boolstuff_dummy.cpp
halevt_CPPFLAGS = -DDATADIR="\"$(datadir)\"" -DSYSCONFDIR="\"$(sysconfdir)\"" \
  $(LIBXML_CFLAGS) $(BOOLSTUFF_CFLAGS) \
  $(HAL_CFLAGS) $(DBUS_GLIB_CFLAGS)
halevt_LDADD = $(LIBXML_LIBS) $(BOOLSTUFF_LIBS) \
  $(HAL_LIBS) $(DBUS_GLIB_LIBS)

boolstuff_dummy.cpp:
	echo 'static void boolstuff_dummy();' > $@

CLEANFILES = boolstuff_dummy.cpp
