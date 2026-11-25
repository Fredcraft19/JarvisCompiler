#pragma once

#include "node.hpp"

#include <unordered_map>

class abstr{

private:

    static inline std::unordered_map<double, std::string> float_consts;

    // carelessly swipped from codegen.hpp 0_0
    template <typename T>
    static std::string str(T x){
        return std::to_string(x);
    }

    static std::string getFloatOperator(std::string& oper, variableData& data){
        std::string suffix;
        switch(data.totalBytes){
            case 4: suffix = "ss"; break;
            case 8: suffix = "sd"; break;
            default: return "";
        }
        if(oper == "+") return "add" + suffix;
        if(oper == "-") return "sub" + suffix;
        if(oper == "*") return "mul" + suffix;

        if(data::isBoolOperator(oper)) throw std::runtime_error("abstract codegen: boolean arithmetic and comparison is not supported for floats yet.");
        return "";
    }

    static std::string getIntOperator(std::string& oper, variableData& data){
        if(oper == "+") return "add";
        if(oper == "-") return "sub";
        if(oper == "*") return "imul";

        if(data::isBoolOperator(oper)) return "cmp";
        return "";
    }

    static std::string getUintOperator(std::string& oper, variableData& data){
        if(oper == "+") return "add";
        if(oper == "-") return "sub";
        if(oper == "*") return "imul";

        if(data::isBoolOperator(oper)) return "cmp";
        return "";
    }

public:

    static std::string getBooleanOperater(std::string& oper, variableData& type){
        if(data::assemblyBoolOperators.find(oper) != data::assemblyBoolOperators.end()){
            return data::assemblyBoolOperators[oper];
        }
        else{
            throw std::runtime_error("abstract codegen: unexpected operator when running 'getBooleanOperator' -> '" + oper + "'.");
        }
    }

    static std::string handleFullOperation(std::string& oper, variableData& type){
        std::string output = "";
        output += handleOperation(oper, type, true);

        if(output == "cmp"){
            output += " " + getRegister(type, true) + ", "; 
            output += getRegister(type, false) + "\n    ";

            output += getBooleanOperater(oper, type);
            output += " al";
        }
        else if(output != ""){
            output += " " + getRegister(type, true) + ", ";
            output += getRegister(type, false);
        }

        return output;
    }
    
    static std::string getDataType(size_t bytes){
        switch(bytes){
            case 1:
                return "byte";
            case 2:
                return "word";
            case 4:
                return "dword";
            case 8:
                return "qword";

            default:
                throw std::runtime_error("abstract codegen: only values, 1, 2, 4 and 8 are allowed for 'getDataType', instead of " + std::to_string(bytes));
        }
    }

    static std::string handleFloatDivision(variableData& type, uint64_t a_address, uint64_t b_address){
        std::string output = "";
        if(type.totalBytes == 8){
            output += "divsd xmm0, xmm1\n"; 
        }
        else if(type.totalBytes == 4){
            output += "divss xmm0, xmm1\n"; 
        }
        else{
            throw std::runtime_error("abstract codegen: unsupported variable size for division, only supports 32 and 64 bit floats");
        }

        return output;
    }
    static std::string handleIntDivision(variableData& type, uint64_t a_address, uint64_t b_address){
        std::string output = "";
        if(type.totalBytes == 8){
            output += "cqo\n";
            output += "    idiv rcx\n";
        }
        else if(type.totalBytes == 4){
            output += "cdq\n";
            output += "    idiv ecx\n";
        }
        else{
            throw std::runtime_error("abstract codegen: unsupported variable size for division, only supports 32 and 64 bit ints");
        }

        return output;
    }

    static std::string mov(variableData& data, std::string reg, std::string value){
        std::string output = "";

        if(data.dataType == variableType::Float) output += "movss";
        else output += "mov";

        output += " " + reg;

        output += ", " + value;

        return output;
    }

    static std::string getFloatConstant(std::string& name, double value){

        // its not janky.. i swear..

        std::string flt_name = "";
        if(float_consts.find(value) != float_consts.end()){
            name = float_consts[value];
            return "";
        }

        size_t index = float_consts.size();
        flt_name = "float_const_" + str(index);
        float_consts[value] = flt_name;
        name = flt_name;
        return flt_name + " dd " + str(value) + "\n";
    }

    static std::string handleUintDivision(variableData& type, uint64_t a_address, uint64_t b_address){
        std::string output = "";
        if(type.totalBytes == 8){
            output += "xor edx, edx\n";
            output += "    div rcx\n";
        }
        else if(type.totalBytes == 4){
            output += "xor edx, edx\n";
            output += "    div ecx\n";
        }
        else{
            throw std::runtime_error("abstract codegen: unsupported variable size for division, only supports 32 and 64 bit uints");
        }

        return output;
    }

