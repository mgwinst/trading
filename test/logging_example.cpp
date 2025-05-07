#include "include/logger.h"

using namespace Common;

int main() {
    
    char c = 'd';
    int i = 3;
    unsigned long ul = 65;
    float f = 3.4;
    double d = 34.56;
    const char* s = "test C-string";
    std::string ss = "test string";

    Logger logger("logging_example.log");
    
    logger.log("Logging a char:% an int:% and an unsigned int:%\n", c, i, ul);
    

}