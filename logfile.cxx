#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <strings.h>
#include <sys/time.h>

#include "logfile.h"


//#define LOG_FILE_CHECK_SIZE_PER_TIME	16
#define LOG_BUFFER_SIZE					(16*1024)


pthread_mutex_t LogFile::access_mutex = PTHREAD_MUTEX_INITIALIZER;

pthread_key_t LogFile::dkey = {0};
pthread_once_t LogFile::once_control = PTHREAD_ONCE_INIT;	



LogFile::LogFile()
{
	memset(logpath,0,LOG_PATH_LEN);
	memset(logname,0,LOG_NAME_LEN);
	
	//pthread_mutex_init(&access_mutex, NULL);	
	
	bzero(&tm4file, sizeof(tm4file));

	//logshare = 0664;
	logshare = 0777;
	logflag = 0;
	prtflag = 0;
	loglevel = 0;
	fileopenflag = 0;
	timeflag = 0;
	
	logsize = 2*1000*1000;		
	loglinemax = 0;		
	logline = 0;
	
	filefix = 0;
	
	checklog = 1;	
	
	whichfile = 0;	
	file0 = -1;
	file1 = -1;
	plogfile = -1;
	
	logtransfer = 0;
	
	logtimeflag = 1;
	
	flag_subdir_month = 0;
	
	flag_multi_thread = 1;	//default I think there are multi thread writing log file
	
	pthread_once(&once_control, LogFile::create_key);
}


LogFile::LogFile(const char* path, const char* name)
{
	memset(logpath,0,LOG_PATH_LEN);
	memset(logname,0,LOG_NAME_LEN);
	memcpy(logpath,path,strlen(path));
	memcpy(logname,name,strlen(name));
	
	//pthread_mutex_init(&access_mutex, NULL);		//use above
	
	bzero(&tm4file, sizeof(tm4file));

	//logshare = 0664;
	logshare = 0777;	//
	logflag = 0;
	prtflag = 0;
	loglevel = 0;
	fileopenflag = 0;
	timeflag = 0;
	
	logsize = 2*1000*1000;		//
	loglinemax = 0;				//
	logline = 0;
	
	filefix = 0;
	
	checklog = 1;		//
	
	whichfile = 0;		//respond to file0 or file1
	file0 = -1;
	file1 = -1;
	plogfile = -1;	//respond to file0 or file1, respond to fileopenflag
	
	logtransfer = 0;	//set logtransfer to 1 before call logopenNext()
	
	logtimeflag = 1;	//
	
	flag_subdir_month = 0;
	
	flag_multi_thread = 1;	//default I think there are multi thread writing log file
	
	pthread_once(&once_control, LogFile::create_key);
}

void LogFile::create_key(void)
{
	pthread_key_create(&dkey, LogFile::buf_free);
	return;
}

void LogFile::buf_free(void * buffer)
{
	if ( buffer == NULL ) return;
	free(buffer);
	return;
}

LogFile::~LogFile()
{
	logclose();
}

long LogFile::loglen()
{
	struct stat filestat;
	
	if (fileopenflag == 0) return 0;
	
	if ( whichfile == 0 ){
		if ( file0 == -1 ) return 0;			//
		//return lseek(file0, 0, SEEK_END);		//
		if ( fstat(file0, &filestat) < 0 ) return 0;
		return filestat.st_size;
	}
	else if ( whichfile == 1 ){
		if ( file1 == -1 ) return 0;			//
		//return lseek(file1, 0, SEEK_END);		//
		if ( fstat(file1, &filestat) < 0 ) return 0;
		return filestat.st_size;
	}
	else
		return 0;
}


int LogFile::SetLogPathName(const char* path, const char* name)
{
	memset(logpath,0,LOG_PATH_LEN);
	memset(logname,0,LOG_NAME_LEN);
	memcpy(logpath,path,strlen(path));
	memcpy(logname,name,strlen(name));
	return 1;
}


int LogFile::logreopen(const char* path, const char* name)
{
	logclose();

	memset(logpath,0,LOG_PATH_LEN);
	memset(logname,0,LOG_NAME_LEN);
	memcpy(logpath,path,strlen(path));
	memcpy(logname,name,strlen(name));

	//logshare = 0664;
	logshare = 0777;
	logflag = 0;
	fileopenflag = 0;
	prtflag = 0;
	timeflag = 0;

	return logopen();
}


