#ifndef __DRV_VC_GET_OPTION_H__
#define __DRV_VC_GET_OPTION_H__

//#include "vdi_osal.h"
struct option {
    const char *name;
    /* has_arg can't be an enum because some compilers complain about
       type mismatches in all the code that assumes it is an int.  */
    int has_arg;
    int *flag;
    int val;
};

#define no_argument       0 // 该选项不带参数
#define required_argument 1 // 该选项必须带一个参数
#define optional_argument 2 // 该选项可以带一个参数也可以不带

extern char *optarg;

int getopt_long(int argc, char **argv, const char *options,
        const struct option *long_options, int *opt_index);

extern int getopt (int __argc, char *const *__argv, const char *__shortopts);

char *drv_strtok_r(char *s, const char *delim, char **save_ptr);
char *drv_strtok(char *str, const char *delim);
void getopt_init(void);
int atoi(const char *str);
int drv_isdigit(int c);
//double atof(const char* str);

#endif /* __DRV_VC_GET_OPTION_H__ */