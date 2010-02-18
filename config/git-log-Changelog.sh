#! /bin/sh

#set -x
./config/gitlog-to-changelog -- 'HEAD~1..HEAD' > ChangeLog-git.new
echo >> ChangeLog-git.new
cp ChangeLog ChangeLog-old.old || exit 1
cat ChangeLog-git.new ChangeLog-old.old > ChangeLog || exit 1
rm ChangeLog-old.old ChangeLog-git.new
git reset --soft HEAD^
git add ChangeLog
git commit -C ORIG_HEAD
