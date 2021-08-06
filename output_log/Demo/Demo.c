#include "output_log.h"

int main()
{
	int ret = LOG_Init("./log_out", LEVEL_DEBUG, LEVEL_WARNING);
	ret == 0 ? (printf("初始化成功\n")):(printf("初始化失败\n"));
	for (int i = 0; i < 1000; ++i) {
		LDEB(0, "用于调试打印");
		LINF(0, "程序正常运行");
		LWAR(0, "程序警告:%s", "警告原因");
	}
#if  defined(_WIN32) || defined(_WIN64)
		LERR(errno - 1, "出现致命错误, 进程ID[%d], 处理结果[%s]", _getpid(), "exit");
#elif defined(__linux)
		LERR(errno - 1, "出现致命错误, 进程ID[%d], 处理结果[%s]", getpid(), "exit");
#endif

	LOG_Destroy();
	return 0;
}
