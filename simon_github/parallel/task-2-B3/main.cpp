#include <iostream>
#include "robot_with_n_legs.h"
#include <thread>

void RunRobot(const std::size_t num_foots, const std::size_t num_steps) {
    Robot centipede{num_foots};

    std::vector<std::thread> foot_threads;

    for (size_t foot = 0; foot < num_foots; ++foot) {
        foot_threads.emplace_back(
            [&, foot, num_steps]() {
                for (size_t step = 0; step < num_steps; ++step) {
                    centipede.Step(foot);
                }
            }
        );
    }

    for (auto& t : foot_threads) {
        t.join();
    }
}
int main()
{
    RunRobot(5,10);
    return 0;
}

