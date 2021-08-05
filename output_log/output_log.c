#include "output_log.h"

static const char LOG_Level[5][6] = {"NOLOG", "DEBUG", "INFO ", "WARN ", "ERROR"};
static char LOG_Path[LOG_PATH_FILE_LEN] = {0};
static char *LOG_Text = NULL;
static int LOG_Write = LEVEL_NOLOG;
static int LOG_Print = LEVEL_NOLOG;
static int LOG_IsInit = 0;

#if defined(_WIN32) || defined(_WIN64)

static int LOG_Init_Windows(const char* pathname, const int writelevel, const int printlevel)
{
	HANDLE LOG_Semid = NULL;
	if ((LOG_Semid = CreateSemaphore(NULL, 1, 1, L"LOG_SEM_KEY")) == NULL) {
		return -1;
	}
	if (WaitForSingleObject(LOG_Semid, INFINITE) == WAIT_OBJECT_0) {

		if (strlen(pathname) <= 0 || strlen(pathname) >= LOG_PATH_FILE_LEN || \
				writelevel < LEVEL_NOLOG || writelevel > LEVEL_ERROR || \
				printlevel < LEVEL_NOLOG || printlevel > LEVEL_ERROR || \
				LOG_IsInit) {
			ReleaseSemaphore(LOG_Semid, 1, NULL);
			CloseHandle(LOG_Semid);
			return -1;
		}
		if ((LOG_Text = (char*)malloc(LOG_OUT_TEXT_LEN)) == NULL) {
			ReleaseSemaphore(LOG_Semid, 1, NULL);
			CloseHandle(LOG_Semid);
			return -1;
		}
		LOG_IsInit = 1;

		LOG_Write = writelevel;
		LOG_Print = printlevel;

		char pathtemp[LOG_PATH_FILE_LEN] = { 0 };
		char pathdir[LOG_PATH_FILE_LEN] = { 0 };
		int index = ((pathname[1] == ':' && (pathname[0] >= 65 || \
						pathname[0] <= 90 || pathname[0] >= 97 || \
						pathname[0] <= 122)) ? 2 : 0);
		int pathrank = 0;

		while (strlen(pathdir) < strlen(pathname) - 1) {
			memset(pathtemp, 0x00, LOG_PATH_FILE_LEN);
			sscanf(pathname + index, "%[^/]", pathtemp) > 1 ? 1 : 0;


			index += strlen(pathtemp) + 1;
			if (pathrank++ > 0) {
				strcat(pathdir, "/");
			}
			if ((pathname[1] == ':' && (pathname[0] >= 65 || \
							pathname[0] <= 90 || pathname[0] >= 97 || \
							pathname[0] <= 122))) {
				strncpy(pathdir, pathname, 2);
			}
			pathdir[LOG_PATH_FILE_LEN - 1] = '\0';
			if (strlen(pathtemp) > 0) {
				strcat(pathdir, pathtemp);

				if (_access(pathdir, 0x00) == -1) {
					if (_mkdir(pathdir)) {
						ReleaseSemaphore(LOG_Semid, 1, NULL);
						CloseHandle(LOG_Semid);
						return -1;
					}
				}
			}
			printf("%s\n", pathdir);
			pathdir[LOG_PATH_FILE_LEN - 1] = '\0';
		}
		strcat(pathdir, "/");
		strcpy(LOG_Path, pathdir);

	}
	ReleaseSemaphore(LOG_Semid, 1, NULL);
	CloseHandle(LOG_Semid);
	return 0;
}

static void LOG_Destroy_Windows(){
	HANDLE LOG_Semid = NULL;
	if((LOG_Semid = CreateSemaphore(NULL, 1, 1, L"LOG_SEM_KEY")) == NULL){
		return;
	}
	if (WaitForSingleObject(LOG_Semid, INFINITE) == WAIT_OBJECT_0) {
		if(LOG_IsInit){
			memset(LOG_Path, 0x00, LOG_PATH_FILE_LEN);
			if(LOG_Text){
				free(LOG_Text);
				LOG_Text = NULL;
			}
			LOG_Write = LEVEL_NOLOG;
			LOG_Print = LEVEL_NOLOG;
			LOG_IsInit = 0;
		}
	}
	ReleaseSemaphore(LOG_Semid, 1, NULL);
	CloseHandle(LOG_Semid);
	return;
}

static void LOG_Write_Windows(char* file, const int level, char* text, int len)
{
	HANDLE LOG_Semid = NULL;
	if((LOG_Semid = CreateSemaphore(NULL, 1, 1, L"LOG_SEM_KEY")) == NULL){
		return;
	}
	if (WaitForSingleObject(LOG_Semid, INFINITE) == WAIT_OBJECT_0) {
		FILE *fp = fopen(file, "a");
		fwrite(text, len, 1, fp);
		fclose(fp);

	}
	ReleaseSemaphore(LOG_Semid, 1, NULL);
	CloseHandle(LOG_Semid);
	return;
}

