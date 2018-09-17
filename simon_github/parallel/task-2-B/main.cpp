#include <iostream>
#include "robot_on_cond_vars.h"
#include <thread>


const size_t steps = 100;
int main() {
    Robot robot;
    std::thread left_thread([&robot] {
        for (size_t i = 0; i < steps; ++i)
            robot.StepLeft();
    });
    std::thread right_thread([&robot] {
        for (size_t i = 0; i < steps; ++i)
            robot.StepRight();
    });
    left_thread.join();
    right_thread.join();
    return 0;
}
