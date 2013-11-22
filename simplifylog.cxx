#include "logfile.h"
#include "simplifylog.h"

namespace SimplifyLog {
inline LogFile* getLogInstance()
{
    static LogFile log;
    return &log;
}

int initLog(char* path, char* filename)
{
    LogFile* log = getLogInstance();
    log->SetLogPathName(path, filename);
    log->SetLogSize(2*1024*1024);
    log->SetLoglevel(6);
    log->SetLogLine(1000);
    log->startlog(1);
    return log->logopen();

}

int setLogLevel(int level)
{
    return getLogInstance()->SetLoglevel(level);
}

int setLogSize(int size)
{
    return getLogInstance()->SetLogSize(size);
}

void closeLog(int flag)
{
    getLogInstance()->logclose();
}

}