#elif defined(__linux)
static int LOG_Semid = -1;

union LOG_Semun
{
	int val;
	struct semid_ds *buf;
	unsigned short *array;
	struct seminfo *__buf;
};

static int KEY_Obtain(const char *file, int proj, int *isCreate, size_t *keyid)
{
	char check[LOG_PATH_FILE_LEN] = {0};
	int fd = -1; 
	*isCreate = -1; 

	if(access(file, F_OK)){
		*isCreate = 1;
		if((fd = open(file, O_RDWR | O_CREAT, 0666)) == -1){    return -1;}
		write(fd, file, strlen(file));
		close(fd);
	}else{
		*isCreate = 0;
		if((fd = open(file, O_RDWR)) == -1){    return -1;}
		read(fd, check, strlen(file));
		close(fd);
		if(strcmp(check, file)){        return -1;}
	}   

	if((*keyid = ftok(file, proj)) == -1){  return -1;}

	return 0;
}

static int SEM_Init(const char *file, int proj, int *semid)
{
	size_t keyid = -1; 
	int isCreate = -1; 
	if(KEY_Obtain(file, proj, &isCreate, &keyid)){  return -1;}

	if(isCreate){
		if((*semid = semget(keyid, 1, IPC_CREAT | IPC_EXCL | 0666)) == -1){     return -1;}

		union LOG_Semun un_sem;
		un_sem.val = 1;
		if(semctl(*semid, 0, SETVAL, un_sem) == -1){    return -1;}
	}else{
		if((*semid = semget(keyid, 0, IPC_CREAT | 0666)) == -1){        return -1;}
	}   

	return 0;
}

static void SEM_Destroy(const char *file, int proj)
{
	size_t keyid = -1; 
	int semid = -1; 
	int isCreate = -1; 
	if(KEY_Obtain(file, proj, &isCreate, &keyid)){  return;}

	if(!isCreate){
		if((semid = semget(keyid, 0, IPC_CREAT | 0666)) != -1){
			semctl(semid, 0, IPC_RMID);
		}   
	}   
	remove(file);
	return; 
}

static int SEM_locked(int semid)
{
	struct sembuf str_sem;
	str_sem.sem_num = 0;
	str_sem.sem_op = -1; 
	str_sem.sem_flg = SEM_UNDO;

	if(semop(semid, &str_sem, 1) == -1){    return -1;}

	return 0;
}

static int SEM_unlock(int semid)
{
	struct sembuf str_sem;
	str_sem.sem_num = 0;
	str_sem.sem_op = 1;
	str_sem.sem_flg = 0;

	if(semop(semid, &str_sem, 1) == -1){    return -1;}

	return 0;
}

static int LOG_Init_Linux(const char *pathname, const int writelevel, const int printlevel)
{
	if(SEM_Init("/tmp/log_ipc_sys_key.sem", 0x00, &LOG_Semid)){
		return -1; 
	}   

	SEM_locked(LOG_Semid);

	if(strlen(pathname) <= 0 || strlen(pathname)>= LOG_PATH_FILE_LEN || \
			writelevel < LEVEL_NOLOG || writelevel > LEVEL_ERROR || \
			printlevel < LEVEL_NOLOG || printlevel > LEVEL_ERROR || \
			LOG_IsInit){
		SEM_unlock(LOG_Semid);
		return -1;
	}
	if((LOG_Text = (char *)malloc(LOG_OUT_TEXT_LEN)) == NULL){
		SEM_unlock(LOG_Semid);
		return -1;
	}

	LOG_Write = writelevel;
	LOG_Print = printlevel;

	char pathtemp[LOG_PATH_FILE_LEN] = { 0 };
	char pathdir[LOG_PATH_FILE_LEN] = { 0 };
	int index = (pathname[0] == '/' ? 1 : 0); 
	int pathrank = 0;
	DIR *dir;
	while(strlen(pathdir) < strlen(pathname) - 1){ 
		memset(pathtemp, 0x00, LOG_PATH_FILE_LEN);
		sscanf(pathname + index, "%[^/]", pathtemp);
		index += strlen(pathtemp) + 1;
		if(pathrank++ > 0 || pathname[0] == '/'){
			strcat(pathdir, "/");
		}   
		strcat(pathdir, pathtemp);

		if((dir = opendir(pathdir)) == NULL){
			if(mkdir(pathdir, 0777)){
				SEM_unlock(LOG_Semid);
				return -1; 
			}   
		}   
		else{
			closedir(dir);
		}   
	}   
	strcat(pathdir, "/");
	strcpy(LOG_Path, pathdir);

	LOG_IsInit = 1;
	SEM_unlock(LOG_Semid);

	return 0;
}

