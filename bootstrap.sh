#! /bin/sh

autopoint
for file in po/*; do
  test $file = po/CVS && continue
  test -f halevt-mount/$file || cp $file halevt-mount/$file
done
autoreconf -i
