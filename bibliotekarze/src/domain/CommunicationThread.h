#pragma once

#include "Helpers.h"

#include "communication/Communication.h"
#include "communication/MPIHandler.h"

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <memory>
#include <optional>
#include <thread>
#include <condition_variable>

//SvcQueue svcQueue;
// Forward declaration - i nie - nie chce pisać di


class CommunicationThread : public IStatable {
private:
    //For Librarian
    bool possibleToGetMPC=false;

    Communication::Message message;

    void IDLE() override;

    void WAIT_MPC() override;

    void WAIT_SVC() override;

    void IN_SECTION_SVC() override;

    void IN_SECTION_MPC() override;

    void MPCReady();


public:
    std::mutex mutex; //NIENAWIDZĘ C++17 :<<
    std::condition_variable cv;
    std::mutex mutexLibrarian;
    std::condition_variable cvLibrarian;

    bool ready = false;

    [[noreturn]] void loop();

    void LibrarianWaitForMPC();

    void syncStart();
    void syncEnd();


};
