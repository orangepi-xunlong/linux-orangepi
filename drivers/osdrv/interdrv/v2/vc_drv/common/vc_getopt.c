#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/poll.h>
#include <linux/cdev.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <asm/io.h>
#include <linux/fs.h>

#include <asm/uaccess.h>
#include<linux/slab.h>
#include<linux/interrupt.h>
#include <linux/io.h>
#include <linux/pm.h>
#include <linux/mutex.h>
#include <linux/miscdevice.h>

#include "vc_getopt.h"

struct _getopt_data {
    int optind;
    int opterr;
    int optopt;
    char *optarg;
    int __initialized;

    char *__nextchar;

    enum {
        REQUIRE_ORDER, PERMUTE, RETURN_IN_ORDER
    } __ordering;

    int __posixly_correct;

    int __first_nonopt;
    int __last_nonopt;


};


#define SWAP_FLAGS(ch1, ch2)


#define _GETOPT_DATA_INITIALIZER	{ 1, 1 }

int optind = 1;

int opterr = 1;
int optopt = 1;
char *optarg;

static struct _getopt_data getopt_data;

static void exchange(char **argv, struct _getopt_data *d)
{
    int bottom = d->__first_nonopt;
    int middle = d->__last_nonopt;
    int top = d->optind;
    char *tem;

    while (top > middle && middle > bottom) {
        if (top - middle > middle - bottom) {
            /* Bottom segment is the short one.  */
            int len = middle - bottom;
            register int i;

            /* Swap it with the top part of the top segment.  */
            for (i = 0; i < len; i++) {
                tem = argv[bottom + i];
                argv[bottom + i] = argv[top - (middle - bottom) + i];
                argv[top - (middle - bottom) + i] = tem;
                SWAP_FLAGS(bottom + i, top - (middle - bottom) + i);
            }

            /* Exclude the moved bottom segment from further swapping.  */
            top -= len;
        } else {
            /* Top segment is the short one.  */
            int len = top - middle;
            register int i;

            /* Swap it with the bottom part of the bottom segment.  */
            for (i = 0; i < len; i++) {
                tem = argv[bottom + i];
                argv[bottom + i] = argv[middle + i];
                argv[middle + i] = tem;
                SWAP_FLAGS(bottom + i, middle + i);
            }

            /* Exclude the moved top segment from further swapping.  */
            bottom += len;
        }
    }

    /* Update records for the slots the non-options now occupy.  */

    d->__first_nonopt += (d->optind - d->__last_nonopt);
    d->__last_nonopt = d->optind;
}


static const char *
_getopt_initialize(int argc, char *const *argv, const char *optstring,
           struct _getopt_data *d)
{
    /* Start processing options with ARGV-element 1 (since ARGV-element 0
       is the program name); the sequence of previously skipped
       non-option ARGV-elements is empty.  */

    d->__first_nonopt = d->__last_nonopt = d->optind;

    d->__nextchar = NULL;

    d->__posixly_correct = 0;//!!getenv ("POSIXLY_CORRECT");

    /* Determine how to handle the ordering of options and nonoptions.  */

    if (optstring[0] == '-') {
        d->__ordering = RETURN_IN_ORDER;
        ++optstring;
    } else if (optstring[0] == '+') {
        d->__ordering = REQUIRE_ORDER;
        ++optstring;
    } else if (d->__posixly_correct)
        d->__ordering = REQUIRE_ORDER;
    else
        d->__ordering = PERMUTE;

    return optstring;
}


