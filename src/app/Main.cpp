#include <cstdlib>
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#else
#include <unistd.h>
#endif

int ManualMain();
int AutomaticMain(int argc, char* argv[]);

bool IsRunningAsAdmin() {
#ifdef _WIN32
	BOOL fIsRunAsAdmin = FALSE;
    PSID pAdminSid = NULL;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;

    // Initialize the SID for the Administrators group
    if (AllocateAndInitializeSid(&NtAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID, 
                                 DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &pAdminSid)) {
        // Check if the token is a member of the Admin group
        if (!CheckTokenMembership(NULL, pAdminSid, &fIsRunAsAdmin)) {
            fIsRunAsAdmin = FALSE;
        }
        FreeSid(pAdminSid);
    }
    return fIsRunAsAdmin;
#else
	return geteuid() == 0;
#endif
}

int main(int argc, char* argv[])
{
    // #ifndef _WIN32
    // std::system("echo main");
    // std::system("echo $(pwd)");
    // std::system("echo LD_LIBRARY_PATH: $LD_LIBRARY_PATH");
    // std::system("echo STEAM_CLIENT_PATH: $STEAM_CLIENT_PATH");
    // std::system("export LD_LIBRARY_PATH=./:$LD_LIBRARY_PATH");
    // std::system("export STEAM_CLIENT_PATH=./");
    // std::system("echo LD_LIBRARY_PATH: $LD_LIBRARY_PATH");
    // std::system("echo STEAM_CLIENT_PATH: $STEAM_CLIENT_PATH");
    // std::system("echo end main");
    // #endif

	if (!IsRunningAsAdmin()) {
		std::cerr << "This application must be run with administrator privileges." << std::endl;
		return EXIT_FAILURE;
	}
	// If command-line arguments are provided, run in automatic mode; otherwise, run in manual mode
	if (argc > 1) {
		return AutomaticMain(argc, argv);
	}

	return ManualMain();
}