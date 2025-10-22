#!/bin/sh
########################################################################### ###
#@File
#@Title         Print the linker version of ld.lld and clang version.
#@Copyright     Copyright (c) Imagination Technologies Ltd. All Rights Reserved
#@License       Dual MIT/GPLv2
#
# The contents of this file are subject to the MIT license as set out below.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# Alternatively, the contents of this file may be used under the terms of
# the GNU General Public License Version 2 ("GPL") in which case the provisions
# of GPL are applicable instead of those above.
#
# If you wish to allow use of your version of this file only under the terms of
# GPL, and not to allow others to use your version of this file under the terms
# of the MIT license, indicate your decision by deleting the provisions above
# and replace them with the notice and other provisions required by GPL as set
# out in the file called "GPL-COPYING" included in this distribution. If you do
# not delete the provisions above, a recipient may use your version of this file
# under the terms of either the MIT license or GPL.
#
# This License is also included in this distribution in the file called
# "MIT-COPYING".
#
# EXCEPT AS OTHERWISE STATED IN A NEGOTIATED AGREEMENT: (A) THE SOFTWARE IS
# PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
# BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
# PURPOSE AND NONINFRINGEMENT; AND (B) IN NO EVENT SHALL THE AUTHORS OR
# COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
# IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
# CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
### ###########################################################################

LANG=C
export LANG

# usage: clang-ld-version.sh [--clang] [--ld.lld]

# Convert the version string x.y.z to a 5 or 6-digits.
# For example,
# $ ld.lld --version
# $ LLD 14.0.6 (compatible with GNU linkers)
# will give 140006.
get_canonical_version() {
	IFS=.
	set -- $1

	# If fields are missing, fill them with zeros.
	echo $((10000 * $1 + 100 * ${2:-0} + ${3:-0}))
}

get_version() {
	# Output of --version.
	IFS=''
	if [ $CLANG -eq 1 ]; then
		set -- $(clang --version)
	elif [ $LLD -eq 1 ]; then
		set -- $(ld.lld --version)
	else
		exit 1
	fi
	# Split the line on spaces.
	IFS=' '
	set -- $1

	while [ $# -gt 1 -a "$1" != "$prefix" ]; do
		shift
	done
	if [ "$1" = "$prefix" ]; then
		echo $(get_canonical_version ${2%-*})
	else
		exit 1
	fi
}

if [ $# -ne 1 ]; then
	exit 1
fi

CLANG=0
LLD=0

# 'prefix' represents the word before the version number.
if [ "$1" = "--clang" ]; then
	# Example: Android (8490178, based on r450784d) clang version 14.0.6
	prefix="version"
	CLANG=1
elif [ "$1" = "--ld.lld" ]; then
	#Example: LLD 14.0.6 (compatible with GNU linkers)
	prefix="LLD"
	LLD=1
else
	exit 1
fi

echo $(get_version $1)