int LogFile::logopen(int nextfile)
{
	time_t now;
	struct tm * ptm;
	
	time(&now);
	ptm = localtime(&now);
	memcpy(&tm4file, ptm, sizeof(struct tm));
	
	unsigned int pathlen,namelen;
	pathlen = strlen(logpath);
	namelen = strlen(logname);
	if(pathlen == 0 || namelen == 0) return 0;
	
	//
	if ( logpath[pathlen-1] != '/' ){	//add '/' to the end of logpath if needed
		logpath[pathlen] = '/';	
		logpath[pathlen+1] = 0;	
	}
	pathlen = strlen(logpath);
	mkdirectory(logpath);
	
	memset(currentLogPathName, 0, sizeof(currentLogPathName));
	
	if ( flag_subdir_month ){
		char tmppath[LOG_PATH_LEN];
		bzero(tmppath, sizeof(tmppath));
		sprintf(tmppath, "%s%d%02d", logpath, tm4file.tm_year+1900, tm4file.tm_mon+1);
		sprintf(currentLogPathName, "%s/%s%s", tmppath, "current.", logname);
		//mkdir() if needed
		mkdirectory(tmppath);
	}else{
		sprintf(currentLogPathName, "%s%s%s", logpath, "current.", logname);
	}

	logline = 0;
	file0 = -1;
	
	//Add by 
		int lastyear = tm4file.tm_year;
		int lastmon = tm4file.tm_mon;		//it must be 0 ~ 11
		if ( lastmon > 0 ){
			lastmon -= 1;
		}else{
			lastmon = 11;
			lastyear -= 1;
		}
		char last_month_dir[LOG_PATH_LEN];
		char last_month_file[LOG_PATH_LEN];
		bzero(last_month_dir, sizeof(last_month_dir));
		bzero(last_month_file, sizeof(last_month_file));
		sprintf(last_month_dir, "%s%d%02d", logpath, lastyear+1900, lastmon+1);
		sprintf(last_month_file, "%s/%s%s", last_month_dir, "current.", logname);
		
		if ( flag_subdir_month && access(last_month_dir, F_OK) == 0 ){
			if ( access(last_month_file, F_OK) == 0 ){
				//file current.xx.log exist in last month directory, change name for it
				struct stat filestat;
				if ( stat(last_month_file, &filestat) == 0 ){
					rename_old_file_before_first_open(last_month_file, filestat.st_mtime);
				}else{
					perror("LogFile::stat() error");
					printf("logname = %s\n",logname);
				}
			}
		}
		
		if ( access(currentLogPathName, F_OK) == 0 ){
			//file current.xx.log exist already
			file0 = open(currentLogPathName, O_WRONLY|O_CREAT|O_APPEND, logshare);
			if ( file0 != -1 ){
				//open file successful
				struct stat filestat;
				if ( fstat(file0, &filestat) == 0 ){
					if ( filestat.st_size > 0 ){		//file length
						if ( !isSameDay(filestat.st_mtime, now) ){	//file modify time
							close(file0);
							file0 = -1;
							rename_old_file_before_first_open(currentLogPathName, filestat.st_mtime);
						}else{
							//CALCULATE "logline" if needed
							if ( loglinemax ){
								logline = get_file_lines(currentLogPathName);
								//printf("LogFile: file '%s' lines = %d when logopen()\n", currentLogPathName, logline);
							}
						}
					}
				}else{
					perror("LogFile::fstat() error");
					printf("logname = %s\n",logname);
					close(file0);
					file0 = -1;
				}
			}
		}
	//add end
	
	if ( file0 == -1 ){
		file0 = open(currentLogPathName, O_WRONLY|O_CREAT|O_APPEND, logshare);
	}
	
	if ( file0 != -1 ){
		fchmod(file0, logshare);
		whichfile = 0;
		plogfile = file0;
		fileopenflag = 1;
		return 1;
	}else{
		plogfile = -1;
		fileopenflag = 0;
		printf("\n\n logopen:logfile: open file %s error = %d!!!\n\n", currentLogPathName, errno);
		return 0;
	}
}//End of logopen()


