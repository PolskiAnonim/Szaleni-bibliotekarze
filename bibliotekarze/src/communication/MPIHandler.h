#pragma once

#include "Communication.h"

#include <cstdint>
#include <functional>
#include <mpi.h>
#include <queue>
#include <stdexcept>
#include <unordered_map>
#include <vector>

class MPIHandler {
private:
    static void initDatatype();
    static int rank;
    static MPI_Datatype MPI_REQUEST;
    static int B;

public:
    static void finalizeMPI();
    static void initMPI(int argc, char **argv);
    static Communication::Message receiveMessage();
    static void sendMessageToAll(Communication::Message &message);
    static void sendMessage(Communication::Message &message);
    static bool compareSendAck(int64_t lamport, Communication::Message &message);
    static int64_t getRank();
    static int getB();
};

