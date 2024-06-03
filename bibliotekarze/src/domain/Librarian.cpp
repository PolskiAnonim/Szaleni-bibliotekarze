#include "Librarian.h"

void Librarian::simulationLoop() {
    while (true) {
        handleState();
    }
}

void Librarian::IDLE() {
    print("Wchodzi do " + STATE::to_string(STATE::State::IDLE));
    std::this_thread::sleep_for(
            std::chrono::seconds(RNG::rng.getRandomValue()));

    commThread->syncStart();

    STATE::toNextState();
}

void Librarian::WAIT_MPC() {
    print("Wchodzi do " + STATE::to_string(STATE::State::WAIT_MPC));
    //Choose MPC
    {
        auto it = std::min_element(MPC::accessQueue.begin(), MPC::accessQueue.end());
        MPC::chosen = distance(MPC::accessQueue.begin(), it);
        ++MPC::accessQueue[MPC::chosen];
    }
    //Create message
    Communication::lastRequestLamport = ++Communication::lamportClock;
    Communication::Message message(MPC::chosen, NOT_USED, Communication::lastRequestLamport, NOT_USED, Communication::MessageType::REQ_MPC);
    //Send to all...
    MPIHandler::sendMessageToAll(message);

    Communication::acksToRcv = MPIHandler::getB()-1;

    commThread->syncEnd();
    //Waiting for MPC
    commThread->LibrarianWaitForMPC();

    STATE::toNextState();

    commThread->syncEnd();

}

void Librarian::IN_SECTION_MPC() {
    print("Wchodzi do sekcji krytycznej MPC: " + std::to_string(MPC::chosen)+" Pozostałe użycia: "+std::to_string(MPC::repairGauge[MPC::chosen].getCount()));
    int number = RNG::rng.getRandomValue();
    for (int x = 0; x < number; x++) {
        --MPC::repairGauge[MPC::chosen];
        std::this_thread::sleep_for(std::chrono::seconds(1L));
    }
    print("Wychodzi z sekcji krytycznej MPC: " + std::to_string(MPC::chosen)+" Pozostałe użycia: "+std::to_string(MPC::repairGauge[MPC::chosen].getCount()));

    std::this_thread::sleep_for(std::chrono::seconds(1));
    STATE::toNextState();
    //IN SECTION MPC-> IDLE
    commThread->syncStart();
    //FIN
    Communication::Message fin(MPC::chosen, MPC::repairGauge[MPC::chosen].getCount(),
                               ++Communication::lamportClock, NOT_USED, Communication::MessageType::FIN_MPC);
    MPIHandler::sendMessageToAll(fin);
    //Delete from queue
    --MPC::accessQueue[MPC::chosen];
    MPC::chosen = -1;

    commThread->syncEnd();
}


void Librarian::WAIT_SVC() {
    print("Wchodzi do "+STATE::to_string(STATE::State::WAIT_SVC));

    //Create message
    Communication::lastRequestLamport = ++Communication::lamportClock;
    Communication::Message message(MPC::chosen, NOT_USED, Communication::lastRequestLamport, NOT_USED, Communication::MessageType::REQ_SVC);
    //Send to all...
    MPIHandler::sendMessageToAll(message);

    Communication::acksToRcv = MPIHandler::getB()-MPC::S;

    commThread->syncEnd();

    commThread->LibrarianWaitForMPC();
}


void Librarian::IN_SECTION_SVC() {
    print("Wchodzi do sekcji krytycznej serwisanta MPC: "+std::to_string(MPC::chosen));
    MPC::repairGauge[MPC::chosen].fix();
    std::this_thread::sleep_for(std::chrono::seconds(2L));
    print("Wychodzi z sekcji krytycznej serwisanta MPC: "+std::to_string(MPC::chosen));

    std::this_thread::sleep_for(std::chrono::seconds(1));
}

void Librarian::handleSvc() {
    commThread->syncStart();
    //To WAIT_SVC
    STATE::toNextStateMalfunction();

    handleState();  //WAIT_SVC

    //To IN_SECTION_SVC
    STATE::toNextState();
    commThread->syncEnd();

    handleState();  //IN_SECTION_SVC
    //To IN_SECTION_MPC

    commThread->syncStart();

    STATE::toNextState();

    //Send back ACKS
    while (!Communication::svcQueue.empty()) {
        Communication::Message msg = Communication::svcQueue.top();
        Communication::svcQueue.pop();
        msg.transformToAck(++Communication::lamportClock);
        MPIHandler::sendMessage(msg);
    }
    commThread->syncEnd();
}

