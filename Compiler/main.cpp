#include <iostream>
#include <fstream>
#include <string>

#include "jc/compiler.hpp"


int main(){
    std::cout << "Compiling Source Code at 'source.jarv'\n";
    
    compiler::compile_and_run("source.jarv", false, false);

    // wait until 'enter' then close. not instantly..
    std::string x;
    std::cin >> x;

    return 0;
}









