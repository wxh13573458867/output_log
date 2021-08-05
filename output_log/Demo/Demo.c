#include "output_log.h"

int main()
{
	int ret = LOG_Init("./log_out", LEVEL_DEBUG, LEVEL_WARNING);
	ret == 0 ? (printf("初始化成功\n")):(printf("初始化失败\n"));

	LOG_Out(LEVEL_DEBUG, 0, "用于调试打印");
	LOG_Out(LEVEL_INFO, 0, "程序正常运行");
	LOG_Out(LEVEL_WARNING, 0, "程序警告:%s", "警告原因");
	LOG_Out(LEVEL_ERROR, 30, "出现致命错误, 进程ID[%d], 处理结果[%s]", getpid(), "exit");

	LOG_Destroy();
	return 0;
}
