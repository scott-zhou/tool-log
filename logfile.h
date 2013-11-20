/*********************************************************************************
 1.The member function startlog() is the new version of function enable() and 
 * disable(), it combines the two functions together. It is strongly recommended
 * to use startlog() rather than enable() and disable(). Pass different parameter
 * to startlog() to set prtflag and logflag, as discriped below:
 * Parameter flag = 0:  both prtflag and logflag are set to 0 
 *                = 1:  prtflag is set to 0, logflag is set to 1
 *                = 2:  prtflag is set to 1, logflag is set to 0
 *                = 3:  both prtflag and logflag are set to 1
 *
 2.The function logreopen() will first close the opened file, and then open a new
 * file as specified with the parameter (char* path, char* name).
 * 
 3.The function loglen() will return the file size.
 * 
 4.logfile can automatically write log into another file if size is large,
 * don't use logreopen()
 *
 5.You must call logopen() first, then call startlog(int).
 * But SetLogLevel(int) and SetLogSize(int) can be called at any time.
 *
 6.logopen() has a default parameter LOG_OPEN_LAST_FILE. 
 * If you want to open a new log file when the whole program first start, you can call
 * logopen() with the parameter LOG_OPEN_NEW_FILE in the monitor process, and 
 * other process call logopen() without parameter. 
 * If you want to continue the last log file, just call logopen(void) in every process
 *
 *********************************************************************************/

#ifndef LOGFILE_H_CC
#define LOGFILE_H_CC

#include <stdio.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>

#define LOG_PATH_LEN 256
#define LOG_NAME_LEN 64

#define LOG_OPEN_NEW_FILE	1
#define LOG_OPEN_LAST_FILE	0

class LogFile
{
private:
	int writing_file(char * printbuf);
	int mkdirectory(char * path_name,char *childpath_name = NULL);
	
public:
	LogFile(const char* path, const char* name);
	~LogFile();
	
	LogFile();
	int SetLogPathName(const char* path, const char* name);
	
	int GetLogPathName(char * pathname);
	
	int logopen(int nextfile=LOG_OPEN_LAST_FILE);
	int logreopen(const char* path, const char* name);
	
	int logwrite(const char* format, ...);
	int logwrite(int level, const char* format, ...);
	int logwrite(int level, int msglen, unsigned char * msgbuf);
	
	int enable(int flag);
	int disable(int flag);
	int startlog(int flag);
	void logclose(int flag = 9);
	long loglen();
	int SetLoglevel(int level);
	int GetLoglevel() const { return loglevel; }
	
	int SetLogSize(int size);
	
	int SetLogLine(int lines);
	
	int IsDateChanged();
	
	int SetLogTimeFlag(int flag);
	int logtime();
	
	int SetSubdirMonth(int flag);
	
	int logChangeFile();	
	
	int SetMultiThreadFlag(int flag);
	
	int isSameDay(time_t t1, time_t t2);
	int rename_old_file_before_first_open(char * current_filename, time_t tfile_modify);
	
	int get_file_lines(char * file);
	
	int isFileInodeChanged(int fd, char * filename);
	
protected:
	int file0;
	int file1;
	int plogfile;
	int	whichfile;
	
	char currentLogPathName[LOG_PATH_LEN];
	
	char logpath[LOG_PATH_LEN];
	char logname[LOG_NAME_LEN];
	unsigned long logshare;
	int logflag;
	int fileopenflag;
	int prtflag;
	int timeflag;
	int loglevel;
	
	int logsize;
	
	int loglinemax;
	int logline;
	
	struct tm tm4file;
	int	filefix;
	
	int checklog;
	
	int logtransfer;
	
	int logtimeflag;
	
	int flag_subdir_month;
	
	int flag_multi_thread;
	
	int flag_file_inode_changed;
	
	static pthread_key_t dkey;
	static pthread_once_t once_control;
	static void buf_free(void * buffer);
	static void create_key(void);
	
	static pthread_mutex_t access_mutex;
};

#endif

