/**
  * To make it more simple to use the LogFile class.
  */

#ifndef __SIMPLIFYLOG_H__
#define __SIMPLIFYLOG_H__


#define	LOGDEBUG	5
#define LOGWARN		4
#define LOGMAJOR	3
#define LOGMINOR	2
#define LOGCRITICAL	1


#define logWrite(level,format) getLogInstance()->logwrite(level,"%s:%d:%s "format,__FILE__,__LINE__,__FUNCTION__)
#define LogWrite(level,format,...)	getLogInstance()->logwrite(level,"%s:%d:%s "format,__FILE__,__LINE__,__FUNCTION__,__VA_ARGS__)

class LogFile;

namespace SimplifyLog {
    inline LogFile* getLogInstance();
    int initLog(char* path, char* filename);
    int setLogLevel(int);
    int setLogSize(int);
    void closeLog(int flag = 9);
}

#endif
