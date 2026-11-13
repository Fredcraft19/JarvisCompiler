#pragma once

#include <vector>
#include <iostream>
#include <memory>
#include <charconv>

#include <cstdint>

#include "token.hpp"
#include "node.hpp"

class parser{
private:

    // input tokens from tokeniser
    static inline std::vector<token>* tokens;

    // output nodes for analasys
    static inline std::vector<std::unique_ptr<node>> nodes;

    // helper: adds a node to the node output for the analyser in the next layer
    static void add(std::unique_ptr<node> n){
        nodes.push_back(std::move(n));
    }
    // helper: returns binding power of operators for parseExpression
    static int bindingPower(std::string& oper){
        if(oper == "+" || oper == "-") return 1;
        if(oper == "*" || oper == "/") return 2;
        return 0;
    }

    // parser helper: keeps track of current index in tokens
    static inline int count;
    // parser helper: views the current token
    static token peek(){return tokens->at(count);}
    // parser helper: views the current token then advances to the next
    static token advance() {return tokens->at(count++);}
    // parser helper: compares the parameters token type with the peek() type, returns true if types are the same
    static bool check(tokenType t) {return tokens->at(count).type == t;}

    static std::unique_ptr<expressionNode> parsePrimary(){
        token tok = advance();

        switch(tok.type){
            case tokenType::integer:{
                int32_t value=0;
                std::from_chars(tok.value.data(), tok.value.data() + tok.value.size(), value);
                variableData data = data::primatives["int32"];
                return std::make_unique<literalNode>(value, data);
            }
            case tokenType::floating:{
                float value=0;
                std::from_chars(tok.value.data(), tok.value.data() + tok.value.size(), value);
                variableData data = data::primatives["float32"];
                return std::make_unique<literalNode>(value, data);
            }
            case tokenType::open_bracket:{
                auto expr = parseExpression();
                if(advance().type != tokenType::close_bracket){
                    throw std::runtime_error("line " + std::to_string(peek().line) + "| expected ')' to close off the brackets");
                }
                return expr;
            }
            case tokenType::identifyer:{
                std::string variableName = tok.value;
                auto ptr = std::make_unique<identifyerNode>();
                ptr->name = variableName;
                return std::move(ptr);
            }
            case tokenType::oper:{
                bool invalidType = peek().type != tokenType::integer && peek().type != tokenType::floating;
                bool wrongOperator = tok.value != "-";
                if(invalidType || wrongOperator) throw std::runtime_error("line " + std::to_string(tok.line) + "| unknown token to parse as primary");

                // parse something like -5 to be 0-5
                tok = advance();
                if(tok.type == tokenType::integer){  // for a negative integer
                    int32_t value=0;
                    std::from_chars(tok.value.data(), tok.value.data() + tok.value.size(), value);
                    variableData data = data::primatives["int32"];
                    return std::make_unique<binaryExpresssionNode>(std::make_unique<literalNode>(0, data::primatives["int32"]), "-", std::make_unique<literalNode>(value, data));
                }
                else if(tok.type == tokenType::floating){    // for a negative float 
                    float value=0;
                    std::from_chars(tok.value.data(), tok.value.data() + tok.value.size(), value);
                    variableData data = data::primatives["float32"];
                    return std::make_unique<binaryExpresssionNode>(std::make_unique<literalNode>(0, data::primatives["float32"]), "-", std::make_unique<literalNode>(value, data));
                }
                else{   // mystery error fall back
                    // somehow though..
                    throw std::runtime_error("line " + std::to_string(tok.line) + "| unknown token to parse as primary\nFUNKY EDITION ERROR!!!");
                }
            }
            default:
                throw std::runtime_error("line " + std::to_string(tok.line) + "| unknown token to parse as primary");
        }
    }

    // parses an expression, returning a unique_ptr of expressionNode. use std::move(parseExpression())
    static std::unique_ptr<expressionNode> parseExpression(int minPrecidence = 0){
        auto left = parsePrimary();

        while(true){
            if(peek().type != tokenType::oper) break;

            std::string oper = peek().value;
            int precidence = bindingPower(oper);
            if(precidence < minPrecidence){
                break;
            }

            advance();  // consumes operator token

            auto right = parseExpression(precidence + 1);

            left = std::make_unique<binaryExpresssionNode>(std::move(left), oper, std::move(right));
        }
        return left;
    }


    // parse a method declaration
    static void parseMethodDecl(variableData& typeData, std::string& name){
        throw std::runtime_error("line " + std::to_string(peek().line) + "| method declaration is not supported for identifyer '" + name+"'.");
    }

    // parse a variable declaration
    static void parseVariableDecl(variableData& typeData, std::string& name){
        std::unique_ptr<variableDeclNode> node = std::make_unique<variableDeclNode>(name, typeData);

        // handle default initialization, expression initlalization or errors
        if(peek().type == tokenType::semi_colon){   // default initialization
            node->expression = std::make_unique<literalNode>(static_cast<int32_t>(0), typeData);
            add(std::move(node));
            return;
        }   
        else if(peek().type == tokenType::equals){
            advance(); // occupy equals
            node->expression = std::move(parseExpression());
        }
        else{
            throw std::runtime_error("line " + std::to_string(peek().line) + "| unexpected token during declaration of variable '" + name + "','" + peek().value+"'"+std::to_string(peek().type)+"\n");
        }

        if(peek().type != tokenType::semi_colon) throw std::runtime_error("expected a semi colon, instead of " + peek().value);
        advance();
        add(std::move(node));
    }

    // used for determining if a method or variable is being declared
    static void parseTypeDecl(){
        variableData typeData;

        if(data::primatives.find(peek().value) != data::primatives.end()){
            typeData = data::primatives[peek().value];
        }
        else if(data::objects.find(peek().value) != data::objects.end()){
            throw std::runtime_error("line "+std::to_string(peek().line) + "| custom types are not supported '"+peek().value + "'." );
        }
        else{
            throw std::runtime_error("line "+std::to_string(peek().line) + "| unknown type for variable/method declaration");
        }

        if(typeData.dataType == variableType::Error){
            throw std::runtime_error("line " + std::to_string(peek().line) + "| type error");
        }
        advance(); // consume data type
        if(!check(tokenType::identifyer)) {throw std::runtime_error("line "+std::to_string(peek().line) + "| expected an identifyer after type in declaration of method or variable.");}
        std::string name = advance().value;    // get identifyer (var name or method name)

        // why no pre-existing var/method checks? Thats in the analyser stage!

        if(peek().type == tokenType::open_bracket){ // method delcaration
            parseMethodDecl(typeData, name);
        }
        else{ // variable declaration
            parseVariableDecl(typeData, name);
        }
    }

    // determins how to process the tokens,
    //as a method declaration? or keyword statment? etc..
    static void parseStatement(){
        switch(peek().type){
            case tokenType::type:
                parseTypeDecl();
                break;

            default:
                throw std::runtime_error("line " + std::to_string(peek().line) + "| invalid or unsupported token starting in a statement parser: '" + peek().value + "'\n");
        }
    }

public:
    static std::vector<std::unique_ptr<node>> parse(std::vector<token>& _toks, bool log = false){
        nodes.clear();
        tokens = &_toks;
        count = 0;
        

        while(tokens->size() > count && peek().type != tokenType::end){
            parseStatement();
        }

        return std::move(nodes);
    }

};