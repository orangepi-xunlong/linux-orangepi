#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/time.h>
#include <string.h>

#include "types.h"
#include "ver_info.h"
#include "log.h"

#define LOG_FILE_PATH   "./log/result.csv"

const char * type_str[]={
    "ADVERTISER",
    "SCANNER",
    "INITATOR",
};

const char * multirole_type_str[]={
    "DUT1",
    "DUT2",
    "DUT3",
    "DUT4",
};

const char result_str[2][8]={
	"SUCCESS",
	"FAIL"
};

void log_init(const char *file_name){

    /* add by ShaoShih 2015_08_06 */
    char cut_Word[]= "Cmd_";
    char cmd_str[128];
    char result[100];
    int source_length = 0;
    int cut_length = 0;

    source_length = strlen(file_name);
    cut_length = strlen(cut_Word);

    memcpy(result, file_name + cut_length, source_length - cut_length);
    result[source_length - cut_length] ='\0';
    sprintf(cmd_str , "rm -f ./log/%s.txt" , result);
    /*****************************/

	system(cmd_str);
	//printf("check result : %s\n" ,result);
	sprintf(cmd_str , "touch ./log/%s.txt" , result);
	system(cmd_str);
}

void log_info(const char *func_name ,const char * fmt,...){

	  FILE *log_file;

	  char file_name[128];
	  sprintf(file_name ,"./log/%s.txt" , func_name);
	  log_file =fopen(file_name , "a");

      char now[256];
      log_time(now);
      printf("%s " ,now);

      fprintf(log_file,"%s " ,now);


	  va_list ap;                                /* special type for variable    */
	  char format[240];                          /* argument lists               */
	  int count = 0;
	  int i, j;                                  /* Need all these to store      */
	  char c;                                    /* values below in switch       */
	  double d;
	  unsigned u;
	  char *s;
	  void *v;

	  va_start(ap, fmt);                         /* must be called before work   */
	  while (*fmt) {
		for (j = 0; fmt[j] && fmt[j] != '%'; j++)
		  format[j] = fmt[j];                    /* not a format string          */
		if (j) {
		  format[j] = '\0';
		  count += fprintf(log_file,"%s" ,format);    /* log it verbatim              */
		  fmt += j;
		} else {
		  for (j = 0; !isalpha(fmt[j]); j++) {   /* find end of format specifier */
			format[j] = fmt[j];
			if (j && fmt[j] == '%')              /* special case printing '%'    */
			  break;
		  }
		  format[j] = fmt[j];                    /* finish writing specifier     */
		  format[j + 1] = '\0';                  /* don't forget NULL terminator */
		  fmt += j + 1;

		  switch (format[j]) {                   /* cases for all specifiers     */
		  case 'd':
		  case 'i':                              /* many use identical actions   */
			i = va_arg(ap, int);                 /* process the argument         */
			count += fprintf(log_file, format, i); /* and log it                 */
			break;
		  case 'o':
		  case 'x':
		  case 'X':
		  case 'u':
			u = va_arg(ap, unsigned);
			count += fprintf(log_file, format, u);
			break;
		  case 'c':
			c = (char) va_arg(ap, int);          /* must cast!                   */
			count += fprintf(log_file, format, c);
			break;
		  case 's':
			s = va_arg(ap, char *);
			count += fprintf(log_file, format, s);
			break;
		  case 'f':
		  case 'e':
		  case 'E':
		  case 'g':
		  case 'G':
			d = va_arg(ap, double);
			count += fprintf(log_file, format, d);
			break;
		  case 'p':
			v = va_arg(ap, void *);
			count += fprintf(log_file, format, v);
			break;
		  case 'n':
			count += fprintf(log_file, "%d", count);
			break;
		  case '%':
			count += fprintf(log_file, "%%");
			break;
		  default:
			fprintf(stderr, "Invalid format specifier in log().\n");
		  }
		}
	  }

	  va_end(ap);                                /* clean up                     */
	  fclose(log_file);
}