int LogFile::SetLoglevel(int level)
{
	loglevel = level;
	logwrite("set log Level = %d\n",level);
	return 1;
}


int LogFile::logwrite(int level, int msglen, unsigned char * msgbuf)
{
	char * logbuffer1 = NULL;
	char * logbuffer2 = NULL;

	if (level > loglevel) return 1;

	if (prtflag == 0 && logflag == 0) return 1;

	//
	char * buflog = NULL;
	if ( (buflog = (char *)pthread_getspecific(dkey)) == NULL ){
		//malloc for this thread, only once
		buflog = (char *)malloc(LOG_BUFFER_SIZE*2);
		if ( buflog == NULL ) return 0;
		pthread_setspecific(dkey, buflog);
	}
	logbuffer1 = buflog;
	logbuffer2 = buflog + LOG_BUFFER_SIZE;
	bzero(logbuffer1, LOG_BUFFER_SIZE);
	bzero(logbuffer2, LOG_BUFFER_SIZE);
	
    //logbuffer2 NOT need here, so I can use logbuffer1+logbuffer2
    if ( LOG_BUFFER_SIZE*2 < (msglen*3 + msglen/16 + 3) ){
    	sprintf(logbuffer1, "LogFile: msglen is %d, bigger than logbuffer size, error\n\n", msglen);
    }
    else{
	    unsigned char * p = msgbuf;
	    char * q = logbuffer1;
	    for ( int j=1; j<=msglen; j++ ){
            sprintf(q, "%02x ", *p);
            p++;
            q += 3;
            if ( j%16 == 0 ){
            	*q = '\n';
            	q++;
            }
	    }
	    *q = '\n';
	    q++;
	    *q = '\n';
	    q++;
	    *q = 0;
	}
	
	return writing_file(logbuffer1);
}//end

int LogFile::logwrite(int level, const char* format, ...)
{
	char * logbuffer1 = NULL;
	char * logbuffer2 = NULL;
	time_t now;
	struct tm * ptm;
	char timebuf[64] = {0};

	if (level > loglevel) return 1;

	if (prtflag == 0 && logflag == 0) return 1;

	//
	char * buflog = NULL;
	if ( (buflog = (char *)pthread_getspecific(dkey)) == NULL ){
		//malloc for this thread, only once
		buflog = (char *)malloc(LOG_BUFFER_SIZE*2);
		if ( buflog == NULL ) return 0;
		pthread_setspecific(dkey, buflog);
	}
	logbuffer1 = buflog;
	logbuffer2 = buflog + LOG_BUFFER_SIZE;
	bzero(logbuffer1, LOG_BUFFER_SIZE);
	bzero(logbuffer2, LOG_BUFFER_SIZE);
	
	va_list marker;
	va_start(marker,format);
	(void) vsprintf(logbuffer1, format, marker);
	va_end(marker);
	
	time(&now);
	memset(timebuf,0,sizeof(timebuf));
	ptm = localtime(&now);

	if(timeflag == 0){
		strftime(timebuf,63,"%Y-%m-%d %H:%M:%S",ptm);
	}else if(timeflag == 1){
		strftime(timebuf,63,"%H:%M:%S",ptm);
	}

	//
	if ( logbuffer1[0]==' ' && logbuffer1[1]==0x00 ){
		return writing_file(logbuffer1);
	}
	
	int i;
	for (i=0; logbuffer1[i]=='\n'; i++) logbuffer2[i] = '\n';
	if (logbuffer1[i]) {
		sprintf(&logbuffer2[strlen(logbuffer2)], "[%s]    %d    %s", timebuf, level, &logbuffer1[i]);
	}
	
	return writing_file(logbuffer2);
}//end

