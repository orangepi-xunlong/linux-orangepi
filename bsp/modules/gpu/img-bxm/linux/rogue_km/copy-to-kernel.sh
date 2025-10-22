#!/bin/bash
########################################################################### ###
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

function usage()
{
  echo "$0 <config_dir> <kernel_dir>"
  echo "  Copy source files and configuration into a kernel tree."
  echo "  The configuration and list of files to copy is found in <config_dir>."
  echo "  The target kernel tree is <kernel_dir>."
  echo "  Before running this script, we recommend that you clean out the old"
  echo "  destination directories in <kernel_dir>."
}

if [ "$#" -lt 2 ]; then
  echo "Not enough arguments"
  usage
  exit 1
fi

CONFIG=$1
DEST=$2

if [ ! -f "$CONFIG/copy_items.sh" ]; then
  echo "$CONFIG does not look like a config directory. copy_items.sh is missing."
  usage
  exit 1
fi

if [ ! -f "$DEST/Kconfig" ] ; then
  echo "$DEST does not look like a kernel directory."
  usage
  exit 1
fi

function copyfile()
{
  src=$1
  dest="$DEST/$2"

  mkdir -p `dirname $dest`
  echo copy $src to $dest
  cp $src $dest
  chmod u+w $dest
}

source "$CONFIG/copy_items.sh"