void log_result(const char*file_name , int result){

    FILE *log_file;

	struct timeval tv;
	time_t nowtime;
	struct tm *nowtm;
	char tmbuf[256], buf[256];

    /* add by ShaoShih 2015_08_06 */
    char cut_Word[]= "Cmd_";
    char result_buf[100];
    int source_length = 0;
    int cut_length = 0;

    source_length = strlen(file_name);
    cut_length = strlen(cut_Word);

    memcpy(result_buf, file_name + cut_length, source_length - cut_length);
    result_buf[source_length - cut_length] ='\0';


	log_file =fopen(LOG_FILE_PATH , "a");

	gettimeofday(&tv, NULL);
	nowtime = tv.tv_sec;
	nowtm = localtime(&nowtime);
    //fprintf(log_file ,"Date , Test Pattern Name , Result\n");
	strftime(tmbuf, sizeof(tmbuf), "%Y-%m-%d %H:%M:%S", nowtm);
	snprintf(buf, sizeof(buf), "%s.%06ld, [%s] , [%s]\n", tmbuf, tv.tv_usec , result_buf ,result_str[result ? 1 : 0]);
	//printf("%s \n" , buf);
	fprintf(log_file ,"%s \n" ,buf);

	fclose(log_file);
}

void log_time(char *time){

	struct timeval tv;
	time_t nowtime;
	struct tm *nowtm;
	char tmbuf[256], buf[256];

	gettimeofday(&tv, NULL);
	nowtime = tv.tv_sec;
	nowtm = localtime(&nowtime);
    //fprintf(log_file ,"Date , Test Pattern Name , Result\n");
	strftime(tmbuf, sizeof(tmbuf), "%Y-%m-%d_%H-%M-%S", nowtm);
    snprintf(time, sizeof(buf), "%s_%06ld", tmbuf, tv.tv_usec);
}

void log_svn_info(){
    FILE *log_file;
	char buf[256];
    u8   files_cnt ;

	log_file =fopen(LOG_FILE_PATH , "a");

    snprintf(buf, sizeof(buf), "compile time : %s", exec_time);
    fprintf(log_file ,"%s\n" ,buf);

    snprintf(buf, sizeof(buf), "pattern version : %s", pattern_version);
    fprintf(log_file ,"%s\n" ,buf);

    snprintf(buf, sizeof(buf), "modified files : ");
    fprintf(log_file ,"%s\n" ,buf);

    for( files_cnt = 0 ; files_cnt < FILE_NAME_SIZE ; files_cnt ++){
        if(isalpha(files_modified[files_cnt][0]) != 0){
            snprintf(buf, sizeof(buf), "\t %s", (files_modified [files_cnt]));
            fprintf(log_file ,"%s\n" ,buf);
        }
        else{
            break;
        }
    }

    fprintf(log_file ,"\n");

    fclose(log_file);
}

void log_version(){
    FILE *log_file;
	char buf[256];

	log_file =fopen(LOG_FILE_PATH , "a");

	snprintf(buf, sizeof(buf), "svn info | grep Revision > %s", LOG_FILE_PATH);
    system(buf);


	fclose(log_file);
}


void log_device(const u8 *dev_addr , const u8 type){

    FILE *log_file;
    char buf[256];

    log_file =fopen(LOG_FILE_PATH , "a");

    if (type < LOG_MULTIROLE_DEVICE_TYPE_1) {
        snprintf(buf, sizeof(buf), "%s = %02x:%02x:%02x:%02x:%02x:%02x",
                type_str[type],
                dev_addr[0],dev_addr[1],dev_addr[2],
                dev_addr[3],dev_addr[4],dev_addr[5]);
    }
    else {
        snprintf(buf, sizeof(buf), "%s = %02x:%02x:%02x:%02x:%02x:%02x",
                multirole_type_str[type - LOG_MULTIROLE_DEVICE_TYPE_1],
                dev_addr[0],dev_addr[1],dev_addr[2],
                dev_addr[3],dev_addr[4],dev_addr[5]);
    }

    fprintf(log_file ,"%s\n" ,buf);

    fclose(log_file);
}
