#pragma once

#include <vector>
#include <unordered_map>
#include <string>

#include <iostream>

#include "node.hpp"

class analyser{
private:
    static inline std::vector<std::unique_ptr<node>>* nodes;

    static inline std::vector<std::unordered_map<std::string, methodData>> methodScopes;

    static inline std::unordered_map<std::string, methodData> methods_for_codegen;

    static inline methodData* currentMethod;

    // bump allocator for stack variables
    static uint64_t getAddress(size_t size){
        if(currentMethod == nullptr){
            throw std::runtime_error("analyser: cannot declare variables outside a method (for now FIX LATER WITH A GLOBAL METHOD )");
        }
        currentMethod->currentAddress += size;

        if(currentMethod->currentAddress > currentMethod->maxStackSize) currentMethod->maxStackSize = currentMethod->currentAddress;

        // method debug stack size logs
        //std::cout << currentMethod->name + ": highest stack size: "<< std::to_string(currentMethod->maxStackSize) << "\n";
        //std::cout << currentMethod->name + ": current stack size: " << std::to_string(currentMethod->currentAddress) << "\n";

        return currentMethod->currentAddress;
    }

    // scope helpers
    static bool checkInScope(std::string& name){
        return currentMethod->hasVariable(name);
    }
    static variableData findInScope(std::string name){
        return currentMethod->getVariable(name);
    }
    static void addInScope(std::string& name, variableData& data){
        currentMethod->addVariable(name, data);
    }

    static void addMethod(methodData& method){
        methodScopes[methodScopes.size()-1][method.name] = method;
    }

    static bool checkMethod(std::string& methodName){
        for(auto& sr : methodScopes){
            if(sr.find(methodName) != sr.end()){
                return true;
            }
        }
        return false;
    }

    static methodData findMethod(std::string& methodName){
        for(auto& sr : methodScopes){
            if(sr.find(methodName) != sr.end()){
                return sr[methodName];
            }
        }
        
        throw std::runtime_error("analyser check error: bad code broski. fix it. fix with a check before find. thank you! (ANALYSER)");
    }

