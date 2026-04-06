#ifdef _WIN32

#include <wintun.h>
#include <exception>
#include <stdexcept>
#include <iphlpapi.h>

#include "lib/TUNWindows.h"
#include "lib/TUN.h"

#include "lib/DebugLog.h"
#include "lib/Utils.h"

#pragma comment(lib, "Iphlpapi.lib")
#pragma comment(lib, "Ws2_32.lib")

static WINTUN_CREATE_ADAPTER_FUNC* WintunCreateAdapter;
static WINTUN_CLOSE_ADAPTER_FUNC* WintunCloseAdapter;
static WINTUN_OPEN_ADAPTER_FUNC* WintunOpenAdapter;
static WINTUN_GET_ADAPTER_LUID_FUNC* WintunGetAdapterLUID;
static WINTUN_GET_RUNNING_DRIVER_VERSION_FUNC* WintunGetRunningDriverVersion;
static WINTUN_DELETE_DRIVER_FUNC* WintunDeleteDriver;
static WINTUN_SET_LOGGER_FUNC* WintunSetLogger;
static WINTUN_START_SESSION_FUNC* WintunStartSession;
static WINTUN_END_SESSION_FUNC* WintunEndSession;
static WINTUN_GET_READ_WAIT_EVENT_FUNC* WintunGetReadWaitEvent;
static WINTUN_RECEIVE_PACKET_FUNC* WintunReceivePacket;
static WINTUN_RELEASE_RECEIVE_PACKET_FUNC* WintunReleaseReceivePacket;
static WINTUN_ALLOCATE_SEND_PACKET_FUNC* WintunAllocateSendPacket;
static WINTUN_SEND_PACKET_FUNC* WintunSendPacket;

static DWORD LastError;


TUNWindows::TUNWindows(TUNMessageReceiver receiver, GUID guid) : 
    receiver(receiver),
	Adapter(NULL),
	Session(NULL),
    Guid(guid) {
	DebugLog("TUNWindows selected\n");
	this->Wintun = TUNWindows::InitializeWintun();
    if (!this->Wintun) {
        return;
    }
    this->CreateAdapter();
}

void TUNWindows::Start(uint32_t ip) {
	DebugLog("Start: ip %s\n", Utils::ToString(ip).c_str());

	this->SetupAdapter(ip);
	this->SetupSession();
    
	// start receiver thread
    if (!this->running)
        this->receiverThread = std::thread(&TUNWindows::Receiver, this);
}

void TUNWindows::Stop() {
	this->running = false;
    if (this->receiverThread.joinable())
		this->receiverThread.join();
    if (this->Session) WintunEndSession(this->Session);
}

void TUNWindows::SendData(const char* message, size_t size) { 
    BYTE* packet = WintunAllocateSendPacket(this->Session, size);
    if (!packet) {
        DebugLog("Failed to WintunAllocateSendPacket %lu\n", GetLastError());
        return;
    }

    memcpy(packet, message, size);

    WintunSendPacket(this->Session, packet);
}

void TUNWindows::Receiver() {
	this->running = true;
    while (this->running) {
        try {
            DWORD PacketSize;
            BYTE* Packet = WintunReceivePacket(Session, &PacketSize);
            if (Packet) {
                this->receiver((const char*)Packet, PacketSize);
                WintunReleaseReceivePacket(Session, Packet);
            }
            else {
                auto LastError = GetLastError();
                switch (LastError) {
                case ERROR_NO_MORE_ITEMS:
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    continue;
                default:
                    DebugLog("Failed to WintunReceivePacket %lu\n", LastError);
                    return;
                }
            }
        }
        catch (std::exception &e) {
            DebugLog("Receiver thread exception: %s\n", e.what());
        }
        catch (...) {
            DebugLog("Receiver thread unknown exception\n");
        }
    }
}

