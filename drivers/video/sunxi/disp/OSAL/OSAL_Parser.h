#ifndef  __OSAL_PARSER_H__
#define  __OSAL_PARSER_H__

#include "OSAL.h"

//#define  __OSAL_PARSER_MASK__

int OSAL_Script_FetchParser_Data(char *main_name, char *sub_name, int value[], int count);
/* returns: 0:invalid, 1: int; 2:str, 3: gpio */
int OSAL_Script_FetchParser_Data_Ex(char *main_name, char *sub_name, int value[], int count);

#endif
