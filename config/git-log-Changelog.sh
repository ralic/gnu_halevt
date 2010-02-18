#! /bin/sh

./config/gitlog-to-changelog -- '@{1}..' > ChangeLog-git.new
echo >> ChangeLog-git.new
cp ChangeLog ChangeLog-old.old || exit 1
cat ChangeLog-git.new ChangeLog-old.old > ChangeLog || exit 1
rm ChangeLog-old.old ChangeLog-git.new
git commit -n --cleanup=verbatim -m '' ChangeLog
# in case there is uncommited changes
git stash save --keep-index
git rebase -i HEAD~2
git stash pop
