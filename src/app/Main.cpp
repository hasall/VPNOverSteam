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

void TUNTest();
int main(int argc, char* argv[])
{
	if (!IsRunningAsAdmin()) {
		std::cerr << "This application must be run with administrator privileges." << std::endl;
		return EXIT_FAILURE;
	}

    // TUNTest();
    // return 0;
    
	// If command-line arguments are provided, run in automatic mode; otherwise, run in manual mode
	if (argc > 1) {
		return AutomaticMain(argc, argv);
	}

	return ManualMain();
}

// #include "lib/TUNLinux.h"
// #include "lib/Config.h"
// #include "lib/Utils.h"

// void TUNTest() {
//     TUNLinux tun([](const char* message, size_t size) {
//         std::cout << "Received message: " << std::string(message, size) << std::endl;
//     });

//     tun.Start(Utils::FromString(Config::ServerIp));


// 	std::cout << "Press Enter to continue..." << std::endl;
//     std::cin.get(); // Waits for a single Enter key press
// }