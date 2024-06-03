#include "domain/Librarian.h"
#include <cstdint>
#include <exception>
#include <iostream>

int main(int argc, char **argv) {
    try {


	    int64_t m = std::stoll(argv[1]);    //MPC number
	    MPC::S = std::stoll(argv[2]);       //SVC number
        int64_t k= std::stoll(argv[3]);      //Readers possible to be handled before malfunction
        MPIHandler::initMPI(argc, argv);
        auto* librarian = new Librarian();
        auto* commThread= new CommunicationThread();
        std::thread thread(&CommunicationThread::loop, commThread); //:>
        MPC::createQueues(librarian,m,k);
        librarian->init(commThread);

    } catch (const std::exception &e) {
	std::cout<<"Argumenty: M S K"<<std::endl;
    std::cout<<"{Ilość MPC} {Ilość serwisantów} {Ilość użyć MPC przed awarią}"<<std::endl;
    }
    MPIHandler::finalizeMPI();
}


void MPC::createQueues(Librarian *lib, int64_t m, int64_t k) {
    accessQueue = MpcAccessQueue(m, 0);
    for (int i = 0; i != m; i++) {
        //Create MPCs
        repairGauge.emplace_back(k, [lib] { lib->handleSvc(); });
    }
}