int LogFile::logwrite(const char* format, ...)
{
	char * logbuffer1 = NULL;
	char * logbuffer2 = NULL;
	time_t now;
	struct tm * ptm;
	char timebuf[64] = {0};
	
	if (prtflag == 0 && logflag == 0) return 1;

	//
	char * buflog = NULL;
	if ( (buflog = (char *)pthread_getspecific(dkey)) == NULL ){
		//malloc for this thread, only once
		buflog = (char *)malloc(LOG_BUFFER_SIZE*2);
		if ( buflog == NULL ) return 0;
		pthread_setspecific(dkey, buflog);
	}
	logbuffer1 = buflog;
	logbuffer2 = buflog + LOG_BUFFER_SIZE;
	bzero(logbuffer1, LOG_BUFFER_SIZE);
	bzero(logbuffer2, LOG_BUFFER_SIZE);

	va_list marker;	
	va_start(marker,format);
	vsprintf(logbuffer1, format, marker);
	va_end(marker);
	
	time(&now);
	memset(timebuf,0,sizeof(timebuf));
	ptm = localtime(&now);

	if(timeflag == 0){
		strftime(timebuf,63,"%Y-%m-%d %H:%M:%S",ptm);
	}else if(timeflag == 1){
		strftime(timebuf,63,"%H:%M:%S",ptm);
	}

	int i;
	for (i=0; logbuffer1[i]=='\n'; i++) logbuffer2[i] = '\n';
	if (logbuffer1[i]) {
		if ( logtimeflag ) 
			sprintf(&logbuffer2[strlen(logbuffer2)], "[%s]    %s", timebuf, &logbuffer1[i]);
		else
			sprintf(&logbuffer2[strlen(logbuffer2)], "%s", &logbuffer1[i]);
	}
	
	return writing_file(logbuffer2);
}//end


// if flag=0,print enable; if flag=1,only log enable; if flag=2,both are enabled
int LogFile::enable(int flag)
{
	if(flag == 0){
		prtflag = 1;
	}else if(flag == 1){
		//if (fileopenflag) logflag = 1;		//
		logflag = 1;
	}else if(flag == 2){
		prtflag = 1;
		//if (fileopenflag) logflag = 1;		//
		logflag = 1;
	}
	return 1;
}

// if flag=0,print disable; if flag=1,only log disable; if flag=2,both are disabled
int LogFile::disable(int flag)
{
	if(flag == 0){
		prtflag = 0;
	}else if(flag == 1){
		logflag = 0;
	}else if(flag == 2){
		prtflag = 0;
		logflag = 0;
	}
	return 1;
}

int LogFile::startlog(int flag) 
{
	switch (flag) {
	case 0:
		prtflag = 0;
		logflag = 0;
		break;
	case 1:
		prtflag = 0;
		//if (fileopenflag) logflag = 1;		//
		logflag = 1;
		break;
	case 2:
		prtflag = 1;
		logflag = 0;
		break;
	case 3:
		prtflag = 1;
		//if (fileopenflag) logflag = 1;		//
		logflag = 1;
		break;
	default:
		prtflag = logflag = 0;
		break;
	}
	return 1;
}


//
int LogFile::SetLogSize(int size)
{ 
	//if ( size <= 500 * 1024 ) size = 500 * 1024;
	logsize = size; 
	return 1;
}

int LogFile::SetLogLine(int lines)
{
	if ( lines > 0 ){
		loglinemax = lines;
	}
	return 1;
}


int LogFile::IsDateChanged()
{
	time_t now;
	struct tm * ptm;
	
	time(&now);
	ptm = localtime(&now);
	
	if ( ptm->tm_mon != tm4file.tm_mon || ptm->tm_mday != tm4file.tm_mday ){
		return 1;
	}
	
	return 0;
}


void LogFile::logclose(int flag)
{
	switch ( flag )
	{
		case 0:
			//printf("  close file0 now\n");
			if ( file0 != -1 ) close(file0);
			file0 = -1;
			if ( file1 == -1 ) {
				plogfile = -1;
				printf("\nlogfile.cxx: close file0 but file1 is not open, error\n\n");
			}
			return;

		case 1:
			//printf("  close file1 now\n");
			if ( file1 != -1 ) close(file1);
			file1 = -1;
			if ( file0 == -1 ) {
				plogfile = -1;
				printf("\nlogfile.cxx: close file1 but file0 is not open, error\n\n");
			}
			return;
			
		default:
			plogfile = -1;
			logflag = 0;
			if (fileopenflag == 0) return;
			fileopenflag = 0;
			
			if ( file0 != -1 ) close(file0);
			file0 = -1;
			if ( file1 != -1 ) close(file1);
			file1 = -1;
			return;
	}
}//end of logclose()


