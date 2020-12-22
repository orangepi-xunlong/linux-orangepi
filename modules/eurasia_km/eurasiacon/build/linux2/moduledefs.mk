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

MODULE_TARGETS :=
MODULE_CFLAGS := $(ALL_CFLAGS) $($(THIS_MODULE)_cflags)
MODULE_CXXFLAGS := $(ALL_CXXFLAGS) $($(THIS_MODULE)_cxxflags)
MODULE_HOST_CFLAGS := $(ALL_HOST_CFLAGS) $($(THIS_MODULE)_cflags)
MODULE_HOST_CXXFLAGS := $(ALL_HOST_CXXFLAGS) $($(THIS_MODULE)_cxxflags)
MODULE_LDFLAGS := $(ALL_LDFLAGS) $($(THIS_MODULE)_ldflags)
MODULE_HOST_LDFLAGS := $(ALL_HOST_LDFLAGS) $($(THIS_MODULE)_ldflags)
MODULE_BISON_FLAGS := $(ALL_BISON_FLAGS) $($(THIS_MODULE)_bisonflags)
MODULE_FLEX_FLAGS := $(ALL_FLEX_FLAGS) $($(THIS_MODULE)_flexflags)

ifneq ($(BUILD),debug)
ifeq ($(USE_LTO),1)
MODULE_HOST_LDFLAGS := \
 $(sort $(filter-out -W% -D%,$(ALL_HOST_CFLAGS) $(ALL_HOST_CXXFLAGS))) \
 $(MODULE_HOST_LDFLAGS)
MODULE_LDFLAGS := \
 $(sort $(filter-out -W% -D%,$(ALL_CFLAGS) $(ALL_CXXFLAGS))) \
 $(MODULE_LDFLAGS)
endif
endif

# Only allow cflags that do not affect code generation. This is to ensure
# proper binary compatibility when LTO (Link-Time Optimization) is enabled.
# We make exceptions for -fPIC, -fPIE and -m32 which will all fail linkage
# in non-LTO mode if incorrectly specified.
#
# NOTE: Only used by static_library and objects right now. Other module
# types should not be affected by complex code generation flags w/ LTO.
MODULE_ALLOWED_CFLAGS := -W% -D% -std=% -fPIC -fPIE -m32

# -L flags for library search dirs: these are relative to $(TOP), unless
# they're absolute paths
MODULE_LIBRARY_DIR_FLAGS := $(foreach _path,$($(THIS_MODULE)_libpaths),$(if $(filter /%,$(_path)),-L$(call relative-to-top,$(_path)),-L$(_path)))
# -I flags for header search dirs (same rules as for -L)
MODULE_INCLUDE_FLAGS := $(foreach _path,$($(THIS_MODULE)_includes),$(if $(filter /%,$(_path)),-I$(call relative-to-top,$(_path)),-I$(_path)))

# Variables used to differentiate between host/target builds
MODULE_OUT := $(if $(MODULE_HOST_BUILD),$(HOST_OUT),$(TARGET_OUT))
# For documentation modules, this variable is overridden by the module type
# makefile to place the intermediates in $(DOCS_OUT)/intermediates.
MODULE_INTERMEDIATES_DIR := $(if $(MODULE_HOST_BUILD),$(HOST_INTERMEDIATES)/$(THIS_MODULE),$(TARGET_INTERMEDIATES)/$(THIS_MODULE))

.SECONDARY: $(MODULE_INTERMEDIATES_DIR)
$(MODULE_INTERMEDIATES_DIR):
	$(make-directory)

# These are used for messages and variable names where we need to say "host"
# or "target" according to the module build type.
Host_or_target := $(if $(MODULE_HOST_BUILD),Host,Target)
host_or_target := $(if $(MODULE_HOST_BUILD),host,target)
HOST_OR_TARGET := $(if $(MODULE_HOST_BUILD),HOST,TARGET)

# These define the rules for finding source files.
# - If a name begins with a slash, we strip $(TOP) off the front if it begins
#   with $(TOP). This is so that we don't get really long error messages from
#   the compiler if the source tree is in a deeply nested directory, but we
#   still do get absolute paths if you say "make OUT=/tmp/somewhere"
# - Otherwise, if a name contains a slash and begins with $(OUT), we leave it
#   as it is. This is so you can say "module_src :=
#   $(TARGET_INTERMEDIATES)/something/generated.c"
# - Otherwise, we assume it's a path referring to somewhere under the
#   directory containing Linux.mk, and add $(THIS_DIR) to it
_SOURCES_WITHOUT_SLASH := $(strip $(foreach _s,$($(THIS_MODULE)_src),$(if $(findstring /,$(_s)),,$(_s))))
_SOURCES_WITH_SLASH := $(strip $(foreach _s,$($(THIS_MODULE)_src),$(if $(findstring /,$(_s)),$(_s),)))
MODULE_SOURCES := $(addprefix $(THIS_DIR)/,$(_SOURCES_WITHOUT_SLASH))
MODULE_SOURCES += $(call relative-to-top,$(filter /%,$(_SOURCES_WITH_SLASH)))

_RELATIVE_SOURCES_WITH_SLASH := $(filter-out /%,$(_SOURCES_WITH_SLASH))
_OUTDIR_RELATIVE_SOURCES_WITH_SLASH := $(filter $(RELATIVE_OUT)/%,$(_RELATIVE_SOURCES_WITH_SLASH))
_THISDIR_RELATIVE_SOURCES_WITH_SLASH := $(filter-out $(RELATIVE_OUT)/%,$(_RELATIVE_SOURCES_WITH_SLASH))
MODULE_SOURCES += $(_OUTDIR_RELATIVE_SOURCES_WITH_SLASH)
MODULE_SOURCES += $(addprefix $(THIS_DIR)/,$(_THISDIR_RELATIVE_SOURCES_WITH_SLASH))

# Generated sources and headers. We use $(MODULE_OUT) because it encourages
# correctly marking modules which generate headers as host/target.
MODULE_SOURCES += $(addprefix $(MODULE_OUT)/intermediates/,$($(THIS_MODULE)_gensrc))
MODULE_GENERATED_HEADERS := $(addprefix $(MODULE_OUT)/intermediates/,$($(THIS_MODULE)_genheaders))

# -l flags for each library. The rules are:
#  - for all static libs, use -lfoo
#  - for all in-tree or external libs, use $(libfoo_ldflags) if that
#    variable is defined (empty counts as defined). Otherwise use
#    -lfoo.
MODULE_LIBRARY_FLAGS := $(addprefix -l, $($(THIS_MODULE)_staticlibs)) $(addprefix -l,$($(THIS_MODULE)_libs)) $(foreach _lib,$($(THIS_MODULE)_extlibs),$(if $(filter undefined,$(origin lib$(_lib)_ldflags)),-l$(_lib),$(lib$(_lib)_ldflags)))

# pkg-config integration; primarily used by X.Org
# FIXME: We don't support arbitrary CFLAGS yet (just includes)
$(foreach _package,$($(THIS_MODULE)_packages),\
 $(eval MODULE_INCLUDE_FLAGS     += `pkg-config --cflags-only-I $(_package)`)\
 $(eval MODULE_LIBRARY_FLAGS     += `pkg-config --libs-only-l $(_package)`)\
 $(eval MODULE_LIBRARY_DIR_FLAGS += `pkg-config --libs-only-L $(_package)`))
