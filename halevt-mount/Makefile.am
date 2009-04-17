SUBDIRS = po

bin_PROGRAMS = halevt-mount

halevt_mount_SOURCES = halevt-mount.c
halevt_mount_CPPFLAGS = -DLOCALSTATEDIR="\"$(localstatedir)\"" \
 -DHALEVT_USER_NAME="\"$(HALEVT_USER_NAME)\"" \
  -DDATADIR="\"$(datadir)\"" \
  $(HAL_CFLAGS) $(DBUS_CFLAGS)
halevt_mount_LDADD = $(HAL_LIBS) $(DBUS_LIBS)

# halevt-umount is a symlink to halevt-mount. We ignore if it fails, because
# the link may already be there

all-local:
	-ln -s halevt-mount halevt-umount

install-exec-local:
	-f=`echo "halevt-mount" | sed 's|^.*/||;$(transform)'`; \
	  t=`echo "halevt-umount" | sed 's|^.*/||;$(transform)'`; \
	  ln -s "$$f" "$(DESTDIR)$(bindir)/$$t"

# data directory for halevt-mount
install-data-local:
	$(MKDIR_P) "$(DESTDIR)$(localstatedir)/lib/$(PACKAGE_NAME)"

uninstall-local:
	t=`echo "halevt-umount" | sed 's|^.*/||;$(transform)'`; \
	  rm -f "$(DESTDIR)$(bindir)/$$t"
	-rmdir "$(DESTDIR)$(localstatedir)/lib/$(PACKAGE_NAME)"

perms:
	chown $(HALEVT_USER_NAME) "$(DESTDIR)$(localstatedir)/lib/$(PACKAGE_NAME)"
	chgrp $(HALEVT_GROUP_NAME) "$(DESTDIR)$(localstatedir)/lib/$(PACKAGE_NAME)"

CLEANFILES = halevt-umount