int _getopt_internal_r(int argc, char *const *argv, const char *optstring,
               const struct option *longopts, int *longind,
               int long_only, struct _getopt_data *d)
{
    int print_errors = d->opterr;

    if (optstring[0] == ':')
        print_errors = 0;

    if (argc < 1)
        return -1;

    d->optarg = NULL;

    if (d->optind == 0 || !d->__initialized) {
        if (d->optind == 0)
            d->optind = 1;    /* Don't scan ARGV[0], the program name.  */

        optstring = _getopt_initialize(argc, argv, optstring, d);
        d->__initialized = 1;
    }



# define NONOPTION_P (argv[d->optind][0] != '-' || argv[d->optind][1] == '\0')

    if (d->__nextchar == NULL || *d->__nextchar == '\0') {
        /* Advance to the next ARGV-element.    */

        /* Give FIRST_NONOPT & LAST_NONOPT rational values if OPTIND has been
        moved back by the user (who may also have changed the arguments).    */
        if (d->__last_nonopt > d->optind)
            d->__last_nonopt = d->optind;

        if (d->__first_nonopt > d->optind)
            d->__first_nonopt = d->optind;

        if (d->__ordering == PERMUTE) {
            /* If we have just processed some options following some non-options,
             exchange them so that the options come first.    */

            if (d->__first_nonopt != d->__last_nonopt
                    && d->__last_nonopt != d->optind)
                exchange((char **) argv, d);
            else if (d->__last_nonopt != d->optind)
                d->__first_nonopt = d->optind;

            /* Skip any additional non-options
             and extend the range of non-options previously skipped.  */

            while (d->optind < argc && NONOPTION_P)
                d->optind++;

            d->__last_nonopt = d->optind;
        }

        /* The special ARGV-element `--' means premature end of options.
        Skip it like a null option,
        then exchange with previous non-options as if it were an option,
        then skip everything else like a non-option.  */

        if (d->optind != argc && !strcmp(argv[d->optind], "--")) {
            d->optind++;

            if (d->__first_nonopt != d->__last_nonopt
                    && d->__last_nonopt != d->optind)
                exchange((char **) argv, d);
            else if (d->__first_nonopt == d->__last_nonopt)
                d->__first_nonopt = d->optind;

            d->__last_nonopt = argc;

            d->optind = argc;
        }

        /* If we have done all the ARGV-elements, stop the scan
        and back over any non-options that we skipped and permuted.  */

        if (d->optind == argc) {
            /* Set the next-arg-index to point at the non-options
             that we previously skipped, so the caller will digest them.  */
            if (d->__first_nonopt != d->__last_nonopt)
                d->optind = d->__first_nonopt;

            return -1;
        }

        /* If we have come to a non-option and did not permute it,
        either stop the scan or describe it to the caller and pass it by.    */

        if (NONOPTION_P) {
            if (d->__ordering == REQUIRE_ORDER)
                return -1;

            d->optarg = argv[d->optind++];
            return 1;
        }

        /* We have found another option-ARGV-element.
        Skip the initial punctuation.    */

        d->__nextchar = (argv[d->optind] + 1
                 + (longopts != NULL && argv[d->optind][1] == '-'));
    }

    if (longopts != NULL
            && (argv[d->optind][1] == '-'
                || (long_only && (argv[d->optind][2]
                          || !strchr(optstring, argv[d->optind][1]))))) {
        char *nameend;
        const struct option *p;
        const struct option *pfound = NULL;
        int exact = 0;
        int ambig = 0;
        int indfound = -1;
        int option_index;

        for (nameend = d->__nextchar; *nameend && *nameend != '='; nameend++) {
            //do nothing
        }

        /* Test all long options for either exact match
        or abbreviated matches.  */
        for (p = longopts, option_index = 0; p->name; p++, option_index++)
            if (!strncmp(p->name, d->__nextchar, nameend - d->__nextchar)) {
                if ((unsigned int)(nameend - d->__nextchar)
                        == (unsigned int) strlen(p->name)) {
                    /* Exact match found.  */
                    pfound = p;
                    indfound = option_index;
                    exact = 1;
                    break;
                } else if (pfound == NULL) {
                    /* First nonexact match found.    */
                    pfound = p;
                    indfound = option_index;
                } else if (long_only
                        || pfound->has_arg != p->has_arg
                        || pfound->flag != p->flag
                        || pfound->val != p->val)
                    /* Second or later nonexact match found.    */
                    ambig = 1;
            }

        if (ambig && !exact) {
            d->__nextchar += strlen(d->__nextchar);
            d->optind++;
            d->optopt = 0;
            return '?';
        }

        if (pfound != NULL) {
            option_index = indfound;
            d->optind++;

            if (*nameend) {
                /* Don't test has_arg with >, because some C compilers don't
                allow it to be used on enums.    */
                if (pfound->has_arg)
                    d->optarg = nameend + 1;
                else {
                    d->__nextchar += strlen(d->__nextchar);

                    d->optopt = pfound->val;
                    return '?';
                }
            } else if (pfound->has_arg == 1) {
                if (d->optind < argc)
                    d->optarg = argv[d->optind++];
                else {
                    d->__nextchar += strlen(d->__nextchar);
                    d->optopt = pfound->val;
                    return optstring[0] == ':' ? ':' : '?';
                }
            }

            d->__nextchar += strlen(d->__nextchar);

            if (longind != NULL)
                *longind = option_index;

            if (pfound->flag) {
                *(pfound->flag) = pfound->val;
                return 0;
            }

            return pfound->val;
        }

        /* Can't find it as a long option.  If this is not getopt_long_only,
        or the option starts with '--' or is not a valid short
        option, then it's an error.
        Otherwise interpret it as a short option.    */
        if (!long_only || argv[d->optind][1] == '-'
                || strchr(optstring, *d->__nextchar) == NULL) {
            d->__nextchar = (char *) "";
            d->optind++;
            d->optopt = 0;
            return '?';
        }
    }

    /* Look at and handle the next short option-character.  */

    {
        char c = *d->__nextchar++;
        char *temp = strchr(optstring, c);

        /* Increment `optind' when we start to process its last character.    */
        if (*d->__nextchar == '\0')
            ++d->optind;

        if (temp == NULL || c == ':') {

            d->optopt = c;
            return '?';
        }

        /* Convenience. Treat POSIX -W foo same as long option --foo */
        if (temp[0] == 'W' && temp[1] == ';') {
            char *nameend;
            const struct option *p;
            const struct option *pfound = NULL;
            int exact = 0;
            int ambig = 0;
            int indfound = 0;
            int option_index;

            /* This is an option that requires an argument.  */
            if (*d->__nextchar != '\0') {
                d->optarg = d->__nextchar;
                /* If we end this ARGV-element by taking the rest as an arg,
                   we must advance to the next element now.  */
                d->optind++;
            } else if (d->optind != argc) {
                d->optarg = argv[d->optind++];
            } else if (d->optind == argc) {
                d->optopt = c;

                if (optstring[0] == ':')
                    c = ':';
                else
                    c = '?';

                return c;
            }

            /* optarg is now the argument, see if it's in the
               table of longopts.  */

            for (d->__nextchar = nameend = d->optarg; *nameend && *nameend != '=';
                    nameend++) {
                //do nothing
            }

            /* Test all long options for either exact match
               or abbreviated matches.    */
            for (p = longopts, option_index = 0; p->name; p++, option_index++)
                if (!strncmp(p->name, d->__nextchar, nameend - d->__nextchar)) {
                    if ((unsigned int)(nameend - d->__nextchar) == strlen(p->name)) {
                        /* Exact match found.  */
                        pfound = p;
                        indfound = option_index;
                        exact = 1;
                        break;
                    } else if (pfound == NULL) {
                        /* First nonexact match found.  */
                        pfound = p;
                        indfound = option_index;
                    } else
                        /* Second or later nonexact match found.  */
                        ambig = 1;
                }

            if (ambig && !exact) {
                d->__nextchar += strlen(d->__nextchar);
                d->optind++;
                return '?';
            }

            if (pfound != NULL) {
                option_index = indfound;

                if (*nameend) {
                    /* Don't test has_arg with >, because some C compilers don't
                       allow it to be used on enums.  */
                    if (pfound->has_arg)
                        d->optarg = nameend + 1;
                    else {
                        d->__nextchar += strlen(d->__nextchar);
                        return '?';
                    }
                } else if (pfound->has_arg == 1) {
                    if (d->optind < argc)
                        d->optarg = argv[d->optind++];
                    else {
                        d->__nextchar += strlen(d->__nextchar);
                        return optstring[0] == ':' ? ':' : '?';
                    }
                }

                d->__nextchar += strlen(d->__nextchar);

                if (longind != NULL)
                    *longind = option_index;

                if (pfound->flag) {
                    *(pfound->flag) = pfound->val;
                    return 0;
                }

                return pfound->val;
            }

            d->__nextchar = NULL;
            return 'W';    /* Let the application handle it.    */
        }

        if (temp[1] == ':') {
            if (temp[2] == ':') {
                /* This is an option that accepts an argument optionally.  */
                if (*d->__nextchar != '\0') {
                    d->optarg = d->__nextchar;
                    d->optind++;
                } else
                    d->optarg = NULL;

                d->__nextchar = NULL;
            } else {
                /* This is an option that requires an argument.  */
                if (*d->__nextchar != '\0') {
                    d->optarg = d->__nextchar;
                    /* If we end this ARGV-element by taking the rest as an arg,
                       we must advance to the next element now.  */
                    d->optind++;
                } else if (d->optind == argc) {

                    d->optopt = c;

                    if (optstring[0] == ':')
                        c = ':';
                    else
                        c = '?';
                } else
                    /* We already incremented `optind' once;
                    increment it again when taking next ARGV-elt as argument.    */
                    d->optarg = argv[d->optind++];

                d->__nextchar = NULL;
            }
        }

        return c;
    }
}




