#ifndef _CLI_H_
#define _CLI_H_

#define CONFIG_PRODUCT_STR      "BLE Ctrl Bench"
#define CLI_BUFFER_SIZE         80              // CLI Buffer size
#define CLI_ARG_SIZE            10               // The number of arguments
#define CLI_PROMPT              "ble> "

#include "types.h"
#include "../../hci_tools/lib/ssv_lib.h"

typedef void (*CliCmdFunc) ( s32, s8 ** );


typedef struct CLICmds_st
{
        const char         *Cmd;
        CliCmdFunc          CmdHandler;
        const char         *CmdUsage;

} CLICmds, *PCLICmds;

extern const CLICmds gCliCmdTable[];
extern const CLICmds gCliCmdTableTI[];
extern const CLICmds gCliCmdTableSSV[];
extern const CLICmds gCliCmdTableDTM[];

extern const CLICmds gCliCmdTableParameter[];
extern const CLICmds gCliCmdTableStable[];
extern const CLICmds gCliCmdTableStress[];
extern const CLICmds gCliCmdTablePerformance[];

extern const CLICmds gCliCmdTableSIG[];
extern const CLICmds gCliCmdTableProflie[];
extern const CLICmds gCliCmdTableFaulty[];
extern const CLICmds gCliCmdTableCFI[];
extern const CLICmds gCliCmdTableFunc[];

extern const CLICmds gCliCmdTableMulti[];


void cli_task(void);




#endif /* _CLI_H_ */
