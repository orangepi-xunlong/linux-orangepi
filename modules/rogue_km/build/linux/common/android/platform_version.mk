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

PLATFORM_CODENAME := REL
PLATFORM_RELEASE := 4.4.2

PLATFORM_RELEASE_MAJ   := $(shell echo $(PLATFORM_RELEASE) | cut -f1 -d'.')
PLATFORM_RELEASE_MIN   := $(shell echo $(PLATFORM_RELEASE) | cut -f2 -d'.')
PLATFORM_RELEASE_PATCH := $(shell echo $(PLATFORM_RELEASE) | cut -f3 -d'.')

# Not all versions have a patchlevel; fix that up here
#
ifeq ($(PLATFORM_RELEASE_PATCH),)
PLATFORM_RELEASE_PATCH := 0
endif

# Macros to help categorize support for features and API_LEVEL for tests.
#
is_at_least_jellybean_mr1 := \
	$(shell ( test $(PLATFORM_RELEASE_MAJ) -gt 4 || \
				( test $(PLATFORM_RELEASE_MAJ) -eq 4 && \
				  test $(PLATFORM_RELEASE_MIN) -ge 2 ) ) && echo 1 || echo 0)
is_at_least_jellybean_mr2 := \
	$(shell ( test $(PLATFORM_RELEASE_MAJ) -gt 4 || \
				( test $(PLATFORM_RELEASE_MAJ) -eq 4 && \
				  test $(PLATFORM_RELEASE_MIN) -ge 3 ) ) && echo 1 || echo 0)
is_at_least_kitkat := \
	$(shell ( test $(PLATFORM_RELEASE_MAJ) -gt 4 || \
				( test $(PLATFORM_RELEASE_MAJ) -eq 4 && \
				  test $(PLATFORM_RELEASE_MIN) -ge 4 ) ) && echo 1 || echo 0)
is_at_least_kitkat_mr1 := \
	$(shell ( test $(PLATFORM_RELEASE_MAJ) -gt 4 || \
				( test $(PLATFORM_RELEASE_MAJ) -eq 4 && \
				  test $(PLATFORM_RELEASE_MIN) -gt 4 ) || \
					( test $(PLATFORM_RELEASE_MAJ) -eq 4 && \
					  test $(PLATFORM_RELEASE_MIN) -eq 4 && \
					  test $(PLATFORM_RELEASE_PATCH) -ge 1 ) ) && echo 1 || echo 0)

# Assume "future versions" are >=5.0, but we don't really know
is_future_version := \
	$(shell ( test $(PLATFORM_RELEASE_MAJ) -ge 5 ) && echo 1 || echo 0)

# Sometimes a feature is introduced in AOSP master that isn't on the current
# PDK branch, but both versions are beyond our support level. This variable
# can be used to differentiate those builds.
#
ifeq ($(PLATFORM_CODENAME)$(is_future_version),AOSP1)
is_aosp_master := 1
else
is_aosp_master := 0
endif

# Picking an exact match of API_LEVEL for the platform we're building
# against can avoid compatibility theming and affords better integration.
#
ifeq ($(is_future_version),1)
# Temporarily pin to 19 until it is actually bumped to 20
API_LEVEL := 19
else ifeq ($(is_at_least_kitkat),1)
API_LEVEL := 19
else ifeq ($(is_at_least_jellybean_mr2),1)
API_LEVEL := 18
else ifeq ($(is_at_least_jellybean_mr1),1)
API_LEVEL := 17
else
$(error Must build against Android >= 4.2)
endif

# Each DDK is tested against only a single version of the platform.
# Warn if a different platform version is used.
#
ifeq ($(is_future_version),1)
$(info WARNING: Android version is newer than this DDK supports)
else ifneq ($(is_at_least_jellybean_mr2),1)
$(info WARNING: Android version is older than this DDK supports)
endif
