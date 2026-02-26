#ifndef _CLI_CMD_MASTER_H_
#define _CLI_CMD_MASTER_H_

#include "cli/cli.h"

void Cmd_Master_Conn            (s32 argc, s8 *argv[]);
void Cmd_Master_Disconn         (s32 argc, s8 *argv[]);
void Cmd_Master_Conn_Update     (s32 argc, s8 *argv[]);
void Cmd_Master_Feature_Exchange(s32 argc, s8 *argv[]);
void Cmd_hcicmd_reset			(s32 argc, s8 *argv[]);
void Cmd_lecc					(s32 argc, s8 *argv[]);
void Cmd_ledc					(s32 argc, s8 *argv[]);
void Cmd_lecan					(s32 argc, s8 *argv[]);
void Cmd_acl					(s32 argc, s8 *argv[]);
void Cmd_acl_fragment			(s32 argc, s8 *argv[]);
void Cmd_dut_reset				(s32 argc, s8 *argv[]);

#endif // _CLI_CMD_MASTER_H_