//static struct _getopt_data getopt_data;

int _getopt_internal(int argc, char **argv, const char *optstring,
             const struct option *longopts, int *longind, int long_only)
{
    int result;

    getopt_data.optind = optind;
    getopt_data.opterr = opterr;

    result = _getopt_internal_r(argc, argv, optstring, longopts,
                    longind, long_only, &getopt_data);

    optind = getopt_data.optind;
    optarg = getopt_data.optarg;
    optopt = getopt_data.optopt;

    return result;
}


int
getopt_long(int argc, char **argv, const char *options,
        const struct option *long_options, int *opt_index)
{
    return _getopt_internal(argc, argv, options, long_options, opt_index, 0);
}

void getopt_init(void)
{
    optind = 1;
    opterr = 1;
    optopt = 1;
    memset(&getopt_data, 0, sizeof(getopt_data));
}


char *drv_strchr(char *s, char c)
{
    while (*s != '\0' && *s != c) {
        ++s;
    }

    return *s == c ? s : NULL;
}


char *drv_strpbrk(const char *cs, const char *ct)
{
    const char *sc1, *sc2;

    for (sc1 = cs; *sc1 != '\0'; ++sc1) {
        for (sc2 = ct; *sc2 != '\0'; ++sc2) {
            if (*sc1 == *sc2)
                return (char *) sc1;
        }
    }

    return NULL;
}