    static void popScope(){
        if(methodScopes.size() < 1){
            throw std::runtime_error("analyser: scope tracker underflow, internal compiler error or a '}' was found without a starting '{'");
        }
        methodScopes.pop_back();  
    }
    static void pushScope(){
        methodScopes.emplace_back();
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
            if(!checkInScope(idName)) throw std::runtime_error("analyser: variable '"+ idName+"' does not exist in scope for primary analysis");
            variableData data = findInScope(idName);
            id->resolvedType = data;
            id->address = data.address;
            return id->resolvedType;            
        }
        else if(auto* call = dynamic_cast<methodNode*>(node)){
            std::string& name = call->name;
            if(!checkMethod(name)) throw std::runtime_error("analyser: method '"+ name+"' does not exist in scope for primary analysis");
            methodData data = findMethod(name);
            call->resolvedType = data.returnType;
            call->returnType = data.returnType;
            return call->resolvedType;
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

    static void analyseVarRes(variableResNode* res){
        if(!checkInScope(res->name)) throw std::runtime_error("analyser: variable '" + res->name + "' does not exist in scope for variable reassignment");
        auto dat = findInScope(res->name);

        if(res->expression->resolvedType.totalBytes == -1){
            res->expression->resolvedType = dat;
        }

        auto exprDat = analyseExpression(res->expression, dat.dataType == variableType::Uint);
        if(dat.dataType != exprDat.dataType){
            throw std::runtime_error("analyser: cannot reassign a variable ('" +res->name+ "') a different type");
        }
        res->type = dat;
    }

    static void analyseBody(std::vector<std::unique_ptr<node>>& body){
        pushScope();
        for(auto& Node : body){
            analyseNode(Node.get());
        }
        popScope();
    }

    static void analyseIfStatement(ifNode* If){
        analyseExpression(If->condition);

        analyseBody(If->body);
        analyseBody(If->elseBody);

    }

    static void analyseWhileStatement(whileNode* While){
        analyseExpression(While->condition);
        analyseBody(While->body);
    }

    static void analyseForStatement(forNode* For){
        
        pushScope();

        if(auto* varDecl = dynamic_cast<variableDeclNode*>(For->decl.get()))
            analyseVarDecl(varDecl);
        else
            throw std::runtime_error("analyser: expected a variable declaration (DECL) for the first part of the for loop, format: 'for(DECL; CONDITION; STEP)'");
        
        analyseExpression(For->condition);

        if(auto* varRes = dynamic_cast<variableResNode*>(For->step.get()))
            analyseVarRes(varRes);
        else
            throw std::runtime_error("analyser: expected a variable reassignment (STEP) for the final part of the for loop, format: 'for(DECL; CONDITION; STEP)'");

        analyseBody(For->body);
        
        popScope();
    }

    static void analyseMethodDecl(methodDeclNode* methodDecl){
        // save current method so when its overwritten it can be brought back
        auto* old_method = currentMethod;

        methodData dat;
        dat.name = methodDecl->name;
        dat.paremeters = methodDecl->parameters;
        dat.returnType = methodDecl->returnType;
        dat.parentMethod = currentMethod;
        addMethod(dat);

        std::vector<std::unique_ptr<variableDeclNode>> parameterNodes;

        for(auto& parem : dat.paremeters){
            auto decl = std::make_unique<variableDeclNode>(parem.name, parem.typeData);
            decl->isParameter = true;
            parameterNodes.push_back(decl);
        }
        
        // update the current method
        currentMethod = &dat;
        // analyse method's body under new currentMethod
        analyseBody(methodDecl->body);
        // reset the current method to the original one (if any)
        currentMethod = old_method;

        // add parameter declarations at the top
        methodDecl->body.insert(methodDecl->body.begin(), parameterNodes.begin(), parameterNodes.end());

        // update codegen's map for its data on stack sizes
        methods_for_codegen[methodDecl->name] = dat;
    }

    static void analyseReturn(returnNode* ret){
        analyseExpression(ret->expression);
        auto& returnType = ret->expression->resolvedType;
        auto& methodType = currentMethod->returnType;
        if(returnType.dataType != methodType.dataType || returnType.totalBytes != methodType.totalBytes) throw std::runtime_error("analyser: return type isnt the same as the method's specified return type");
    }

    static void analyseCall(methodCallNode* call){
        if(!checkMethod(call->methodName)) throw std::runtime_error("analyser: method of name '" + call->methodName+ "' does not exist in scope");

        auto method = findMethod(call->methodName);
        if(method.paremeters.size() != call->parameters.size()) if(!checkMethod(call->methodName)) throw std::runtime_error("analyser: invalid parameters for method: '" + call->methodName + "'");
    
        for(auto& parem : call->parameters){
            analyseExpression(parem);
        }

        for(int i = 0; i < method.paremeters.size(); i++){
            auto& methodParemData = method.paremeters[i].typeData;
            auto& callerParemData = call->parameters[i]->resolvedType;

            if((methodParemData.totalBytes != callerParemData.totalBytes) && (methodParemData.dataType != callerParemData.dataType)){
                throw std::runtime_error("analyser: mis-matched types for method declared parameter data type and caller paremeter data type");
            }
        }
    }

    static void analyseNode(node* node){
        if(auto* varDecl = dynamic_cast<variableDeclNode*>(node)){
            analyseVarDecl(varDecl);
        }
        else if(auto* resVar = dynamic_cast<variableResNode*>(node)){
            analyseVarRes(resVar);
        }
        else if(auto* whileStatement = dynamic_cast<whileNode*>(node)){
            analyseWhileStatement(whileStatement);
        }
        else if(auto* ifStatement = dynamic_cast<ifNode*>(node)){
            analyseIfStatement(ifStatement);
        }
        else if(auto* forStatement = dynamic_cast<forNode*>(node)){
            analyseForStatement(forStatement);
        }
        else if(auto* methodDecl = dynamic_cast<methodDeclNode*>(node)){
            analyseMethodDecl(methodDecl);
        }
        else if(auto* ret = dynamic_cast<returnNode*>(node)){
            analyseReturn(ret);
        }
        else if(auto* call = dynamic_cast<methodCallNode*>(node)){
            analyseCall(call);
        }
        else{
            throw std::runtime_error("analyser: unexpected node to be analysed");
        }
    }

    static bool hasMain(){
        for(auto& m : methods_for_codegen){
            if(m.first == "main"){
                if(m.second.returnType.dataType == variableType::Int && m.second.returnType.totalBytes == 4){  // enforce int32 return value
                    return true;
                }
                else{
                    throw std::runtime_error("analyser: 'main' must be of 'int' (or int32) return type");
                }
            }
        }
        return false;
    }

public:
    // takes a reference of a vector<node> and analyses it
    // looking for errors and semantic stuff. idk bro.
    // it will throw errors if it finds them!!!
    // it changes the nodes you input so the return is the max stack size
    // for allocation in the codegen
    static std::unordered_map<std::string, methodData>& analyse(std::vector<std::unique_ptr<node>>& _nodes, bool log = false){
        nodes = &_nodes;

        methodData global;
        currentMethod = &global;

        methodScopes.clear();
        
        pushScope();

        if (log) std::cout <<"node cound:\n" << std::to_string(nodes->size())+"\n";
    
        for(int i = 0; i < nodes->size(); i++){
            analyseNode(nodes->at(i).get());
        }

        popScope();

        if(!hasMain()) throw std::runtime_error("analyser: code requires a 'main' method as an entry point");

        return methods_for_codegen;
    }
};