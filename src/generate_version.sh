#!/usr/bin/env bash
echo "#include <stdio.h>" > version.c
echo -n "void print_version() { const char *msg = \"built from commit:\n" >> version.c


echo -n `git log --pretty=format:'%ai' -n 1` >> version.c
echo -n '\n' >> version.c
echo -n `git log --pretty=format:'%h %B' -n 1` >> version.c
echo -n '\n' >> version.c

git diff --exit-code > /dev/null
unstaged=$?
git diff --cached --exit-code > /dev/null
staged=$?
if [ "$unstaged" = 1 ] || [ "$staged" = 1 ] ; then
  echo -n "with further chages\n" >> version.c
fi
echo "\"; puts(msg); }" >> version.c