#pragma once

#include "Helpers.h"
#include "communication/Communication.h"

#include "CommunicationThread.h"


#include <chrono>
#include <thread>


class Librarian : public IStatable {
    CommunicationThread* commThread;
    void IDLE() override;

    void WAIT_MPC() override;

    void IN_SECTION_MPC() override;

    void WAIT_SVC() override;

    void IN_SECTION_SVC() override;


    [[noreturn]] void simulationLoop();
public:
    void handleSvc();

    void init(CommunicationThread* cThread) {
        this->commThread = cThread;
        simulationLoop();
    }


};

