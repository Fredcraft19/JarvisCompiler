#pragma once

#include <vector>
#include <unordered_map>
#include <string>

#include <iostream>

#include "node.hpp"

class analyser{
private:
    static inline std::vector<std::unique_ptr<node>>* nodes;

    static inline std::vector<std::unordered_map<std::string, variableData>> scopes;

    static inline uint64_t currentAddress;
    static inline uint64_t maxStackSize;

    // bump allocator for stack variables
    static uint64_t getAddress(size_t size){
        currentAddress += size;

        if(currentAddress > maxStackSize) maxStackSize = currentAddress;

        return currentAddress + 32;
    }

    // scope helpers
    static bool checkInScope(std::string name){
        for(auto& map : scopes){
            if(map.find(name) != map.end()){
                return true;
            }
        }
        return false;
    }
    static variableData findInScope(std::string name){
        for(auto& map : scopes){
            if(map.find(name) != map.end()){
                return map[name];
            }
        }
        return variableData(0, variableType::Error);
    }
    static void addInScope(std::string name, variableData data){
        scopes[scopes.size()-1][name] = data;
    }
    static void popScope(){
        if(scopes.size() < 1){
            throw std::runtime_error("analyser: scope tracker underflow, internal compiler error or a '}' was found without a starting '{'");
        }

        auto& wipedVars = scopes[scopes.size() -1];
        for(auto& [str, val] : wipedVars){
            currentAddress -= val.totalBytes;
        }

        scopes.pop_back();
    }
    static void pushScope(){
        scopes.emplace_back();
    }

    static variableData analysePrimary(node* node, bool expectUnsigned = false){
        if(auto* lit = dynamic_cast<literalNode*>(node)){
            if(lit->resolvedType.dataType == variableType::Int && expectUnsigned){
                lit->resolvedType.dataType = variableType::Uint;

                bool negative = std::visit([](auto&& value) {
                    return value < 0;
                }, lit->value);

                if(negative){
                    throw std::runtime_error("analyser: negative numbers are not supported for uint.");
                }
            }
            return lit->resolvedType;
        }
        else if(auto* id = dynamic_cast<identifyerNode*>(node)){
            std::string& idName = id->name;
            if(!checkInScope(idName)) throw std::runtime_error("analyser: variable '"+ idName+"' does not exist in scope");
            variableData data = findInScope(idName);
            id->resolvedType = data;
            id->address = data.address;
            return id->resolvedType;            
        }
        else if(auto* cast = dynamic_cast<castNode*>(node)){
            analyseExpression(cast->expression);
            return cast->resolvedType;
        }

        throw std::runtime_error("analyser: unknown primary 'expression' was attempted to be analysed");
    }

    static variableData analyseExpression(std::unique_ptr<expressionNode>& expr, bool expectUnsigned = false){
        if(!expr) throw std::runtime_error("analyser: null expression pointer");

        if (auto* bin = dynamic_cast<binaryExpresssionNode*>(expr.get())){
            variableData leftType = analyseExpression(bin->left, expectUnsigned);
            variableData rightType = analyseExpression(bin->right, expectUnsigned);

            if(leftType.dataType != rightType.dataType){
                throw std::runtime_error("analyser: type mismatch in binary expression");
            }

            if(leftType.dataType == variableType::Int && expectUnsigned){
                leftType.dataType = variableType::Uint;
                rightType.dataType = variableType::Uint; // right type is the same (ensured from the error throw a few lines above)
            }
            bin->resolvedType = leftType;
            return leftType;
        }
        return analysePrimary(expr.get(), expectUnsigned);
    } 

    static void analyseVarDecl(variableDeclNode* node){
        if(checkInScope(node->name)){
            throw std::runtime_error("analyser: variable or method of name '" + node->name+"' already exists in scope");
        }

        node->type.address = getAddress(node->type.totalBytes);

        addInScope(node->name, node->type);

        if(node->expression){
            variableData exprType = analyseExpression(node->expression, node->type.dataType == variableType::Uint);
            if(exprType.dataType != node->type.dataType){
                throw std::runtime_error("analyser: cannot assign different type from expression '"+std::to_string(exprType.dataType)+"' to variable as type '"+std::to_string(node->type.dataType)+"'");
            }
        }
    }

    static void analyseNode(node* node){
        if(auto* varDecl = dynamic_cast<variableDeclNode*>(node)){
            analyseVarDecl(varDecl);
        }
        else{
            throw std::runtime_error("analyser: unexpected node to be analysed");
        }
    }

public:
    // takes a reference of a vector<node> and analyses it
    // looking for errors and semantic stuff. idk bro.
    // it will throw errors if it finds them!!!
    // it changes the nodes you input so the return is the max stack size
    // for allocation in the codegen
    static size_t analyse(std::vector<std::unique_ptr<node>>& _nodes, bool log = false){
        nodes = &_nodes;
        maxStackSize = 0;
        currentAddress = 0;
        scopes.clear();
        pushScope();

        if (log) std::cout <<"node cound:\n" << std::to_string(nodes->size())+"\n";
    
        for(int i = 0; i < nodes->size(); i++){
            analyseNode(nodes->at(i).get());
        }

        popScope();

        return maxStackSize;
    }
};