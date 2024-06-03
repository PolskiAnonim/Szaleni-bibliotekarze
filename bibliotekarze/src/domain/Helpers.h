#pragma once

#include <random>
#include <vector>
#include <unordered_map>
#include <mutex>
#include "MPIHandler.h"

class Librarian;

#define VERBOSE true

namespace MPC {
    class Counter {
    protected:
        int64_t count;
        int64_t initial_count;
        std::function<void()> callback;

    public:
        Counter(int64_t initial_count, const std::function<void()> &callback)
                : count(initial_count), initial_count(initial_count), callback(callback){};

        void updateCount(int64_t c) {
            count = c;
        }

        Counter &operator--() {
            if (--count == 0) {
                callback();
                //fix();
            }
            return *this;
        }

        int64_t getCount() {
            return count;
        }

        void fix() {
            count = initial_count;
        }

    };

    inline int64_t S;

    using MpcAccessQueue = std::vector<int64_t>;
    using MpcRepairGauge = std::vector<Counter>;

    inline MpcAccessQueue accessQueue;
    inline MpcRepairGauge repairGauge;

    inline int64_t chosen=0;

    void createQueues(Librarian *lib, int64_t m, int64_t k);
}

namespace RNG {
    class RNG {
    public:
        RNG() : mt(rd()), uint_dist30(1, 5) {}

        uint32_t getRandomValue() {
            return uint_dist30(mt);
        }

    private:
        std::random_device rd;
        std::mt19937 mt;
        std::uniform_int_distribution<uint32_t> uint_dist30;
    };

    inline static RNG rng;
}

namespace STATE {

    static std::string getColorCode(int64_t rank) {
        static const std::vector<int> distinguishableColors = {
                160, 196, 202, 208, 214, 220, 226, 190, 154, 118, 82, 46, 47, 48, 49, 50,
                51, 87, 123, 159, 195, 231, 201, 165, 129, 93, 57, 21, 27, 33, 39, 45
        };

        int colorIndex = rank % distinguishableColors.size();

        int colorCode = distinguishableColors[colorIndex];

        return "\033[38;5;" + std::to_string(colorCode) + "m";
    }

    const std::string RESET = "\033[0m";

    enum class State {
        IDLE = 0,
        WAIT_MPC,
        IN_SECTION_MPC,
        WAIT_SVC,
        IN_SECTION_SVC,
    };

    static inline const std::string to_string(State state) {
        switch (state) {
            case State::IDLE:
                return "IDLE";
            case State::WAIT_MPC:
                return "WAIT_MPC";
            case State::IN_SECTION_MPC:
                return "IN_SECTION_MPC";
            case State::WAIT_SVC:
                return "WAIT_SVC";
            case State::IN_SECTION_SVC:
                return "IN_SECTION_SVC";
            default:
                return "UNKNOWN";
        }
    }

    // W namespace żeby nie nadpisać
    static inline const std::unordered_map<State, State> stateTransitions{
            {State::IDLE, State::WAIT_MPC},
            {State::WAIT_MPC, State::IN_SECTION_MPC},
            {State::IN_SECTION_MPC, State::IDLE},
            {State::WAIT_SVC, State::IN_SECTION_SVC},
            {State::IN_SECTION_SVC, State::IN_SECTION_MPC}};

    static std::mutex stateMutex=std::mutex();
    inline State currentState=State::IDLE;

    inline void toNextState() {
        std::lock_guard lock(stateMutex);
        STATE::currentState = stateTransitions.at(currentState);
    }

    inline void toNextStateMalfunction() {
        std::lock_guard lock(stateMutex);
        currentState = State::WAIT_SVC;
    }

    inline STATE::State getState() {
        std::lock_guard lock(stateMutex);
        return currentState;
    }

}

/* statable | /ˈsteɪtəbl/
 *  Capable of being stated.
 *  // bo według języka angielskiego coś takiego jak stateAble nie istnieje
 */
class IStatable {
    using StateHandler = void (IStatable::*)();
protected:
    virtual void IDLE() = 0;
    virtual void WAIT_MPC() = 0;
    virtual void IN_SECTION_MPC() = 0;
    virtual void WAIT_SVC() = 0;
    virtual void IN_SECTION_SVC() = 0;
    virtual ~IStatable() = default;

    std::unordered_map<STATE::State, StateHandler> stateHandlers{
            {STATE::State::IDLE, &IStatable::IDLE},
            {STATE::State::WAIT_MPC, &IStatable::WAIT_MPC},
            {STATE::State::IN_SECTION_MPC, &IStatable::IN_SECTION_MPC},
            {STATE::State::WAIT_SVC, &IStatable::WAIT_SVC},
            {STATE::State::IN_SECTION_SVC, &IStatable::IN_SECTION_SVC}};

    void handleState() {
        (this->*(stateHandlers.at(STATE::getState())))();
    }

public:
    static void print(const std::string& msg) {
        if (VERBOSE) {
            auto rank = MPIHandler::getRank();
            printf("%s%li [t%li] %s%s\n",STATE::getColorCode(rank).data(),rank,Communication::lamportClock.load(),msg.data(),STATE::RESET.data());
        }
    }
};