int LogFile::SetLogTimeFlag(int flag)	//
{
	logtimeflag = flag;
	return 1;
}


int LogFile::logtime()					//
{
	time_t now;
	struct tm * ptm;
	char timebuf[64];
	char logbuffer[64] = {0};
	
	time(&now);
	memset(timebuf,0,sizeof(timebuf));
	ptm = localtime(&now);

	if(timeflag == 0){
		strftime(timebuf,63,"%Y-%m-%d %H:%M:%S",ptm);
	}else if(timeflag == 1){
		strftime(timebuf,63,"%H:%M:%S",ptm);
	}
	
	if( logflag && fileopenflag ){
		if ( plogfile != -1 ){
			bzero(logbuffer, sizeof(logbuffer));
			sprintf(logbuffer, "[%s]\n", timebuf);
			write(plogfile, logbuffer, strlen(logbuffer));
			#ifdef __FLUSHLOG__
				//fflush(plogfile);
			#endif
		}
	}
	
	if(prtflag == 1) printf("[%s]\n",timebuf);
	
	return 1;
}


int LogFile::writing_file(char * printbuf)
{
	if( logflag && fileopenflag ){					//
		checklog++;
		//check log file size first				//
		//if ( logsize && (checklog%LOG_FILE_CHECK_SIZE_PER_TIME == 0) ){
		if ( logsize || loglinemax ){
			checklog = 1;
			//user has set logsize, must check it
			//lock
			if ( (flag_multi_thread == 0) 
				|| (flag_multi_thread && pthread_mutex_trylock(&access_mutex) == 0) )
			{
				//if ( (  ( access(currentLogPathName, F_OK) < 0 ) 
				if ( (  ( flag_file_inode_changed = isFileInodeChanged(plogfile, currentLogPathName) ) 
						|| ( loglinemax > 0 && logline >= loglinemax ) 
						|| (loglen() >= logsize) 
						|| IsDateChanged()
					 ) 
					&& (!logtransfer) )
				{
					//flag_file_inode_changed must get a value before logChangeFile()
					logtransfer = 1;
					logflag = 0;
					//printf("file %s.%03d is full, will open next log file, whichfile = %d\n", logname, filefix, whichfile);
					//must open a new log file first
					if ( logChangeFile() ){	
						//printf("  file %s.%03d logChangeFile() ok, whichfile = %d\n", logname, filefix, whichfile);
						//logline = 0;
						logclose(((whichfile == 0) ? 1 : 0));
					}else{
						printf("LogFile: file %s.%03d logChangeFile() error, whichfile = %d\n", logname, filefix, whichfile);
					}
					logflag = 1;
					logtransfer = 0;
				}
				if ( flag_multi_thread ){
					//unlock
					pthread_mutex_unlock(&access_mutex);
				}
			}//end of trylock
		}
		//end of check log file size
		
		if ( printbuf[0]==' ' && printbuf[1]==0x00 ) return 1;
		
		if ( plogfile != -1 ){
			logline++;
			write(plogfile, printbuf, strlen(printbuf));
			#ifdef __FLUSHLOG__
				//fflush(plogfile);
			#endif
		}
	}

	if(prtflag == 1){
		if ( printbuf[0]==' ' && printbuf[1]==0x00 ){
			;
		}else
			printf("%s", printbuf);
	}
	
	return 1;
}//end


int LogFile::GetLogPathName(char * pathname)
{
	strcpy(pathname, currentLogPathName);
	return 1;
}