    static std::string handleDivFlip(variableData& type){
        if(type.dataType == variableType::Int || type.dataType == variableType::Uint) {
            if(type.totalBytes == 8){
                return "mov rcx, rax";
            }
            else if(type.totalBytes == 4){
                return "mov ecx, eax";
            }
            else{
                throw std::runtime_error("abstract codegen: unsupported data type size for division, not32/64bit (div-flip)");
            }
        }
        else if(type.dataType == variableType::Float){
            if(type.totalBytes == 8){
                throw std::runtime_error("abstract codegen: unsupported data type float64 (div-flip)");
                return "mov xmm1, xmm0";
            }
            else if(type.totalBytes == 4){
                return "mov xmm1, xmm0";
            }
            else{
                throw std::runtime_error("abstract codegen: unsupported data type size for division, not32/64bit (div-flip)");
            }
        }
        else{
            throw std::runtime_error("abstract codegen: unsupported data type for division (div-flip)");
        }
    }

    static std::string handleDivision(variableData& type, uint64_t a_address, uint64_t b_address){
        bool isFloat = type.dataType == variableType::Float;
        bool isInt = type.dataType == variableType::Int;
        bool isUint = type.dataType == variableType::Uint;

        if(isFloat){
            return handleFloatDivision(type, a_address, b_address);
        }
        else if(isInt){
            return handleIntDivision(type, a_address, b_address);
        }
        else if(isUint){
            return handleUintDivision(type, a_address, b_address);
        }
        else{
            throw std::runtime_error("abstract codegen: not float, int or uint for handling division. Invalid type to use arithmetic on");
        }
    }
    
    static std::string handleOperation(std::string& oper, variableData& type, bool allowPassthrough = false){
        bool isFloat = type.dataType == variableType::Float;
        bool isInt = type.dataType == variableType::Int;
        bool isUint = type.dataType == variableType::Uint;

        if(isFloat){
            return getFloatOperator(oper, type);
        }
        else if(isInt){
            return getIntOperator(oper, type);
        }
        else if(isUint){
            return getUintOperator(oper, type);
        }
        else{
            if(allowPassthrough) return "";

            throw std::runtime_error("abstract codegen: unsupported data type that isnt float, int or uint. type: " + std::to_string(type.dataType));
        }
    }

    // Is Primary determins if its the primary or secondary register (eg: rax and rcx)
    // is Normal determins if the register is used for push/pop (not normal) or if its used normally, if its used abnormally then it will use rax and rcx for stack push/pops
    static std::string getRegister(variableData& data, bool isPrimary = true, bool isNormal = true){

        if(data.dataType == variableType::Float) return isPrimary ? "xmm0" : "xmm1";

        if(!isNormal){
            return isPrimary ? "rax" : "rcx";
        }
        
        switch(data.totalBytes){
            case 1: return isPrimary ? "al"  : "cl";
            case 2: return isPrimary ? "ax"  : "cx";
            case 4: return isPrimary ? "eax" : "ecx";
            case 8: return isPrimary ? "rax" : "rcx";
            default: throw std::runtime_error("abstract codegen: unsupported register size " + std::to_string(data.totalBytes));
        }
    }

    static std::string identifyerAddress(identifyerNode* id){
        auto& type = id->resolvedType;
        
        if(type.isReference){ 
            return "mov rax, [rbp - " + str(type.address) + "]";
        }
        else{ 
            return "lea rax, [rbp - " + str(type.address) + "]";
        }
    }

    static std::string getResize(variableData& from, variableData& to, variableType type){
        if(from.totalBytes == to.totalBytes) return "";
        if(from.totalBytes > to.totalBytes) return "";

        std::string target = getRegister(to, true); // to true!! hahaha.. that joke is to true.. haha..
        std::string original = getRegister(from, true);

        if(type == variableType::Int){
            if(from.totalBytes == 4) {return "movsxd " + target + ", " + original;}
            else {return "movsx " + target + ", " + original;}
        }
        else if(type == variableType::Uint){
            if(from.totalBytes == 4) {return "mov eax, eax";}
            else {return "movzx " + target + ", " + original;}
        }
        else if(type == variableType::Float){
            throw std::runtime_error("abstract codegen: float resizing not supported");
        }
        else{
            throw std::runtime_error("abstract codegen: unsupported type was attempted to be resized");
        }
    }

};