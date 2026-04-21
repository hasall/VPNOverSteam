#include "lib/SteamMessageProcessor.h"
#include "lib/DebugLog.h"

void SteamMessageProcessor::OnConnectionRequested(SteamNetworkingMessagesSessionRequest_t* request) {
    DebugLog("AcceptSession\n");
    SteamAPI_ISteamNetworkingMessages_AcceptSessionWithUser(SteamAPI_SteamGameServerNetworkingMessages_SteamAPI(), request->m_identityRemote);
}
void SteamMessageProcessor::OnConnectionFailed(SteamNetworkingMessagesSessionFailed_t* request) {
    DebugLog("FailSession\n");
}

SteamMessageProcessor::SteamMessageProcessor(
    CallbackReceiveData messageReceiver,
    CallbackReceiveData systemMessageReceiver
): 
    messageReceiver(messageReceiver),
    systemMessageReceiver(systemMessageReceiver),
    running(false)
{
    DebugLog("SteamMessageProcessor::SteamMessageProcessor\n");
}

SteamMessageProcessor::~SteamMessageProcessor() {
    this->Stop();
}

void SteamMessageProcessor::Start() {
    DebugLog("SteamMessageProcessor::Start\n");
    if (this->running == false) {
        this->running = true;
        this->runMessageThread = std::thread(&SteamMessageProcessor::ReceiveDataLoop, this, MessageChannel, this->messageReceiver);
        this->runSystemMessageThread = std::thread(&SteamMessageProcessor::ReceiveDataLoop, this, SystemMessageChannel, this->systemMessageReceiver);
    }
    else {
        DebugLog("SteamMessageProcessor::Start: already running\n");
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
        SteamAPI_SteamGameServerNetworkingMessages_SteamAPI(),
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
    while (this->running == true) {
        SteamNetworkingMessage_t* messages[10];

        int num = SteamAPI_ISteamNetworkingMessages_ReceiveMessagesOnChannel(SteamAPI_SteamGameServerNetworkingMessages_SteamAPI(), channel, messages, 10);

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
                    if (callback) {
                        callback(msg->m_identityPeer.GetSteamID(), (char*)msg->m_pData, msg->m_cbSize);
                    }
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
