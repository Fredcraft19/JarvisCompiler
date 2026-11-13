#pragma once

#include <iostream>

#include <vector>
#include <string>
#include <memory>

#include "node.hpp"
#include "abstract_nasm.hpp"

class codegen{
private:
    static inline std::string header;
    static inline std::string data_;

    static inline std::string text;
    static inline bool tabbed;

    static inline bool logs;

    static void print(std::string x){
        if(logs) std::cout << x << "\n";
    }

    // helper: quicker way to type std::to_string(x)
    template <typename T>
    static std::string str(T x){
        return std::to_string(x);
    }

    static void emit(std::string _nasm){
        if(tabbed) text+="    ";
        text+=_nasm + "\n";
    }
    static void label(std::string _lbl){
        text+= _lbl + ":\n";
    }

    static void data(std::string newData, bool addSpace = true){
        if(addSpace) data_ += "    ";
        data_ += newData + "\n";
    }

    static void pushFloat(bool isPrimary = true){
        emit("sub rsp, 8");

        if(isPrimary) emit("movss [rsp], xmm0");
        else emit("movss [rsp], xmm1");
    }
    static void popFloat(bool isPrimary = true){
        if(isPrimary) emit("movss xmm0, [rsp]");
        else emit("movss xmm1, [rsp]");

        emit("add rsp, 8");
    }


    // isPrimary determins if it should prefer the primary or secondary register of its type
    // for example:
    // 45 + 5
    // 45 would be primary (rax) as its left
    // 5 would be secondary (rcx) as its right
    static void generateExpression(expressionNode* expr, bool isPrimary) {
        if (!expr) {
            throw std::runtime_error("codegen: null expression node encountered");
        }

        if (auto* lit = dynamic_cast<literalNode*>(expr)) {
            print("    literal node");
            std::visit([&](auto&& val) {
                if(lit->resolvedType.dataType == variableType::Float){
                    std::string const_name;
                    data(abstr::getFloatConstant(const_name, (float)val));
                    emit("movss "+abstr::getRegister(lit->resolvedType, isPrimary)+", [rel " + const_name + "]");
                }
                else{
                    emit("mov "+abstr::getRegister(lit->resolvedType, isPrimary)+", " + str(val));
                }
            }, lit->value);
        }
        else if (auto* id = dynamic_cast<identifyerNode*>(expr)) {
            print("    identifyer node");
            if(id->address == 0) {throw std::runtime_error("codegen: non existant address for identifyer node in expression");}

            emit(abstr::mov(id->resolvedType, abstr::getRegister(id->resolvedType, true), "[rbp - " + str(id->resolvedType.address) + "]"));
        }
        else if (auto* bin = dynamic_cast<binaryExpresssionNode*>(expr)) {
            print("    binary node");
            auto* l = bin->left.get();

            // evaluate left expression (result lands in eax/rax)
            generateExpression(bin->left.get(), true);
            
            // save left result onto the stack
            if(l->resolvedType.dataType == variableType::Float){
                pushFloat(true);
            }
            else emit("push " + abstr::getRegister(l->resolvedType, true, false));
            
            // evaluate right expression (result lands in eax/rax)
            generateExpression(bin->right.get(), true);

            if(bin->oper == "/"){
                emit(abstr::handleDivFlip(bin->left->resolvedType));
                if(l->resolvedType.dataType == variableType::Float){
                    popFloat(true);
                }else{
                emit("pop " + abstr::getRegister(l->resolvedType, true, false));}
            }
            else{
                if(l->resolvedType.dataType == variableType::Float){
                    popFloat(false);
                }else{
                emit("pop " + abstr::getRegister(l->resolvedType, false, false));}
            }
            
            // pop the saved left result into rcx/ecx
            

            std::string regPrimary = abstr::getRegister(l->resolvedType, true);     // eax
            std::string regSecondary = abstr::getRegister(l->resolvedType, false);   // ecx

            std::string operation = abstr::handleFullOperation(bin->oper, l->resolvedType);

            if(operation !=  ""){
                emit(operation);
            }
            else if (bin->oper == "/") {
                emit(abstr::handleDivision(l->resolvedType, l->resolvedType.address, bin->right->resolvedType.address));    
            } 
            else {
                throw std::runtime_error("codegen: unsupported binary operator '" + bin->oper + "'");
            }
        }
        else if (auto* cast = dynamic_cast<castNode*>(expr)) {
            print("    cast node");
            generateExpression(cast->expression.get(), isPrimary);
        }
        else {
            throw std::runtime_error("codegen: unknown expression node type");
        }
        print("    retruning from expr");
    }

    static void generateVariableDecl(variableDeclNode* node){

        // expression
       if(logs) emit("; " + node->name + "'s expression");
        generateExpression(node->expression.get(), true);

        // any casting
        auto& from = node->expression->resolvedType;
        auto& to = node->type;
        std::string caster = abstr::getResize(from, to, node->expression->resolvedType.dataType);
        if(logs && caster.length() > 0) emit("; cast value from " + str(from.totalBytes*8) + "-bit to " + str(to.totalBytes*8) + "-bit");
        emit(caster);

        // saving onto stack
        if(logs) emit("; " + node->name + " saved in stack");
        if(node->type.dataType == variableType::Float) emit("movss " + abstr::getDataType(to.totalBytes) + " [rbp - " + str(node->type.address) + "], "+ abstr::getRegister(node->type, true));
        else emit("mov " + abstr::getDataType(to.totalBytes) + " [rbp - " + str(node->type.address) + "], "+ abstr::getRegister(node->type, true));
        if(logs) emit(""); // new line. makes assembly easier to read and nicer
    }

    static void generateHeader(size_t stackSize){
        tabbed = false;
        header += "bits 64\n";
        header += "default rel\n";
        header += "extern printf\n";

        data("\nsection .data", false);
        data (R"(fmt db "%f", 10, 0)");

        emit("\nsection .text");
        emit("global main");

        emit("");
        
        // BEFORE METHODS CAN BE MADE (for now..):
        label("main");

        tabbed = true;

        // save old frame pointer
        emit("push rbp");
        // ancher new base pointer
        emit("mov rbp, rsp");
        // get stack space 'carve out' ig bro..
        emit("sub rsp, "+ str(stackSize));
        emit("");
    }

    static void generateEnd(){
        tabbed = true;
        text += R"(
    movss    xmm0, dword [rbp - 4]
    cvtss2sd xmm1, xmm0            ; printf takes doubles, so float32 must be widened
    movq     rdx, xmm1             ; variadic: the double goes in BOTH xmm1 and rdx
    lea      rcx, [rel fmt]        ; arg 1 = format string
    call     printf)";
        emit("");
        emit("mov  rsp, rbp");
        emit("pop rbp");
        emit("ret");
    }

    static void generateNode(node* node){
        if(auto* varDecl = dynamic_cast<variableDeclNode*>(node)){
            generateVariableDecl(varDecl);
            return;
        }
        else{
            throw std::runtime_error("codegen: unexpected node to be generated");
        }
        
        
    }
public:
    static std::string generate(std::vector<std::unique_ptr<node>>& nodes, size_t stackSize, bool log = false){
        header.clear();
        data_.clear();
        text.clear();


        logs = log;   

        generateHeader(stackSize);

        for(auto& node : nodes){
            generateNode(node.get());
        }

        generateEnd();

        std::string assembly = header + data_ + text;

        return assembly;
    }
};