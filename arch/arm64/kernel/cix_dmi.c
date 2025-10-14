/* SPDX-License-Identifier: GPL-2.0-only */
#include "cix_dmi.h"

static char *cpu_name = "Version";
static bool get_cpu_flag = false;
static void get_cpuname_by_dmi(const struct dmi_header *dm, void *data)
{
        const char *bp;
        const u8 *nsp;
        char *sm ;
        char s;
        if(!dm)
                return;
        if (dm->type != DMI_ENTRY_PROCESSOR)
                return;

        bp = ((u8 *) dm) + dm->length;
        sm = (char *)dm;
        s = sm[0x10];

        if (s) {
                while (--s > 0 && *bp)
                        bp += strlen(bp) + 1;

                /* Strings containing only spaces are considered empty */
                nsp = bp;
                while (*nsp == ' ')
                        nsp++;
                if (*nsp != '\0'){
                        cpu_name = dmi_alloc(strlen(bp) + 1);
                        if (cpu_name != NULL)
                                strcpy(cpu_name, bp);

                        get_cpu_flag = true;
                        return;
                }
        }

        return;
}

char *get_cpu_name(void)
{
        if(!get_cpu_flag)
                dmi_walk(get_cpuname_by_dmi, cpu_name);

        if(cpu_name == NULL) {
                /* Some BIOS don't support getting CPU name from DMI  */
                return "UNKNOWN-CPU";
        }

        return cpu_name;
}

