#include "lib/SteamMessageProcessor.h"
#include "lib/DebugLog.h"

void SteamMessageProcessor::OnConnectionRequested(SteamNetworkingMessagesSessionRequest_t* request) {
    DebugLog("AcceptSession\n");
    SteamAPI_ISteamNetworkingMessages_AcceptSessionWithUser(SteamNetworkingMessages(), request->m_identityRemote);
}
void SteamMessageProcessor::OnConnectionFailed(SteamNetworkingMessagesSessionFailed_t* request) {
    DebugLog("FailSession\n");
}

SteamMessageProcessor::SteamMessageProcessor(
    CallbackReceiveData messageReceiver,
    CallbackReceiveData systemMessageReceiver
): 
    messageReceiver(messageReceiver),
    systemMessageReceiver(systemMessageReceiver)
{}

SteamMessageProcessor::~SteamMessageProcessor() {
    this->Stop();
}

void SteamMessageProcessor::Start() {
    DebugLog("SteamMessageProcessor::Start\n");
    if (!this->running) {
        this->runMessageThread = std::thread(&SteamMessageProcessor::ReceiveDataLoop, this, MessageChannel, this->messageReceiver);
        this->runSystemMessageThread = std::thread(&SteamMessageProcessor::ReceiveDataLoop, this, SystemMessageChannel, this->systemMessageReceiver);
    }
}

void SteamMessageProcessor::Stop() {
    DebugLog("SteamMessageProcessor::Stop\n");
    this->running = false;
    if (this->runMessageThread.joinable()) {
        this->runMessageThread.join();
    }
    if (this->runSystemMessageThread.joinable()) {
        this->runSystemMessageThread.join();
    }
}

void SteamMessageProcessor::SendMessage(SteamNetworkingIdentity userId, const char* data, size_t size) {
    DebugLog("SteamMessageProcessor::SendMessage\n");
    this->SendData(userId, data, size, MessageChannel);
}
void SteamMessageProcessor::SendSystemMessage(SteamNetworkingIdentity userId, const char* data, size_t size) {
    DebugLog("SteamMessageProcessor::SendSystemMessage\n");
    this->SendData(userId, data, size, SystemMessageChannel);
}

void SteamMessageProcessor::SendData(SteamNetworkingIdentity userId, const char* data, size_t size, int channel) {
    DebugLog("SteamMessageProcessor::SendData\n");
    DebugLogArr(data, size);
    EResult res = SteamAPI_ISteamNetworkingMessages_SendMessageToUser(
        SteamNetworkingMessages(),
        userId,
        data,
        (uint32)size,
        k_nSteamNetworkingSend_Reliable,
        channel
    );

    if (res != k_EResultOK) {
        DebugLog("SteamMessageProcessor::SendData Send failed: %u\n", res);
    }
}

void SteamMessageProcessor::ReceiveDataLoop(int channel, CallbackReceiveData callback) {
    this->running = true;
    while (this->running) {
        SteamNetworkingMessage_t* messages[10];

        int num = SteamAPI_ISteamNetworkingMessages_ReceiveMessagesOnChannel(SteamNetworkingMessages(), channel, messages, 10);

        for (int i = 0; i < num; ++i) {
            SteamNetworkingMessage_t* msg = messages[i];

            if (msg == nullptr) {
                continue;
            }

            try {
                DebugLog("SteamMessageProcessor::ReceiveDataLoop [RECV] From %llu : %u\n", msg->m_identityPeer.GetSteamID().ConvertToUint64(), channel);

                if (msg->m_pData == nullptr || msg->m_cbSize == 0) {
                    DebugLog("SteamMessageProcessor::ReceiveDataLoop: empty message from %llu\n", msg->m_identityPeer.GetSteamID().ConvertToUint64());
                } else {
                    DebugLogArr(((uint8_t*)msg->m_pData), msg->m_cbSize);
                    callback(msg->m_identityPeer.GetSteamID(), (char*)msg->m_pData, msg->m_cbSize);
                }
            }
            catch (const std::exception& ex) {
                DebugLog("SteamMessageProcessor::ReceiveDataLoop: exception in callback: %s\n", ex.what());
            }
            catch (...) {
                DebugLog("SteamMessageProcessor::ReceiveDataLoop: unknown exception in callback\n");
            }

            msg->Release();
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}