int LogFile::logChangeFile()
{
	time_t now;
	struct tm * ptm;
	char * pfilename;
	char filename[LOG_PATH_LEN+LOG_NAME_LEN]={0};
	unsigned int pathlen,namelen;
	
	//try to decide 'close file and open it again' or 'rename file and reopen file'
	if ( flag_file_inode_changed ){
		goto CHANGE_TO_NEXT_FILE;
	}
	/*
	if ( isFileInodeChanged(plogfile, currentLogPathName) ){
		goto CHANGE_TO_NEXT_FILE;
	}
	*/
	/*
	struct stat stat1, stat2;
	bzero(&stat1, sizeof(struct stat));
	bzero(&stat2, sizeof(struct stat));
	
	if (fileopenflag == 0) goto CHANGE_TO_NEXT_FILE;
	if ( whichfile == 0 ){
		if ( file0 == -1 ) goto CHANGE_TO_NEXT_FILE;
		if ( fstat(file0, &stat1) < 0 ){
			perror("LogFile: fstat() error");
			printf("logname = %s\n",logname);
			goto CHANGE_TO_NEXT_FILE;
		}
	}
	else if ( whichfile == 1 ){
		if ( file1 == -1 ) goto CHANGE_TO_NEXT_FILE;
		if ( fstat(file1, &stat1) < 0 ){
			perror("LogFile: fstat() error");
			printf("logname = %s\n",logname);
			goto CHANGE_TO_NEXT_FILE;
		}
	}
	else{
		printf("LogFile: logChangeFile() error because whichfile = %d\n", whichfile);
		goto CHANGE_TO_NEXT_FILE;
	}
	
	if ( stat(currentLogPathName, &stat2) < 0 ){
		//file not exist or other process are doing now
		//so 'close file and open it again'
		goto CHANGE_TO_NEXT_FILE;
	}
	
	if ( stat1.st_ino != stat2.st_ino ){
		//other process have done it
		//printf("LogFile: inode1 = %ld and inode2 = %ld, will not do rename()\n", stat1.st_ino, stat2.st_ino);
		goto CHANGE_TO_NEXT_FILE;
	}
	*/
	
	//rename file
	pathlen = strlen(logpath);
	namelen = strlen(logname);
	if (pathlen == 0 || namelen == 0) return 0;

	if ( logpath[pathlen-1] != '/' ){	//add '/' to the end of logpath if needed
		logpath[pathlen] = '/';	
		logpath[pathlen+1] = 0;	
	}
	pathlen = strlen(logpath);
	
	memset(filename,0,sizeof(filename));
	
	if ( flag_subdir_month ){
		char tmppath[LOG_PATH_LEN];
		bzero(tmppath, sizeof(tmppath));
		sprintf(tmppath, "%s%d%02d", logpath, tm4file.tm_year+1900, tm4file.tm_mon+1);
		sprintf(filename, "%s/%s", tmppath, logname);
		//mkdir() if needed
		mkdirectory(tmppath);
	}else{
		sprintf(filename, "%s%s", logpath, logname);
	}
	
	pfilename = &filename[strlen(filename)];
	sprintf(pfilename, ".%02d%02d.%03d", 
		tm4file.tm_mon+1, 
		tm4file.tm_mday,
		filefix);
	//now filename is the next log file for this class 
	
	if ( access(filename, F_OK) == 0 ){
		//file exist, try next
		do{
			filefix++;
			sprintf(pfilename, ".%02d%02d.%03d", 
				tm4file.tm_mon+1, 
				tm4file.tm_mday,
				filefix);
		}while ( access(filename, F_OK) == 0 );
		//file not exist now
	}
	
	rename(currentLogPathName, filename);
	//printf("LogFile: rename(%s, %s)\n", currentLogPathName, filename);
	
	filefix++;
	
  CHANGE_TO_NEXT_FILE:
	
	time(&now);
	ptm = localtime(&now);
	if ( ptm->tm_mon != tm4file.tm_mon || ptm->tm_mday != tm4file.tm_mday ){
		memcpy(&tm4file, ptm, sizeof(struct tm));
		filefix = 0;
	}
	
	if ( flag_subdir_month ){
		char tmppath[LOG_PATH_LEN];
		bzero(tmppath, sizeof(tmppath));
		sprintf(tmppath, "%s%d%02d", logpath, tm4file.tm_year+1900, tm4file.tm_mon+1);
		memset(currentLogPathName, 0, sizeof(currentLogPathName));
		sprintf(currentLogPathName, "%s/%s%s", tmppath, "current.", logname);
		//mkdir() if needed
		mkdirectory(tmppath);
	}
	
	if ( whichfile == 0 ){
		if ( file1 == -1 ){
			file1 = open(currentLogPathName, O_WRONLY|O_CREAT|O_APPEND, logshare);
		}else{
			printf("LogFile: logChangeFile() error because file1 = %d\n", file1);
		}
			
		if ( file1 != -1 ){
			fchmod(file1, logshare);
			logline = 0;
			plogfile = file1;
			whichfile = 1;
			fileopenflag = 1;
			return 1;
		}else{
			printf("\n\nlogfile: open file %s error = %d!!!\n\n", currentLogPathName, errno);
			plogfile = file0;
			whichfile = 0;
			fileopenflag = 1;
			return 0;
		}
	}
	else if ( whichfile == 1 ){
		if ( file0 == -1 ){
			file0 = open(currentLogPathName, O_WRONLY|O_CREAT|O_APPEND, logshare);
		}else{
			printf("LogFile: logChangeFile() error because file0 = %d\n", file0);
		}
		
		if ( file0 != -1 ){
			fchmod(file0, logshare);
			logline = 0;
			plogfile = file0;
			whichfile = 0;
			fileopenflag = 1;
			return 1;
		}else{
			printf("\n\nlogfile: open file %s error = %d!!!\n\n", currentLogPathName, errno);
			plogfile = file1;
			whichfile = 1;
			fileopenflag = 1;
			return 0;
		}
	}
	else{
		fileopenflag = 0;
		plogfile = -1;
		printf("\nlogfile: whichfile = %d, error\n", whichfile);
		return 0;
	}
}


