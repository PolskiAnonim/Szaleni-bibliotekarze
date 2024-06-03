#include "CommunicationThread.h"

void CommunicationThread::MPCReady() {
    {
        std::lock_guard lk(mutexLibrarian);
        possibleToGetMPC = true;
    }
    cvLibrarian.notify_one();

    //possibleToGetMPC.notify_all(); //C++20...

}


void CommunicationThread::IDLE() {
    //Messages types and actions
    //Requests
    if (message.messageType == Communication::MessageType::REQ_SVC
        || message.messageType == Communication::MessageType::REQ_MPC) {
        if (message.messageType == Communication::MessageType::REQ_MPC) {
            ++MPC::accessQueue[message.request.mpcNo];
        }
        //Send ACK
        message.transformToAck(++Communication::lamportClock);
        MPIHandler::sendMessage(message);
    }
        //Fin
    else if (message.messageType == Communication::MessageType::FIN_MPC) {
        MPC::repairGauge[message.request.mpcNo].updateCount(message.request.uses_left);
        --MPC::accessQueue[message.request.mpcNo];
    }
    //Acks ignored
}

void CommunicationThread::WAIT_MPC() {
    //Receive message and update clock
    //Reqests
    if (message.messageType == Communication::MessageType::REQ_SVC) {
        //Send ack
        message.transformToAck(++Communication::lamportClock);
        MPIHandler::sendMessage(message);
    } else if (message.messageType == Communication::MessageType::REQ_MPC) {
        ++MPC::accessQueue[message.request.mpcNo];
        //if process wants another MPC or lamport clock in message says that request from message is older
        if (MPC::chosen != message.request.mpcNo ||
            MPIHandler::compareSendAck(Communication::lastRequestLamport, message)) {
            //Send ack
            message.transformToAck(++Communication::lamportClock);
            MPIHandler::sendMessage(message);
        }
    }
        //Ack/fin
    else if (message.messageType == Communication::MessageType::ACK_MPC) {
        //print("dostałem acka od " + std::to_string(message.fromTo));
        //Acknowledge acknowledgment
        --Communication::acksToRcv;
    } else if (message.messageType == Communication::MessageType::FIN_MPC) {
        --MPC::accessQueue[message.request.mpcNo];//delete from queue
        MPC::repairGauge[message.request.mpcNo].updateCount(message.request.uses_left);
        if (message.request.mpcNo == MPC::chosen) {//Acknowledge acknowledgment
            //print("dostałem fina od " + std::to_string(message.fromTo) + " " +
            //     std::to_string(message.request.uses_left));
            --Communication::acksToRcv;
        }
    }
    //ACK SVC ignored
    if (Communication::acksToRcv == 0) {
        syncStart();
        MPCReady();
    }
}

void CommunicationThread::IN_SECTION_MPC() {
    //Requests
    if (message.messageType == Communication::MessageType::REQ_SVC) {
        //Send ack
        message.transformToAck(++Communication::lamportClock);
        MPIHandler::sendMessage(message);
    }
    else if (message.messageType == Communication::MessageType::REQ_MPC) {
        ++MPC::accessQueue[message.request.mpcNo];
        //if process in message wants another MPC
        if (MPC::chosen != message.request.mpcNo) {
            //Send ack
            message.transformToAck(++Communication::lamportClock);
            MPIHandler::sendMessage(message);
        }
    }
    //FIN
    else if (message.messageType == Communication::MessageType::FIN_MPC) {
        --MPC::accessQueue[message.request.mpcNo];
        MPC::repairGauge[message.request.mpcNo].updateCount(message.request.uses_left);
    }
    //ACKS ignored
}

void CommunicationThread::WAIT_SVC() {
    //Requests
    if (message.messageType == Communication::MessageType::REQ_SVC) {
        if (MPIHandler::compareSendAck(Communication::lastRequestLamport, message)) {
            //Send ack
            message.transformToAck(++Communication::lamportClock);
            MPIHandler::sendMessage(message);
        } else
            Communication::svcQueue.push(message);//Add to queue to send after exit

    } else if (message.messageType == Communication::MessageType::REQ_MPC) {
        ++MPC::accessQueue[message.request.mpcNo];
        //if process in message wants another MPC
        if (MPC::chosen != message.request.mpcNo) {
            //Send ack
            message.transformToAck(++Communication::lamportClock);
            MPIHandler::sendMessage(message);
        }
    }
        //FIN
    else if (message.messageType == Communication::MessageType::FIN_MPC) {
        --MPC::accessQueue[message.request.mpcNo];
        MPC::repairGauge[message.request.mpcNo].updateCount(message.request.uses_left);
    }
        //ACK
    else if (message.messageType == Communication::MessageType::ACK_SVC) {
        --Communication::acksToRcv;
    }
    //ACK MPC ignored

    if (Communication::acksToRcv <= 0) {
        syncStart();
        MPCReady();
        //possibleToGetMPC.notify_all();    //C++20
    }
}
void CommunicationThread::IN_SECTION_SVC() {
    //Requests
    if (message.messageType == Communication::MessageType::REQ_SVC) {
        //push to queue
        Communication::svcQueue.push(message);
    } else if (message.messageType == Communication::MessageType::REQ_MPC) {
        ++MPC::accessQueue[message.request.mpcNo];
        //if process in message wants another MPC
        if (MPC::chosen != message.request.mpcNo) {
            //Send ack
            message.transformToAck(++Communication::lamportClock);
            MPIHandler::sendMessage(message);
        }
    }
        //FIN
    else if (message.messageType == Communication::MessageType::FIN_MPC) {
        --MPC::accessQueue[message.request.mpcNo];
        MPC::repairGauge[message.request.mpcNo].updateCount(message.request.uses_left);
    }
    //ACK MPC/SVC ignored
}

void CommunicationThread::loop() {
    while(true) {

        //Receive message and update LamportClock
        message = MPIHandler::receiveMessage();   //AAAAAAAA

        Communication::updateClockAfterRcv(message.request.lamportClock);
        //Synchronization if needed
        {
            std::unique_lock lk(mutex);
            cv.wait(lk, [this] { return ready; });
        }
        handleState();
    }
}

void CommunicationThread::syncStart() {
    ready = false;
}

void CommunicationThread::syncEnd() {
    {
        std::lock_guard lk(mutex);
        ready = true;
    }
    cv.notify_one();
}

void CommunicationThread::LibrarianWaitForMPC() {
    {
        std::unique_lock lk(mutexLibrarian);
        cvLibrarian.wait(lk, [this] { return possibleToGetMPC; });
    }
        //possibleToGetMPC.wait(false);
        possibleToGetMPC=false;
}


