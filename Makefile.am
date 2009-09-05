ACLOCAL_AMFLAGS = -I m4

SUBDIRS = po doc src scripts halevt-mount examples

confdir = $(pkgdatadir)

#dist_conf_DATA = $(PACKAGE).xml

EXTRA_DIST = config/config.rpath m4/ChangeLog  halevt.spec halevt.spec.in \
  halevt-initscript bootstrap.sh
DISTCLEANFILES = halevt.spec

all-local: halevt.spec

halevt.spec: halevt.spec.in config.log
	sed -e 's/[@]PACKAGE_VERSION[@]/$(PACKAGE_VERSION)/' \
	    $< > halevt.spec

# install and uninstall directories for pid and system conf files
# if they are not empty leave them as is
install-data-local:
	$(MKDIR_P) "$(DESTDIR)$(sysconfdir)/$(PACKAGE)"
	$(MKDIR_P) "$(DESTDIR)$(halevt_piddir)"

uninstall-local:
	-rmdir "$(DESTDIR)$(sysconfdir)/$(PACKAGE)"
	-rmdir "$(DESTDIR)$(halevt_piddir)"

perms:
	if groupmod $(HALEVT_GROUP_NAME) > /dev/null 2>&1; then : ; \
	else \
	   groupadd $(HALEVT_GROUP_NAME) ; \
	fi
	if id -un $(HALEVT_USER_NAME) > /dev/null 2>&1; then : ; \
	else \
	   useradd -c 'Halevt system user' -g $(HALEVT_GROUP_NAME) -d "$(localstatedir)/lib/$(PACKAGE_NAME)" -s '/bin/false' $(HALEVT_USER_NAME) ; \
	fi
	chown $(HALEVT_USER_NAME) "$(DESTDIR)$(halevt_piddir)"
	chgrp $(HALEVT_GROUP_NAME) "$(DESTDIR)$(halevt_piddir)"
	(cd scripts && $(MAKE) $(AM_MAKEFLAGS) perms)