int LogFile::SetSubdirMonth(int flag)
{
	flag_subdir_month = flag;
	return 1;
}


int LogFile::mkdirectory(char* path_name, char* childpath_name)
{
	char tmpDir[LOG_PATH_LEN];
	memset(tmpDir,0,sizeof(tmpDir));
	char Dir[LOG_PATH_LEN];
	memset(Dir,0,sizeof(Dir));
	strncpy(tmpDir,path_name,sizeof(tmpDir) - 1);
	if(tmpDir[strlen(tmpDir) - 1] == '/'){
		tmpDir[strlen(tmpDir) - 1] = '\0';
	}
	if ( access(tmpDir, F_OK) < 0 ) {
		if ( mkdir(tmpDir, 00777) < 0 ){
			char *fatherDir;
			char *tmpfatherDir;
			fatherDir = strstr(tmpDir,"/");
			if(fatherDir == NULL){
				return 0;
			}
			while(fatherDir!=NULL){
				tmpfatherDir = &fatherDir[1];	
				fatherDir = strstr(tmpfatherDir,"/");
			}
			strncpy(Dir,logpath,tmpfatherDir - tmpDir);
			mkdirectory(Dir,tmpDir);
			if(tmpDir == NULL){
			}
			else if ( access(tmpDir, F_OK) < 0 ) {
				if ( mkdir(tmpDir, 00777) < 0 ){
					return 0;
				}
				chmod(tmpDir, 00777);
			}
		} 
		else{
			chmod(tmpDir, 00777);
		}
	}
	return 1;

}
/*
int LogFile::mkdirectory(char * path_name)
{
	char *fatherDir;
	fatherDir = path_name;
	if ( access(path_name, F_OK) < 0 ) {
		if ( mkdir(path_name, 00777) < 0 ){
			char *fatherDir;
			fatherDir = strstr(path_name,"/");
			while(fatherDir!=NULL){
				fatherDir = strstr(fatherDir,"/");
			}
			perror("mkdir() error");
			printf("logname = %s\n",logname);
			return 0;
		}
		chmod(path_name, 00777);
	}
	return 1;
}
*/

int LogFile::SetMultiThreadFlag(int flag)
{
	flag_multi_thread = flag;
	return 1;
}



