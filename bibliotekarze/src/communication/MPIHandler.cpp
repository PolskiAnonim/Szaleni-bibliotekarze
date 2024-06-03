#include "MPIHandler.h"

int MPIHandler::rank;
MPI_Datatype MPIHandler::MPI_REQUEST;
int MPIHandler::B;

int64_t MPIHandler::getRank() {
    return rank;
}

void MPIHandler::finalizeMPI() {
    MPI_Type_free(&MPI_REQUEST);
    MPI_Finalize();
}

void MPIHandler::initDatatype() {
    int block_lengths[] = {1, 1, 1};
    MPI_Datatype types[] = {MPI_INT64_T, MPI_INT64_T, MPI_INT64_T};
    MPI_Aint displacements[] = {
	    offsetof(Communication::Request, mpcNo),
	    offsetof(Communication::Request, uses_left),
	    offsetof(Communication::Request, lamportClock)};

    MPI_Type_create_struct(3, block_lengths, displacements, types, &MPI_REQUEST);
    MPI_Type_commit(&MPI_REQUEST);
}

void MPIHandler::initMPI(int argc, char **argv) {
    int provided;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);

    initDatatype();

    MPI_Comm_size(MPI_COMM_WORLD, &B);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
}


Communication::Message MPIHandler::receiveMessage() {   //Mam dość
    Communication::Request request{};
    MPI_Status status;
    MPI_Recv(&request, 1, MPI_REQUEST, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

    return {request, status.MPI_SOURCE, Communication::MessageType(status.MPI_TAG)};
}

void MPIHandler::sendMessageToAll(Communication::Message &message) {
    for (int i=0;i<B;++i) {
        message.changeDest(i);
        if (i != rank)
            sendMessage(message);
    }
}

void MPIHandler::sendMessage(Communication::Message &message) {
	MPI_Send(&(message.request), 1, MPI_REQUEST, message.fromTo, message.messageType, MPI_COMM_WORLD);
}

bool MPIHandler::compareSendAck(int64_t lamport, Communication::Message &message) {
    //if process lamport clock is newer/younger (pogubię się)
    if (lamport > message.request.lamportClock)
	return true;
    else if (lamport < message.request.lamportClock)
	return false;
    else if (message.fromTo > rank)
	return false;
    else
	return true;
}

int MPIHandler::getB() {
    return B;
}
