#include "appsettings.h"

AppSettings::AppSettings()
    : QSettings("QtAppSettings", "6502Assembler")
{

}

AppSettings& settings()
{
    static AppSettings s;
    return s;
}
