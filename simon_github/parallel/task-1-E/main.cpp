#include <iostream>
#include "treemutex.h"


int main()
{
    size_t num_threads = 5;
    TreeMutex mtx(num_threads);
    mtx.lock(0);
    // critical section
    mtx.unlock(0);
}