static void LOG_Destroy_Linux()
{
	SEM_locked(LOG_Semid);
	if(LOG_IsInit){
		memset(LOG_Path, 0x00, LOG_PATH_FILE_LEN);
		if(LOG_Text){
			free(LOG_Text);
			LOG_Text = NULL;
		}
		LOG_Write = LEVEL_NOLOG;
		LOG_Print = LEVEL_NOLOG;
		LOG_IsInit = 0;
	}
	SEM_unlock(LOG_Semid);
	//	SEM_Destroy("/tmp/log_ipc_sys_key.sem", 0x00);
	return;
}

static void LOG_Write_Linux(char *file, const int level, char *text, int len)
{
	SEM_locked(LOG_Semid);
	FILE *fp = fopen(file, "a");
	fwrite(text, len, 1, fp);
	fclose(fp);

	if(level == LEVEL_ERROR){
		char cmd[1024] = {0};


		memset(cmd, 0x00, 1024);
		sprintf(cmd, "echo \"start--------------------------------------------------------------------------\" >> %s", file);
		system(cmd);

		memset(cmd, 0x00, 1024);
		sprintf(cmd, "top -b -n 1 -p %ld >> %s", getpid(), file);
		system(cmd);

		memset(cmd, 0x00, 1024);
		sprintf(cmd, "echo \"----------------------------------------------------------------------------end\n\n\" >> %s", file);
		system(cmd);
	}
	SEM_unlock(LOG_Semid);
	return;
}
#endif

static void LOG_Core(const char *profile, const int proline, const int level, const int status, const char *notes, va_list args)
{
	memset(LOG_Text, 0x00, LOG_OUT_TEXT_LEN);
	char LOG_File[LOG_PATH_FILE_LEN] = { 0 };
	int LOG_TextIndex = 0;

	time_t LOG_Time = 0;
	size_t LOG_TimeLen = 0;
	struct tm *LOG_TimeTm = NULL;

	LOG_Time = time(NULL);	
	LOG_TimeTm = localtime(&LOG_Time);	

	sprintf(LOG_File, "%s%d_%02d_%02d.log", LOG_Path, 1900 + LOG_TimeTm->tm_year,\
			1 + LOG_TimeTm->tm_mon, LOG_TimeTm->tm_mday);	

	LOG_TextIndex += sprintf(LOG_Text + LOG_TextIndex, "[%02d:%02d:%02d] ", LOG_TimeTm->tm_hour,\
			LOG_TimeTm->tm_min, LOG_TimeTm->tm_sec);
	LOG_TextIndex += sprintf(LOG_Text + LOG_TextIndex, "[%s:", LOG_Level[level]);

	if(!status){
		LOG_TextIndex += sprintf(LOG_Text + LOG_TextIndex, " SUCCESS] ");
	}
	else{
		char LOG_StatusTemp[8] = { 0 };
		char LOG_StatusBuf[8] = { 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0 };
		char LOG_StatusLen = 0;
		LOG_StatusLen = sprintf(LOG_StatusTemp, "%d", status);
		strcpy(LOG_StatusBuf + 7 - LOG_StatusLen, LOG_StatusTemp);

		LOG_TextIndex += sprintf(LOG_Text + LOG_TextIndex, " %s] ", LOG_StatusBuf);
	}
	LOG_TextIndex += sprintf(LOG_Text + LOG_TextIndex, "[%s:", profile);
	LOG_TextIndex += sprintf(LOG_Text + LOG_TextIndex, "%d]\n\t", proline);
	LOG_TextIndex += vsprintf(LOG_Text + LOG_TextIndex, notes, args);
	LOG_TextIndex += sprintf(LOG_Text + LOG_TextIndex, "\n\n");

	if(level >= LOG_Write){
#if defined(_WIN32) || defined(_WIN64)
		LOG_Write_Windows(LOG_File, level, LOG_Text, LOG_TextIndex);
#elif defined(__linux)
		LOG_Write_Linux(LOG_File, level, LOG_Text, LOG_TextIndex);
#endif
	}

	if(level >= LOG_Print){
		printf("%s", LOG_Text);
	}

	return ;	
}

int LOG_Init(const char *pathname, const int writelevel, const int printlevel)
{	
#if defined(_WIN32) || defined(_WIN64)
	return LOG_Init_Windows(pathname, writelevel, printlevel);
#elif defined(__linux)
	return LOG_Init_Linux(pathname, writelevel, printlevel);
#endif
	return -1;
}
void LOG_Destroy()
{
#if defined(_WIN32) || defined(_WIN64)
	LOG_Destroy_Windows();
#elif defined(__linux)
	LOG_Destroy_Linux();
#endif
}

void LOG_Output(const char *profile, const int proline, const int level,\
		const int status, const char *notes, ...)
{
	if(LOG_Write == LEVEL_NOLOG && LOG_Print == LEVEL_NOLOG){
		return;
	}

	if((!LOG_IsInit) || level <= LEVEL_NOLOG || level > LEVEL_ERROR ||\
			(level < LOG_Write && level < LOG_Print)){
		return ;
	}

	va_list args;
	va_start(args, notes);
	LOG_Core(profile, proline, level, status, notes, args);
	va_end(args);

	return;
}

