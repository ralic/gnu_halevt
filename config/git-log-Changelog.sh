#! /bin/sh

#set -x
./config/gitlog-to-changelog -- 'HEAD~1..HEAD' > ChangeLog-git.new
echo >> ChangeLog-git.new
cp ChangeLog ChangeLog-old.old || exit 1
cat ChangeLog-git.new ChangeLog-old.old > ChangeLog || exit 1
rm ChangeLog-old.old ChangeLog-git.new
git reset --soft HEAD^
git add ChangeLog
git commit -a -c ORIG_HEAD
#git commit -n --cleanup=verbatim -m '' ChangeLog
# in case there is uncommited changes
# doesn't work... stash with and without keep-index reverts the ChangeLog...
#git stash save 
# without interactive rebase the empty commit message is left...
#git rebase -i HEAD~2
#git stash pop
