#include <stdio.h>
#include <string.h>

#include "types.h"
#include "cli.h"


/* Command Line: */
static s8   sg_cmd_buff[CLI_BUFFER_SIZE+1];
static s8*  sg_argv[CLI_ARG_SIZE];
static u32  sg_argc;
static u32  sg_curpos = 0;

static bool cli_init_flag=false;
bool cli_exit_flag = false;

static void cli_cmd_usage( void )
{
    const CLICmds *cmd_ptr;

    printf("\n\nUsage:\n");
    for( cmd_ptr=gCliCmdTable; cmd_ptr->Cmd; cmd_ptr ++ )
    {
        printf("%-20s\t\t%s\n", cmd_ptr->Cmd, cmd_ptr->CmdUsage);
    }


    for( cmd_ptr=gCliCmdTableTI; cmd_ptr->Cmd; cmd_ptr ++ )
    {
        printf("%-20s\t\t%s\n", cmd_ptr->Cmd, cmd_ptr->CmdUsage);
    }

    for( cmd_ptr=gCliCmdTableSSV; cmd_ptr->Cmd; cmd_ptr ++ )
    {
        printf("%-20s\t\t%s\n", cmd_ptr->Cmd, cmd_ptr->CmdUsage);
    }


    for( cmd_ptr=gCliCmdTableDTM; cmd_ptr->Cmd; cmd_ptr ++ )
    {
        printf("%-20s\t\t%s\n", cmd_ptr->Cmd, cmd_ptr->CmdUsage);
    }


    for( cmd_ptr=gCliCmdTableParameter; cmd_ptr->Cmd; cmd_ptr ++ )
    {
        printf("%-20s\t\t%s\n", cmd_ptr->Cmd, cmd_ptr->CmdUsage);
    }

    for( cmd_ptr=gCliCmdTableStable; cmd_ptr->Cmd; cmd_ptr ++ )
    {
        printf("%-20s\t\t%s\n", cmd_ptr->Cmd, cmd_ptr->CmdUsage);
    }

    for( cmd_ptr=gCliCmdTableStress; cmd_ptr->Cmd; cmd_ptr ++ )
    {
        printf("%-20s\t\t%s\n", cmd_ptr->Cmd, cmd_ptr->CmdUsage);
    }


    for( cmd_ptr=gCliCmdTablePerformance; cmd_ptr->Cmd; cmd_ptr ++ )
    {
        printf("%-20s\t\t%s\n", cmd_ptr->Cmd, cmd_ptr->CmdUsage);
    }


    for( cmd_ptr=gCliCmdTableSIG; cmd_ptr->Cmd; cmd_ptr ++ )
    {
        printf("%-20s\t\t%s\n", cmd_ptr->Cmd, cmd_ptr->CmdUsage);
    }

    for( cmd_ptr=gCliCmdTableProflie; cmd_ptr->Cmd; cmd_ptr ++ )
    {
        printf("%-20s\t\t%s\n", cmd_ptr->Cmd, cmd_ptr->CmdUsage);
    }

    for( cmd_ptr=gCliCmdTableFaulty; cmd_ptr->Cmd; cmd_ptr ++ )
    {
        printf("%-20s\t\t%s\n", cmd_ptr->Cmd, cmd_ptr->CmdUsage);
    }

    for( cmd_ptr=gCliCmdTableFunc; cmd_ptr->Cmd; cmd_ptr ++ )
    {
        printf("%-20s\t\t%s\n", cmd_ptr->Cmd, cmd_ptr->CmdUsage);
    }

    for( cmd_ptr=gCliCmdTableMulti; cmd_ptr->Cmd; cmd_ptr ++ )
    {
        printf("%-20s\t\t%s\n", cmd_ptr->Cmd, cmd_ptr->CmdUsage);
    }
    printf("\n%s", CLI_PROMPT);
}


