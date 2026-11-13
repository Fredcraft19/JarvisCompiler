#pragma once

#include <unordered_map>
#include <string>
#include <vector>
#include <cstdint>

enum variableType{
    Int,
    Uint,
    Float,

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



class data{
public:

    static inline std::vector<std::string> keywords = {
        
    };

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

        
        {"void",        variableData(0, Void)}
    };

    static inline std::unordered_map<std::string, objectData> objects = {};
};