int drv_strspn(const char *s, const char *accept)
{
    const char *p;
    const char *a;
    int count = 0;

    for (p = s; *p != '\0'; ++p) {
        for (a = accept; *a != '\0'; ++a) {
            if (*p == *a) {
                break;
            }
        }

        if (*a == '\0') {
            return count;
        }

        ++count;
    }

    return count;
}


char *drv_strtok_r(char *s, const char *delim, char **save_ptr)
{
    char *token;

    if (s == NULL)
        s = *save_ptr;

    /* Scan leading delimiters.  */
    s += drv_strspn(s, delim);

    if (*s == '\0')
        return NULL;

    /* Find the end of the token.  */
    token = s;
    s = drv_strpbrk(token, delim);

    if (s == NULL)
        /* This token finishes the string.  */
        *save_ptr = drv_strchr(token, '\0');
    else {
        /* Terminate the token and make *SAVE_PTR point past it.  */
        *s = '\0';
        *save_ptr = s + 1;
    }

    return token;
}


char *drv_strtok(char *str, const char *delim)
{
    static char *rembmberLastString;
    const char *indexDelim = delim;
    int flag = 1, index = 0;
    char *temp = NULL;

    if (str == NULL) {
        str = rembmberLastString;
    }

    for (; *str; str++) {
        delim = indexDelim;

        for (; *delim; delim++) {
            if (*str == *delim) {
                *str = 0;
                index = 1;
                break;
            }
        }

        if (*str != 0 && flag == 1) {
            temp = str;
            flag = 0;
        }

        if (*str != 0 && flag == 0 && index == 1) {
            rembmberLastString = str;
            return temp;
        }
    }

    rembmberLastString = str;
    return temp;
}

int drv_isdigit(int c)
{
    return '0' <= c && c <= '9';
}


#define DRV_INT_MAX 2147483647
#define DRV_INT_MIN -2147483678

#if 0
long long strtoi(const char *str, int minus)
{
    long long num = 0;
    int flag = minus ? -1 : 1;

    while (*str != '\0') {
        if (*str >= '0' && *str <= '9') {
            num = num * 10 + flag * (*str - '0');
            ++str;
        } else {
            break;
        }
    }

    return num;
}
#endif
extern long long strtoi(const char *str, int minus);

int atoi(const char *str)
{
    int num = 0;
    int minus = false;

    if (str != NULL && *str != '\0') {
        while (*str == ' ') {
            ++str;
            continue;
        }

        if (*str == '+') {
            ++str;
        } else if (*str == '-') {
            ++str;
            minus = true;
        }

        if (*str != '\0') {
            num = strtoi(str, minus);
        }
    }

    return num;
}

