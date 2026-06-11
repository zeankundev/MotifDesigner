#ifndef LOGGER_H
#define LOGGER_H
#include <iostream>
class Logger {
    public:
        static void log(const char* message) {
            std::cout << "[dbg] " << message << std::endl;
        };
};
#endif
