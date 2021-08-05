#ifndef OUTPUT_LOG_H
#define OUTPUT_LOG_H

#if  defined(_WIN32) || defined(_WIN64)
#define  _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <direct.h>
#include <io.h>
#elif defined(__linux)
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <dirent.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <time.h>

#define LEVEL_NOLOG	(0)
#define LEVEL_DEBUG	(1)
#define LEVEL_INFO	(2)
#define LEVEL_WARNING	(3)
#define LEVEL_ERROR	(4)

#define LOG_PATH_FILE_LEN (512)
#define LOG_OUT_TEXT_LEN (1024 * 10)

#define LOG_Out(level, status, ...) \
	LOG_Output(__FILE__, __LINE__, level, status, __VA_ARGS__)


/*
 *@brief	初始化LOG日志
 *@param:pathname	存放日志的路径
 *@param:writelevel	写入文件的最小级别
 *@param:printlevel	打印屏幕的最小级别
 *@return       成功:0  失败:-1
 **/
int LOG_Init(const char *pathname, const int writelevel, const int printlevel);

/*
 *@brief	清理LOG初始化
 *@return	无
 **/
void LOG_Destroy();

/*
 *@brief	输出LOG日志
 *@param:profile	输出日志的源文件
 *@param:proline	输出日志的源文件的行号
 *@param:level	当前日志的级别
 *@param:status	错误号
 *@param:notes	日志文本
 *@return	无
 **/
void LOG_Output(const char *profile, const int proline, const int level,\
		const int status, const char *notes, ...);

#endif
