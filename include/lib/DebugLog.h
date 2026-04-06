#ifndef DEBUGLOG_H
#define DEBUGLOG_H

#include <string>

char* GetDebugBuffer1();
char* GetDebugBuffer2();
int GetDebugBuffer1Size();
int GetDebugBuffer2Size();

#ifdef _WIN32
#define DebugLog(f, ...) \
    do { \
        sprintf_s(GetDebugBuffer1(), GetDebugBuffer1Size(), f, ##__VA_ARGS__); \
        sprintf_s(GetDebugBuffer2(), GetDebugBuffer2Size(), "D[%d]: %s", __LINE__, GetDebugBuffer1()); \
        printf(GetDebugBuffer2()); \
    } while (0)
#else
#define DebugLog(f, ...) \
    do { \
        snprintf(GetDebugBuffer1(), GetDebugBuffer1Size(), f, ##__VA_ARGS__); \
        snprintf(GetDebugBuffer2(), GetDebugBuffer2Size(), "D[%d]: %s", __LINE__, GetDebugBuffer1()); \
        printf("%s", GetDebugBuffer2()); \
    } while (0)
#endif

#define DebugLogArr(message, size) \
    do { \
        for (size_t i = 0; i < size; i++) { \
            printf("%u ", message[i]); \
        } printf("\n"); \
    } while (0)

#endif // DEBUGLOG_H
