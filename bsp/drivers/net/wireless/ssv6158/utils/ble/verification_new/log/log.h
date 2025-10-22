#ifndef _LOG_H_
#define _LOG_H_

#include "types.h"

#define NONE  "\e[m"
#define COLOR "\e[0;32;31m"
#define ERR   "\e[1;33m"
#define INV   "\e[7m"

#define COLOR_NONE			"\e[m"
#define COLOR_RED			"\e[0;31m"
#define COLOR_GREEN		"\e[0;32m"
#define COLOR_BROWN			"\e[0;33m"
#define COLOR_BLUE			"\e[0;34m"
#define COLOR_MAGENTA		"\e[0;35m"
#define COLOR_CYAN			"\e[0;36m"
#define COLOR_LIGHT_GRAY	"\e[0;37m"
#define COLOR_DARK_RED		"\e[1;31m"
#define COLOR_DARK_GREEN	"\e[1;32m"
#define COLOR_YELLOW		"\e[1;33m"
#define COLOR_DARK_BLUE		"\e[1;34m"
#define COLOR_DARK_MAGENTA	"\e[1;35m"
#define COLOR_DARK_CYAN		"\e[1;36m"
#define COLOR_WHITE		"\e[1;37m"

#define COLOR_DBG		COLOR_DARK_BLUE
#define COLOR_INFO		COLOR_CYAN
#define COLOR_WARN		COLOR_DARK_MAGENTA
#define COLOR_ERR		COLOR_YELLOW

#define LOG_PRINTF(fmt, ...)    printf(fmt, ##__VA_ARGS__)
#define LOG_INFO(...)           log_info(__FUNCTION__ , __VA_ARGS__);printf(__VA_ARGS__);
#define LOG_DUT(...)            log_info(log_file , __VA_ARGS__);printf(__VA_ARGS__);
#define LOG_INIT()              //log_init(__FUNCTION__)
#define LOG_RESULT(VAL)         log_result(__FUNCTION__ , VAL)

#define LOG_DEVICE(ADDR , TYPE)         log_device(ADDR ,TYPE)

#define LOG_DEVICE_TYPE_ADVERTISER  0x00
#define LOG_DEVICE_TYPE_SCANNER     0x01
#define LOG_DEVICE_TYPE_INITATOR    0x02

#define LOG_MULTIROLE_DEVICE_TYPE_1  0x04
#define LOG_MULTIROLE_DEVICE_TYPE_2  0x05
#define LOG_MULTIROLE_DEVICE_TYPE_3  0x06
#define LOG_MULTIROLE_DEVICE_TYPE_4  0x07

void log_time(char* now);
void log_init(const char *file_name);
void log_info(const char *func_name ,const char * fmt,...);
void log_result(const char*file_name , int result);
void log_version();
void log_svn_info();
void log_device(const u8 *dev_addr , const u8 type);

#endif	// _LOG_H_
