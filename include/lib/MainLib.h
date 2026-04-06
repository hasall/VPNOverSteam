#ifndef MAINLIB_H
#define MAINLIB_H

#ifdef _WIN32
    #ifdef VPN_OVER_STEAM_EXPORTS
        #define VPN_OVER_STEAM_EXPORTS_API __declspec(dllexport)
    #else
        #define VPN_OVER_STEAM_EXPORTS_API __declspec(dllimport)
    #endif
#else
    #define VPN_OVER_STEAM_EXPORTS_API
#endif

VPN_OVER_STEAM_EXPORTS_API int add(int a, int b);

#endif // MAINLIB_H
