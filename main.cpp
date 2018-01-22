#include <iostream>
#include "rotating_file_sink_archive.h"

using namespace std;

// # TESTS
// Prepare:
// $ rm -Rf ./logs/*
// Run & check:
// $ zlib-flate -uncompress < mylogfile.txt_archive/mylogfile.txt_<curdate_and_time>_3.zlib
// [<curdate_and_time>] [some_logger_name] [info] 2
// $ zlib-flate -uncompress < mylogfile.txt_archive/mylogfile.txt_<curdate_and_time>.zlib
// [<curdate_and_time>] [some_logger_name] [info] 3
// $ cat mylogfile.3.txt
// [<curdate_and_time>] [some_logger_name] [info] 4
// $ cat mylogfile.2.txt
// [<curdate_and_time>] [some_logger_name] [info] 5
// $ cat mylogfile.1.txt
// [<curdate_and_time>] [some_logger_name] [info] 6
// $ cat mylogfile.txt
// [<curdate_and_time>] [some_logger_name] [info] 7
//
int main(int, char*[])
{
    std::cout << "Started tests..." << std::endl;
    try
    {
        auto rotating_logger = spdlog::create<spdlog::sinks::rotating_file_sink_archive_mt>("some_logger_name", "logs/mylogfile.txt", 100, 3, 1, 1);

        int counter = 0;
        for (int i = 0; i < 5; ++i)
            rotating_logger->info("{}", ++counter);

        std::this_thread::sleep_for(std::chrono::milliseconds(3000));

        for (int i = 0; i < 2; ++i)
            rotating_logger->info("{}", ++counter);

        // Release and close all loggers
        spdlog::drop_all();
    }
    // Exceptions will only be thrown upon failed logger or sink construction (not during logging)
    catch (const spdlog::spdlog_ex& ex)
    {
        std::cout << "Log init failed: " << ex.what() << std::endl;
        return 1;
    }
    std::cout << "Done. \n Note: Check the results in 'logs' dir." << std::endl;

    return 0;
}