HMODULE TUNWindows::InitializeWintun() {
	DebugLog("Initializing Wintun\n");
    HMODULE Wintun =
        LoadLibraryExW(L"wintun.dll", NULL, LOAD_LIBRARY_SEARCH_APPLICATION_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (!Wintun) {
        DebugLog("Failed to LoadLibraryExW %lu\n", GetLastError());
        return NULL;
    }

#define X(Name) ((*(FARPROC *)&Name = GetProcAddress(Wintun, #Name)) == NULL)
    if (X(WintunCreateAdapter) || X(WintunCloseAdapter) || X(WintunOpenAdapter) || X(WintunGetAdapterLUID) ||
        X(WintunGetRunningDriverVersion) || X(WintunDeleteDriver) || X(WintunSetLogger) || X(WintunStartSession) ||
        X(WintunEndSession) || X(WintunGetReadWaitEvent) || X(WintunReceivePacket) || X(WintunReleaseReceivePacket) ||
        X(WintunAllocateSendPacket) || X(WintunSendPacket))
#undef X

    {
        DebugLog("Failed to GetProcAddress %lu\n", GetLastError());
        FreeLibrary(Wintun);
        return NULL;
    }
    return Wintun;
}

void TUNWindows::CreateAdapter() {
	DebugLog("Creating adapter\n");
	Adapter = WintunCreateAdapter(L"VPNOverSteam", L"Wintun", &Guid);
    if (!Adapter)
    {
        DebugLog("Failed to CreateAdapter %lu\n", GetLastError());
        this->CleanupAdapter();
        return;
    }
    DWORD Version = WintunGetRunningDriverVersion();
	DebugLog("Wintun version: %u.%u\n", (Version >> 16) & 0xff, (Version >> 0) & 0xff);
}

void TUNWindows::SetupAdapter(uint32_t ip) {
	DebugLog("Setting up adapter\n");
    MIB_UNICASTIPADDRESS_ROW AddressRow;
    InitializeUnicastIpAddressEntry(&AddressRow);
    WintunGetAdapterLUID(Adapter, &AddressRow.InterfaceLuid);
    AddressRow.Address.Ipv4.sin_family = AF_INET;
    AddressRow.Address.Ipv4.sin_addr.S_un.S_addr = ip;
    AddressRow.OnLinkPrefixLength = 24; /* This is a /24 network */
    AddressRow.DadState = IpDadStatePreferred;
    LastError = CreateUnicastIpAddressEntry(&AddressRow);
    if (LastError != ERROR_SUCCESS) {
        DebugLog("Failed to CreateUnicastIpAddressEntry %lu\n", GetLastError());
    }
}

void TUNWindows::SetupSession() {
	DebugLog("Setting up session\n");
    this->Session = WintunStartSession(Adapter, 0x400000);
    if (!this->Session) {
        DebugLog("Failed to WintunStartSession %lu\n", GetLastError());
    }
}

void TUNWindows::CleanupAdapter() {
	DebugLog("Cleaning up adapter\n");
    if (this->Session) {
        WintunEndSession(this->Session);
        this->Session = NULL;
    }
    if (this->Adapter) {
        WintunCloseAdapter(this->Adapter);
        this->Adapter = NULL;
    }
    if (this->Wintun) {
        FreeLibrary(this->Wintun);
        this->Wintun = NULL;
    }
}

/*
cleanupWorkers:
    HaveQuit = TRUE;
    SetEvent(QuitEvent);
    for (size_t i = 0; i < _countof(Workers); ++i)
    {
        if (Workers[i])
        {
            WaitForSingleObject(Workers[i], INFINITE);
            CloseHandle(Workers[i]);
        }
    }
    WintunEndSession(Session);
cleanupAdapter:
    WintunCloseAdapter(Adapter);
cleanupQuit:
    SetConsoleCtrlHandler(CtrlHandler, FALSE);
    CloseHandle(QuitEvent);
cleanupWintun:
    FreeLibrary(Wintun);
    return LastError;
*/

#endif // _WIN32