void cli_init(void)
{
    printf("\n\n********************************\n"
           "  %s %s %s\n"
           "********************************\n",
    CONFIG_PRODUCT_STR, __DATE__, __TIME__);
    printf("\n<<Start Command Line>>\n\n\tPress \'?\'  for help......\n\n\n");
    printf("\n%s", CLI_PROMPT);
    fflush(stdout);

    memset( (void *)sg_argv, 0, sizeof(u8 *)*5 );
    sg_cmd_buff[0] = 0x00;
    sg_curpos = 0;
    sg_argc = 0;
    cli_init_flag = true;
}


void cli_task(void)
{
    const CLICmds *CmdPtr;
    u8 ch;
    s8 *pch;

    if (cli_init_flag == false)
        cli_init();

    /*
    if ( !kbhit() )
    {
        return;
    }
    */

    switch ( (ch=getchar()) )
    {
        case 0x00: /* Special Key, read again for real data */
            ch = getchar();
            break;

        case 0x08: /* Backspace */
            if ( 0 < sg_curpos )
            {
                putchar(0x08);
                putchar(0x20);
                putchar(0x08);
                fflush(stdout);
                sg_curpos --;
                sg_cmd_buff[sg_curpos] = 0x00;
            }
            break;

#ifdef __linux__
        case 0x0a: /* Enter */
#else
        case 0x0d: /* Enter */
#endif
            for( sg_argc=0,ch=0, pch=sg_cmd_buff; (*pch!=0x00)&&(sg_argc<CLI_ARG_SIZE); pch++ )
            {
                if ( (ch==0) && (*pch!=' ') )
                {
                    ch = 1;
                    sg_argv[sg_argc] = pch;
                }

                if ( (ch==1) && (*pch==' ') )
                {
                    *pch = 0x00;
                    ch = 0;
                    sg_argc ++;
                }
            }
            if ( ch == 1)
            {
                sg_argc ++;
            }
            else if ( sg_argc > 0 )
            {
                *(pch-1) = ' ';
            }

            if ( sg_argc > 0 )
            {
                /* Dispatch command */
                for( CmdPtr=gCliCmdTable; CmdPtr->Cmd; CmdPtr ++ )
                {
                    if ( !strcmp(sg_argv[0], CmdPtr->Cmd) )
                    {
                        printf("\n");
                        fflush(stdout);
                        CmdPtr->CmdHandler( sg_argc, sg_argv );
                        break;
                    }
                }

                for( CmdPtr=gCliCmdTableTI; CmdPtr->Cmd; CmdPtr ++ )
                {
                    if ( !strcmp(sg_argv[0], CmdPtr->Cmd) )
                    {
                        printf("\n");
                        fflush(stdout);
                        CmdPtr->CmdHandler( sg_argc, sg_argv );
                        break;
                    }
                }

                for( CmdPtr=gCliCmdTableSSV; CmdPtr->Cmd; CmdPtr ++ )
                {
                    if ( !strcmp(sg_argv[0], CmdPtr->Cmd) )
                    {
                        printf("\n");
                        fflush(stdout);
                        CmdPtr->CmdHandler( sg_argc, sg_argv );
                        break;
                    }
                }

                for( CmdPtr=gCliCmdTableDTM; CmdPtr->Cmd; CmdPtr ++ )
                {
                    if ( !strcmp(sg_argv[0], CmdPtr->Cmd) )
                    {
                        printf("\n");
                        fflush(stdout);
                        CmdPtr->CmdHandler( sg_argc, sg_argv );
                        break;
                    }
                }


                for( CmdPtr=gCliCmdTableParameter; CmdPtr->Cmd; CmdPtr ++ )
                {
                    if ( !strcmp(sg_argv[0], CmdPtr->Cmd) )
                    {
                        printf("\n");
                        fflush(stdout);
                        CmdPtr->CmdHandler( sg_argc, sg_argv );
                        break;
                    }
                }

                for( CmdPtr=gCliCmdTableStable; CmdPtr->Cmd; CmdPtr ++ )
                {
                    if ( !strcmp(sg_argv[0], CmdPtr->Cmd) )
                    {
                        printf("\n");
                        fflush(stdout);
                        CmdPtr->CmdHandler( sg_argc, sg_argv );
                        break;
                    }
                }

                for( CmdPtr=gCliCmdTableStress; CmdPtr->Cmd; CmdPtr ++ )
                {
                    if ( !strcmp(sg_argv[0], CmdPtr->Cmd) )
                    {
                        printf("\n");
                        fflush(stdout);
                        CmdPtr->CmdHandler( sg_argc, sg_argv );
                        break;
                    }
                }


                for( CmdPtr=gCliCmdTablePerformance; CmdPtr->Cmd; CmdPtr ++ )
                {
                    if ( !strcmp(sg_argv[0], CmdPtr->Cmd) )
                    {
                        printf("\n");
                        fflush(stdout);
                        CmdPtr->CmdHandler( sg_argc, sg_argv );
                        break;
                    }
                }

                for( CmdPtr=gCliCmdTableSIG; CmdPtr->Cmd; CmdPtr ++ )
                {
                    if ( !strcmp(sg_argv[0], CmdPtr->Cmd) )
                    {
                        printf("\n");
                        fflush(stdout);
                        CmdPtr->CmdHandler( sg_argc, sg_argv );
                        break;
                    }
                }

                for( CmdPtr=gCliCmdTableProflie; CmdPtr->Cmd; CmdPtr ++ )
                {
                    if ( !strcmp(sg_argv[0], CmdPtr->Cmd) )
                    {
                        printf("\n");
                        fflush(stdout);
                        CmdPtr->CmdHandler( sg_argc, sg_argv );
                        break;
                    }
                }

                for( CmdPtr=gCliCmdTableFaulty; CmdPtr->Cmd; CmdPtr ++ )
                {
                    if ( !strcmp(sg_argv[0], CmdPtr->Cmd) )
                    {
                        printf("\n");
                        fflush(stdout);
                        CmdPtr->CmdHandler( sg_argc, sg_argv );
                        break;
                    }
                }

                for( CmdPtr=gCliCmdTableCFI; CmdPtr->Cmd; CmdPtr ++ )
                {
                    if ( !strcmp(sg_argv[0], CmdPtr->Cmd) )
                    {
                        printf("\n");
                        fflush(stdout);
                        CmdPtr->CmdHandler( sg_argc, sg_argv );
                        break;
                    }
                }

                for( CmdPtr=gCliCmdTableFunc; CmdPtr->Cmd; CmdPtr ++ )
                {
                    if ( !strcmp(sg_argv[0], CmdPtr->Cmd) )
                    {
                        printf("\n");
                        fflush(stdout);
                        CmdPtr->CmdHandler( sg_argc, sg_argv );
                        break;
                    }
                }

                for( CmdPtr=gCliCmdTableMulti; CmdPtr->Cmd; CmdPtr ++ )
                {
                    if ( !strcmp(sg_argv[0], CmdPtr->Cmd) )
                    {
                        printf("\n");
                        fflush(stdout);
                        CmdPtr->CmdHandler( sg_argc, sg_argv );
                        break;
                    }
                }

#if 0
                if ( (NULL==CmdPtr->Cmd) && (0!=sg_curpos) )
                {
                    printf("\nCommand not found!!\n");
                    fflush(stdout);
                }
#endif
            }
            printf("\n%s", CLI_PROMPT);
            fflush(stdout);
            sg_cmd_buff[0] = 0x00;
            sg_curpos = 0;
            break;

        case '?': /* for help */
            putchar(ch);
            fflush(stdout);
            cli_cmd_usage();
            break;

        default: /* other characters */
            if ( (CLI_BUFFER_SIZE-1) > sg_curpos )
            {
                sg_cmd_buff[sg_curpos++] = ch;
                sg_cmd_buff[sg_curpos] = 0x00;
                putchar(ch);
                fflush(stdout);
            }
            break;
    }



}
