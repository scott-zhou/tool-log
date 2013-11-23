#include "logfile.h"
#include "simplifylog.h"

int main(void)
{
    using namespace SimplifyLog;
    char filename[] = "test.log";
    initLog("./", filename);
    logWrite(LOGDEBUG, "Log without parameter.");
    LogWrite(LOGDEBUG, "Log with parameter, in log [file %s].", filename);
    closeLog();
    return 0;
}
