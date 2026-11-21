#pragma once

#include <string>
#include <cstdlib>
#include <filesystem>
#include <chrono>
#include <cmath>

#include "tokeniser.hpp"
#include "parser.hpp"
#include "analyser.hpp"
#include "codegen.hpp"

class compiler{
private:
    static std::string read(std::string fileName){
        std::ifstream file(fileName);
        std::string source = "";
        std::string line;
        while(std::getline(file, line)){
            source += line;
        }
        return source;
    }
    static void write(std::string fileName, std::string& text){

        std::ofstream file(fileName);

        if(!file.is_open()) throw std::runtime_error("Failed to open file '"+fileName+"'\n");

        file << text;

        file.close();

    }

public:
    // uses all layers of the compiler compiling all
    // source code in one simple method.
    // returns assembly (86x_64 NASM)
    static std::string compile_to_nasm(std::string source, bool log = false){
        using clock = std::chrono::steady_clock;

        using us = std::chrono::duration<double, std::micro>;

        auto t0 = clock::now();

        if(log) std::cout << "Tokenising\n";
        auto tokens = tok::tokenise(source, log);
        auto t1 = clock::now();

        if(log){ for(size_t i = 0; i < tokens.size(); i++){
            std::cout << tokens[i].string() << "\n";
        }}

        if(log) std::cout << "Parsing\n";
        auto t2 = clock::now();
        auto nodes = parser::parse(tokens, log);
        auto t3 = clock::now();

        if(log) std::cout << "Analysing\n";
        auto t4 = clock::now();
        auto& method_data = analyser::analyse(nodes, log);
        auto t5 = clock::now();

        if(log) std::cout << "Generating Code\n";
        auto t6 = clock::now();
        std::string assembly = codegen::generate(nodes, method_data, log);
        auto t7 = clock::now();

        double tokenTime    = us(t1 - t0).count();
        double parseTime    = us(t3 - t2).count();
        double analyseTime  = us(t5 - t4).count();
        double codegenTime  = us(t7 - t6).count();
        double total = tokenTime + parseTime + analyseTime + codegenTime;

        auto fmt = [](double v){ return std::round(v * 1000.0) / 1000.0; };

        std::cout << "Tokenising: "         << fmt(tokenTime)   << "us\n"
                << "Parsing: "            << fmt(parseTime)   << "us\n"
                << "Analysation: "        << fmt(analyseTime) << "us\n"
                << "Code Generation: "    << fmt(codegenTime) << "us\n"
                << "Total Compilation: "  << fmt(total)       << "us\n";

        return assembly;
    }

    static bool assemble(std::string asmFile, std::string exeFile){
        std::string objFile = "output.obj";

        std::string Nasm = "nasm -f win64 \"" + asmFile + "\" -o " + objFile; 
        if(std::system(Nasm.c_str()) != 0){
            throw std::runtime_error("nasm failed");
            return false; // yknow bro.
        }

        std::string Gcc = "g++ " + objFile + " -o \"" + exeFile + "\"";
        if(std::system(Gcc.c_str()) != 0){
            throw std::runtime_error("g++ failed");
            return false;   // useful code..
        }

        std::filesystem::remove(objFile);

        return true;
    }

    static void compile(std::string sourceFile, std::string outputFile,bool log= false, bool autoClearUp = true){
        std::string assembly = compile_to_nasm(read(sourceFile), log);
        std::string assemblyFile = "output.nasm";
        write(assemblyFile, assembly);

        assemble(assemblyFile, outputFile);

        if(autoClearUp) {std::filesystem::remove(assemblyFile);}
    }

    static void compile_and_run(std::string sourceFile, bool log = false, bool autoClearUp = true){
        compile(sourceFile, "jit_produce.exe",log,  autoClearUp);
        int stat = std::system("jit_produce.exe");
        std::cout << "program exited with code:\n" << std::to_string(stat) << "\n";

        if(autoClearUp){std::filesystem::remove("jit_produce.exe");}
    }

    
};