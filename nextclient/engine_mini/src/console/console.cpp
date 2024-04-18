#include "console.h"

#include <fstream>
#include <common.h>

#include "../common/filesystem.h"
#include "../common/sys_dll.h"

bool con_debuglog;

void Con_DebugLog(const char* file, const char* format, ...)
{
    static char text[8192];

    va_list params;
    va_start(params, format);
    V_vsnprintf(text, sizeof(text), format, params);
    va_end(params);

    std::ofstream stream(file, std::ios::app | std::ios::out);
    if (stream.is_open())
        stream.write(text, V_strlen(text));
}

void Con_Init()
{
    con_debuglog = COM_CheckParm("-condebug");
    if (con_debuglog)
        FS_RemoveFile("qconsole.log", nullptr);
}

void Con_DPrintf(ConLogType type, const char* format, ...)
{
    if (developer->value == 0.0 ||
        *p_scr_con_current != 0.0 && cls->state == ca_active)
        return;

    char text[4096];

    va_list params;
    va_start(params, format);
    V_vsnprintf(text, sizeof(text), format, params);
    va_end(params);

    Sys_Printf("%s", text);

    if (con_debuglog)
        Con_DebugLog("qconsole.log", "%s", text);

    if (type == ConLogType::Info)
    {
        g_GameConsole->Printf("%s", text);
    }
    else
    {
        const Color& color = g_LogTypeColors[(int)type];
        g_GameConsoleNext->ColorPrintf(color.r(), color.g(), color.b(), "%s", text);
    }
}
