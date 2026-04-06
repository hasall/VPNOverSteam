#include <thread>

#include "lib/TUNMock.h"
#include "lib/DebugLog.h"

static TUNMock* mock;
TUNMock* getMock() {
	return mock;
}

TUNMock::TUNMock(TUNMessageReceiver receiver) : 
	receiver(receiver),
	testCallback(nullptr)
{
	DebugLog("TUNMock selected\n");
	mock = this;
}

void TUNMock::Start() {}
void TUNMock::Stop() {}

void TUNMock::SendData(const char* message, size_t size) {
    DebugLog("TUNMock::SendData\n");
	DebugLogArr(message, size);

	if (this->testCallback) {
		this->testCallback(message, size);
	}
}

void TUNMock::Receiver(const char* message, size_t size) {
	std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	try {
		DebugLog("TUNMock::Receiver\n");
		DebugLogArr(message, size);
		this->receiver(message, size);
	}
	catch (const std::exception& ex) {
		DebugLog("TUNMock::Receiver: exception in receiver: %s\n", ex.what());
	}
	catch (...) {
		DebugLog("TUNMock::Receiver: unknown exception in receiver\n");
	}
}