ACLOCAL_AMFLAGS = -I m4

SUBDIRS = po doc src scripts halevt-mount examples

confdir = $(pkgdatadir)

#dist_conf_DATA = $(PACKAGE).xml

EXTRA_DIST = config/config.rpath m4/ChangeLog  halevt.spec halevt.spec.in \
  halevt-initscript bootstrap.sh config/gitlog-to-changelog config/git-log-Changelog.sh
DISTCLEANFILES = halevt.spec

all-local: halevt.spec

halevt.spec: halevt.spec.in config.log
	sed -e 's/[@]PACKAGE_VERSION[@]/$(PACKAGE_VERSION)/' \
	    $< > halevt.spec

# install and uninstall directories for pid and system conf files
# if they are not empty leave them as is
install-data-local:
	$(MKDIR_P) "$(DESTDIR)$(sysconfdir)/$(PACKAGE)"
	$(MKDIR_P) "$(DESTDIR)$(HALEVT_PIDDIR)"

uninstall-local:
	-rmdir "$(DESTDIR)$(sysconfdir)/$(PACKAGE)"
	-rmdir "$(DESTDIR)$(HALEVT_PIDDIR)"

perms:
	if groupmod $(HALEVT_DEFAULT_GROUP) > /dev/null 2>&1; then : ; \
	else \
	   groupadd $(HALEVT_DEFAULT_GROUP) ; \
	fi
	if id -un $(HALEVT_DEFAULT_USER) > /dev/null 2>&1; then : ; \
	else \
	   useradd -c 'Halevt system user' -g $(HALEVT_DEFAULT_GROUP) -d "$(localstatedir)/lib/$(PACKAGE_NAME)" -s '/bin/false' $(HALEVT_DEFAULT_USER) ; \
	fi
	chown $(HALEVT_DEFAULT_USER) "$(DESTDIR)$(HALEVT_PIDDIR)"
	chgrp $(HALEVT_DEFAULT_GROUP) "$(DESTDIR)$(HALEVT_PIDDIR)"
	(cd scripts && $(MAKE) $(AM_MAKEFLAGS) perms)
