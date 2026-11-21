#pragma once

#include <unordered_map>
#include <string>
#include <vector>
#include <cstdint>

enum variableType{
    Int,
    Uint,
    Float,

    Bool,
    Void,

    Error
};

struct variableData{
    size_t totalBytes;
    variableType dataType;
    uint64_t address = 0;
    variableData(size_t bytes, variableType type){
        totalBytes = bytes; 
        dataType = type;    
    }
    variableData(){
        totalBytes = 0;
        dataType = variableType::Error;
    }
};

struct objectData{
    size_t totalBytes;
    // to be added onto with classes later?
};

struct parameterData{
    std::string name;
    variableData typeData;
    uint64_t address;
};

struct methodData{
    std::string name;
    std::vector<parameterData> paremeters;
    variableData returnType;

    uint64_t currentAddress = 32;
    uint64_t maxStackSize = 32;
    std::unordered_map<std::string, variableData> scope;
    methodData* parentMethod = nullptr;
    bool hasVariable(std::string& vname){
        if(scope.find(vname) != scope.end()){
            return true;    
        }
        else if(parentMethod != nullptr){
            return parentMethod->hasVariable(vname);
        }
        else{
            return false;
        }
    }
    variableData getVariable(std::string& vname){
        if(scope.find(vname) != scope.end()){
            return scope[vname];    
        }
        else if(parentMethod != nullptr){
            return parentMethod->getVariable(vname);
        }
        else{
            throw std::runtime_error("variable '" + vname + "' does not exist in scope of method '" + name + "'");
        }
    }

    void addVariable(std::string vname, variableData data){
        if(hasVariable(vname)) throw std::runtime_error("variable of name '" + vname + "' already exists in scope of method '" + name + "'");

        scope[vname] = data;
    }
};

class data{
public:

    static inline std::vector<std::string> keywords = {
        "if",
        "else",
        "while",
        "for",

        "return"
    };

    static inline std::vector<std::string> boolOperators = {
        "==",
        "!=",
        "<",
        "<=",
        ">",
        ">="
    };

    // i know i can just refit the vector above for this purpose but i really cant be asked
    // to re-write like 5 lines of code.
    static inline std::unordered_map<std::string, std::string> assemblyBoolOperators = {
        {"==",   "sete"},
        {"!=",   "setne"},
        {"<",    "setl"},
        {"<=",   "setle"},
        {">",    "setg"},
        {">=",   "setge"}
    };

    static inline bool isBoolOperator(std::string& oper){
        for(auto& o : boolOperators){
            if(oper == o){
                return true;
            }
        }
        return false;
    }

    static inline std::unordered_map<std::string, variableData> primatives = {
        {"int8",        variableData(1,Int)},
        {"int16",       variableData(2,Int)},
        {"int32",       variableData(4,Int)},
        {"int64",       variableData(8,Int)},

        {"uint8",       variableData(1,Uint)},
        {"uint16",      variableData(2,Uint)},
        {"uint32",      variableData(4,Uint)},
        {"uint64",      variableData(8,Uint)},

        {"float32",     variableData(4,Float)},
        {"float64",     variableData(8,Float)},

        // short, easy to type shortcut type names
        {"int",         variableData(4, Int)},
        {"uint",        variableData(4, Uint)},
        {"float",       variableData(4, Float)},


        {"bool",        variableData(1, Int)},
        
        {"void",        variableData(0, Void)}
    };

    static inline std::unordered_map<std::string, objectData> objects = {};
};