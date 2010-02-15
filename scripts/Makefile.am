bin_SCRIPTS = hvmount

EXTRA_DIST = hvmount.in

hvmount: hvmount.in ../config.log
	sed -e 's:[@]HALEVT_DEFAULT_USER[@]:$(HALEVT_DEFAULT_GROUP):' \
	    -e 's:[@]PACKAGE_NAME[@]:$(PACKAGE_NAME):' \
	    -e 's:[@]localstatedir[@]:$(localstatedir):' \
	    $< > hvmount
	chmod a+x hvmount

# hvumount is a symlink to hvmount. We ignore if it fails, because
# the link may already be there
install-exec-local:
	-f=`echo "hvmount" | sed 's|^.*/||;$(transform)'`; \
	  t=`echo "hvumount" | sed 's|^.*/||;$(transform)'`; \
	  $(LN_S) "$$f" "$(DESTDIR)$(bindir)/$$t"

# data directory for hvmount
install-data-local:
	$(MKDIR_P) "$(DESTDIR)$(localstatedir)/lib/$(PACKAGE_NAME)"

uninstall-local:
	t=`echo "hvumount" | sed 's|^.*/||;$(transform)'`; \
	  $(RM) "$(DESTDIR)$(bindir)/$$t"
	-rmdir "$(DESTDIR)$(localstatedir)/lib/$(PACKAGE_NAME)"

perms:
	chown $(HALEVT_DEFAULT_USER) "$(DESTDIR)$(localstatedir)/lib/$(PACKAGE_NAME)"
	chgrp $(HALEVT_DEFAULT_GROUP) "$(DESTDIR)$(localstatedir)/lib/$(PACKAGE_NAME)"

CLEANFILES = hvmount hvumount
