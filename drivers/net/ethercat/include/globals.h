/******************************************************************************
 *
 *  Copyright (C) 2006-2021  Florian Pose, Ingenieurgemeinschaft IgH
 *
 *  This file is part of the IgH EtherCAT master.
 *
 *  The file is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU Lesser General Public License as published by the
 *  Free Software Foundation; version 2.1 of the License.
 *
 *  This file is distributed in the hope that it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
 *  License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this file. If not, see <http://www.gnu.org/licenses/>.
 *
 *****************************************************************************/

/**
   \file
   Global definitions and macros.
*/

/*****************************************************************************/

#ifndef __EC_GLOBALS_H__
#define __EC_GLOBALS_H__

#include "config.h"

/******************************************************************************
 *  Overall macros
 *****************************************************************************/

/** Helper macro for EC_STR(), literates a macro argument.
 *
 * \param X argument to literate.
 */
#define EC_LIT(X) #X

/** Converts a macro argument to a string.
 *
 * \param X argument to stringify.
 */
#define EC_STR(X) EC_LIT(X)

/** Master version string
 */
#define EC_MASTER_VERSION VERSION " " EC_STR(REV)

/*****************************************************************************/

#endif
