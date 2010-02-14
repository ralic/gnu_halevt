#! /bin/sh

autopoint
for file in po/*; do
  test $file = po/CVS && continue
  # certainly not useful, the .git seems to be only in the toplevel directory
  test $file = po/.git && continue
  test -f halevt-mount/$file || cp $file halevt-mount/$file
done
autoreconf -i
