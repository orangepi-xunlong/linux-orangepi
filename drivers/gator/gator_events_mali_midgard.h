/**
 * Copyright (C) ARM Limited 2013-2016. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef GATOR_EVENTS_MALI_MIDGARD_H_
#define GATOR_EVENTS_MALI_MIDGARD_H_

#define SYMBOL_GET(FUNCTION, ERROR_COUNT) \
    do { \
        if (FUNCTION ## _symbol) { \
            pr_err("gator: mali " #FUNCTION " symbol was already registered\n"); \
            (ERROR_COUNT)++; \
        } else { \
            FUNCTION ## _symbol = symbol_get(FUNCTION); \
            if (!FUNCTION ## _symbol) { \
                pr_err("gator: mali online " #FUNCTION " symbol not found\n"); \
                (ERROR_COUNT)++; \
            } \
        } \
    } while (0)

#define SYMBOL_CLEANUP(FUNCTION) \
    do { \
        if (FUNCTION ## _symbol) { \
            symbol_put(FUNCTION); \
            FUNCTION ## _symbol = NULL; \
        } \
    } while (0)


// We're using a single place where name for mali GPU architecture is defined.
extern const char* mali_name;

#endif /* GATOR_EVENTS_MALI_MIDGARD_H_ */
