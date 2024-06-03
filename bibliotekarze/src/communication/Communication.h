#pragma once

#include <iostream>
#include <unordered_map>
#include <queue>
#include <vector>
#include <atomic>

#define NOT_USED (-1)

namespace Communication {   //YAY same inline
    inline std::atomic_int64_t lamportClock=0;  //:< :>

    inline int64_t lastRequestLamport=0;

    inline int acksToRcv=0;

    inline static void updateClockAfterRcv(int64_t messageClock) {
        int64_t lamport = lamportClock;
        lamportClock = std::max(lamport, messageClock) + 1;
    }

    enum MessageType {
        REQ_MPC = 0,
        ACK_MPC,
        FIN_MPC,
        REQ_SVC,
        ACK_SVC
    };

    struct Request {
        int64_t mpcNo;
        int64_t uses_left;
        int64_t lamportClock;

        Request(int64_t d, int64_t d2, int64_t l) : mpcNo(d), uses_left(d2), lamportClock(l) {};

        Request() : mpcNo(NOT_USED), uses_left(NOT_USED), lamportClock(NOT_USED) {};
    };

    struct Message {
    private:
        std::unordered_map<MessageType, MessageType> acknowledgements{
                {MessageType::REQ_MPC, MessageType::ACK_MPC},
                {MessageType::REQ_SVC, MessageType::ACK_SVC}};
    public:
        Request request;
        int fromTo;
        MessageType messageType;

        Message(Request req, int frt, MessageType type) : request(req), fromTo(frt), messageType(type) {};

        Message(int64_t mpcNo, int64_t uses_left, int64_t lamport, int fromTo, MessageType type) :
        request(mpcNo, uses_left, lamport), fromTo(fromTo), messageType(type) {};

        Message() : request(), fromTo(NOT_USED), messageType(MessageType::ACK_SVC) {};

        void transformToAck(long lamport) {
            request.lamportClock = lamport;
            messageType = acknowledgements[messageType];
        }

        void changeDest(int dest) {
            fromTo = dest;
        }
    };

    struct MessageCompare {
        bool operator()(const Message &a, const Message &b) {
            if (a.request.lamportClock > b.request.lamportClock)
                return true;
            else if (a.request.lamportClock < b.request.lamportClock)
                return false;
            else if (a.fromTo > b.fromTo)
                return true;
            else
                return false;
        }
    };

    using SvcQueue = std::priority_queue<Message, std::vector<Message>, MessageCompare>;
    inline SvcQueue svcQueue={};
}