int LogFile::rename_old_file_before_first_open(char * current_filename, time_t tfile_modify)
{
	int filefix = 0;		//filefix is a local variable here
	
	struct tm mfile;
	memcpy(&mfile, localtime(&tfile_modify), sizeof(struct tm));
	
	char * pfilename;
	char filename[LOG_PATH_LEN+LOG_NAME_LEN]={0};
	unsigned int pathlen,namelen;
	
	//rename file
	pathlen = strlen(logpath);
	namelen = strlen(logname);
	if (pathlen == 0 || namelen == 0) return 0;

	if ( logpath[pathlen-1] != '/' ){	//add '/' to the end of logpath if needed
		logpath[pathlen] = '/';	
		logpath[pathlen+1] = 0;	
	}
	pathlen = strlen(logpath);
	
	memset(filename,0,sizeof(filename));
	
	if ( flag_subdir_month ){
		char tmppath[LOG_PATH_LEN];
		bzero(tmppath, sizeof(tmppath));
		sprintf(tmppath, "%s%d%02d", logpath, mfile.tm_year+1900, mfile.tm_mon+1);
		sprintf(filename, "%s/%s", tmppath, logname);
		//mkdir() if needed
		mkdirectory(tmppath);
	}else{
		sprintf(filename, "%s%s", logpath, logname);
	}
	
	pfilename = &filename[strlen(filename)];
	sprintf(pfilename, ".%02d%02d.%03d", 
		mfile.tm_mon+1, 
		mfile.tm_mday,
		filefix);
	
	if ( access(filename, F_OK) == 0 ){
		//file exist, try next
		do{
			filefix++;
			sprintf(pfilename, ".%02d%02d.%03d", 
				mfile.tm_mon+1, 
				mfile.tm_mday,
				filefix);
		}while ( access(filename, F_OK) == 0 );
		//file not exist now
	}
	
	rename(current_filename, filename);
	//printf("LogFile: rename(%s, %s)\n", currentLogPathName, filename);
	
	filefix++;		//this line is not harmful because it's local
	
	return 1;
}


//return 1 for yes
int LogFile::isSameDay(time_t t1, time_t t2)
{
	struct tm tm1;
	struct tm tm2;
	
	memcpy(&tm1, localtime(&t1), sizeof(struct tm));
	memcpy(&tm2, localtime(&t2), sizeof(struct tm));
	
	if ( tm1.tm_year == tm2.tm_year 
		&& tm1.tm_mon == tm2.tm_mon 
		&& tm1.tm_mday == tm2.tm_mday )
	{
		return 1;
	}
	
	return 0;
}


int LogFile::get_file_lines(char * file)
{
	char rbff[256] = {0};
	FILE * fp;
	char * retval = NULL;
	int trytimes = 0;
	int val1 = 0, val2, val3;
	struct timeval time_sleep;
		
	char cmd[128] = {0};
	sprintf(cmd, "cat %s | wc", file);

	if ((fp = popen(cmd, "r")) == NULL){
		perror("LogFile: popen() error");
		printf("logname = %s\n",logname);
		return 0;
	}
	
  TRY_GETTING_FILE_LINES:
	trytimes++;
	bzero(rbff, sizeof(rbff));
	retval = fgets(rbff, sizeof(rbff), fp);
	
	if ( retval == NULL ){
		//fgets() failed
		if ( trytimes < 3 && errno == EINTR ){		//Interrupted system call
			//wait a while
			time_sleep.tv_sec = 0;
			time_sleep.tv_usec = 100 * 1000;
			select(0, NULL, NULL, NULL, &time_sleep);
			//try again
			goto TRY_GETTING_FILE_LINES;
		}
		//
		perror("LogFile: fgets() error");
		printf("logname = %s\n",logname);
		pclose(fp);
		return 0;
	}

	sscanf(rbff, "%d %d %d", &val1, &val2, &val3);
	
	pclose(fp);
	
	return val1;
}


int LogFile::isFileInodeChanged(int fd, char * filename)
{
	struct stat stat1, stat2;
	bzero(&stat1, sizeof(struct stat));
	bzero(&stat2, sizeof(struct stat));
	
	if ( fd < 0 ) return 1;
	if ( fstat(fd, &stat1) < 0 ){
		perror("LogFile: fstat() error");
		printf("logname = %s\n",logname);
		return 1;
	}
	
	if ( stat(filename, &stat2) < 0 ){
		//file not exist or other error
		return 1;
	}
	
	if ( stat1.st_ino != stat2.st_ino ){
		//file inode changed
		return 1;
	}
	
	return 0;
}

