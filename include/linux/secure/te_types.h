#ifndef __TE_TYPES_H__
#define __TE_TYPES_H__

/*
 * Return Codes
 */
enum {
	/* Success */
	TE_SUCCESS			= 0x00000000,
	TE_ERROR_NO_ERROR		= TE_SUCCESS,
	/* Non-specific cause */
	TE_ERROR_GENERIC		= 0xFFFF0000,
	/* Access priviledge not sufficient */
	TE_ERROR_ACCESS_DENIED		= 0xFFFF0001,
	/* The operation was cancelled */
	TE_ERROR_CANCEL		= 0xFFFF0002,
	/* Concurrent accesses conflict */
	TE_ERROR_ACCESS_CONFLICT	= 0xFFFF0003,
	/* Too much data for req was passed */
	TE_ERROR_EXCESS_DATA		= 0xFFFF0004,
	/* Input data was of invalid format */
	TE_ERROR_BAD_FORMAT		= 0xFFFF0005,
	/* Input parameters were invalid */
	TE_ERROR_BAD_PARAMETERS	= 0xFFFF0006,
	/* Oper invalid in current state */
	TE_ERROR_BAD_STATE		= 0xFFFF0007,
	/* The req data item not found */
	TE_ERROR_ITEM_NOT_FOUND	= 0xFFFF0008,
	/* The req oper not implemented */
	TE_ERROR_NOT_IMPLEMENTED	= 0xFFFF0009,
	/* The req oper not supported */
	TE_ERROR_NOT_SUPPORTED		= 0xFFFF000A,
	/* Expected data was missing */
	TE_ERROR_NO_DATA		= 0xFFFF000B,
	/* System ran out of resources */
	TE_ERROR_OUT_OF_MEMORY		= 0xFFFF000C,
	/* The system is busy */
	TE_ERROR_BUSY			= 0xFFFF000D,
	/* Communication failed */
	TE_ERROR_COMMUNICATION		= 0xFFFF000E,
	/* A security fault was detected */
	TE_ERROR_SECURITY		= 0xFFFF000F,
	/* The supplied buffer is too short */
	TE_ERROR_SHORT_BUFFER		= 0xFFFF0010,
};

/*
 * Return Code origins
 */
enum {
	/* Originated from OTE Client API */
	TE_RESULT_ORIGIN_API = 1,
	/* Originated from Underlying Communication Stack */
	TE_RESULT_ORIGIN_COMMS = 2,
	/* Originated from Common OTE Code */
	TE_RESULT_ORIGIN_KERNEL = 3,
	/* Originated from Trusted APP Code */
	TE_RESULT_ORIGIN_TRUSTED_APP = 4,
};

#endif
