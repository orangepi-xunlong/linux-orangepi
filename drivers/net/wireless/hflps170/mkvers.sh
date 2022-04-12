#!/bin/bash
#
# Outputs svn revision of directory $VERDIR into $TARGET if it changes $TARGET
# example output omitting enclosing quotes:
#
# '#define BL_VERS_REV "svn1676M"'
# '#define BL_VERS_MOD "vX.X.X.X"'
# '#define BL_VERS_OTH "build: username date hour"'

set -e

VERDIR=$(dirname $(readlink -f $0))
TARGET=$1

DATE_FORMAT="%b %d %Y %T"
BL_VERS_MOD=$(grep BL_VERS_NUM $VERDIR/Makefile | cut -f2 -d=)

tmpout=$TARGET.tmp
cd $VERDIR

if [ -e ".svn" ]
then
    svnrev=$(svnversion -c | sed 's/.*://')
elif (git status -uno > /dev/null)
then
    # maybe git-svn
    idx=0
    while [ "$svnrev" = "" ]
    do
	# loop on all commit to find the first one that match a svn revision
        # svnrev=$(git svn find-rev HEAD~$idx)
        # we not use svn,we use git
        svnrev=$(git rev-parse HEAD~$idx)
	if [ $? -ne 0 ]
	then
	    svnrev=": Unknown Revision"
	elif [ "$svnrev" = "" ]
	then
	    idx=$((idx + 1))
	elif [ $idx -gt 0 ] || (git status -uno --porcelain | grep -q '^ M')
	then
	    # If this is not the HEAD, or working copy is not clean then we're
	    # not at a commited svn revision so add 'M'
	    svnrev=$svnrev"M"
	fi
    done

    # append git info (sha1 and branch name)
    git_sha1=$(git rev-parse --short HEAD)
    git_branch=$(git symbolic-ref --short HEAD 2>/dev/null || echo "detached")
    svnrev="$svnrev ($git_sha1/$git_branch)"
else
    svnrev=": Unknown Revision"
fi

date=$(LC_TIME=C date +"$DATE_FORMAT")

BL_VERS_REV="git$svnrev"
#      "lmac vX.X.X.X - build:"
banner="bl v$BL_VERS_MOD - build: $(whoami) $date - $BL_VERS_REV"

define() { echo "#define $1 \"$2\""; }
{
	define "BL_VERS_REV"    "$BL_VERS_REV"
	define "BL_VERS_MOD"    "$BL_VERS_MOD"
	define "BL_VERS_BANNER" "$banner"
} > $tmpout




cmp -s $TARGET $tmpout && rm -f $tmpout || mv $tmpout $TARGET
