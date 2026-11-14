#pragma once

#include <vector>
#include <iostream>
#include <memory>
#include <charconv>
#include <unordered_map>
#include <string>
#include <stdexcept>

#include <cstdint>

#include "token.hpp"
#include "node.hpp"

class parser{
private:

    // input tokens from tokeniser
    static inline std::vector<token>* tokens;

    // helper: returns binding power of operators for parseExpression
    static int bindingPower(std::string& oper){
        if(oper == "||" || oper == "&&") return 1;
        if(data::isBoolOperator(oper)) return 2;
        if(oper == "+" || oper == "-") return 3;
        if(oper == "*" || oper == "/") return 4;
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
                auto ptr = std::make_unique<identifyerNode>();
                ptr->name = tok.value;
                return ptr;
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
                    throw std::runtime_error("line " + std::to_string(tok.line) + "| unknown token to parse as primary\nFUNKY EDITION ERROR!!!");
                }
            }
            default:
                throw std::runtime_error("line " + std::to_string(tok.line) + "| unknown token to parse as primary");
        }
    }

    // parses an expression, returning a unique_ptr of expressionNode
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
    static std::unique_ptr<node> parseMethodDecl(variableData& typeData, std::string& name){
        throw std::runtime_error("line " + std::to_string(peek().line) + "| method declaration is not supported for identifyer '" + name + "'.");
    }

    // parse a variable declaration
    static std::unique_ptr<node> parseVariableDecl(variableData& typeData, std::string& name){
        auto decl = std::make_unique<variableDeclNode>(name, typeData);

        // handle default initialization, expression initlalization or errors
        if(check(tokenType::semi_colon)){   // default initialization
            decl->expression = std::make_unique<literalNode>(static_cast<int32_t>(0), typeData);
        }
        else if(check(tokenType::equals)){
            advance(); // occupy equals
            decl->expression = parseExpression();
        }
        else{
            throw std::runtime_error("line " + std::to_string(peek().line) + "| unexpected token during declaration of variable '" + name + "','" + peek().value + "'" + std::to_string(peek().type) + "\n");
        }

        if(!check(tokenType::semi_colon)) throw std::runtime_error("line " + std::to_string(peek().line) + "| expected a semi colon, instead of " + peek().value);
        advance(); // consume the semi colon
        std::cout << "next tok to parse: " + peek().string() + "\n";
        return decl;
    }

    // used for determining if a method or variable is being declared
    static std::unique_ptr<node> parseTypeDecl(){
        variableData typeData;

        if(data::primatives.find(peek().value) != data::primatives.end()){
            typeData = data::primatives[peek().value];
        }
        else if(data::objects.find(peek().value) != data::objects.end()){
            throw std::runtime_error("line " + std::to_string(peek().line) + "| custom types are not supported '" + peek().value + "'.");
        }
        else{
            throw std::runtime_error("line " + std::to_string(peek().line) + "| unknown type for variable/method declaration");
        }

        if(typeData.dataType == variableType::Error){
            throw std::runtime_error("line " + std::to_string(peek().line) + "| type error");
        }
        advance(); // consume data type
        if(!check(tokenType::identifyer)) throw std::runtime_error("line " + std::to_string(peek().line) + "| expected an identifyer after type in declaration of method or variable.");
        std::string name = advance().value;    // get identifyer (var name or method name)

        // why no pre-existing var/method checks? Thats in the analyser stage!

        if(check(tokenType::open_bracket)){ // method delcaration
            return parseMethodDecl(typeData, name);
        }
        return parseVariableDecl(typeData, name); // variable declaration
    }

    // parses '{ statement* }' into a list of nodes (consumes both curlies)
    static std::vector<std::unique_ptr<node>> parseBody(){
        if(!check(tokenType::open_curly)) throw std::runtime_error("line " + std::to_string(peek().line) + "| expected a '{' to start the body");
        advance(); // take '{'

        std::vector<std::unique_ptr<node>> body;
        while(!check(tokenType::close_curly)){
            if(check(tokenType::end)) throw std::runtime_error("line " + std::to_string(peek().line) + "| expected a '}' to close the body");
            
            body.push_back(parseStatement());
        }

        advance(); // take '}'
        return body;
    }

    static std::unique_ptr<node> parseIf(){
        advance(); // take 'if'
        if(!check(tokenType::open_bracket)) throw std::runtime_error("line " + std::to_string(peek().line) + "| expected a '(' after the use of the 'if' keyword");
        advance(); // take '('

        auto ifnode = std::make_unique<ifNode>();
        ifnode->condition = parseExpression();

        if(!check(tokenType::close_bracket)) throw std::runtime_error("line " + std::to_string(peek().line) + "| expected a ')' to close 'if' expression");
        advance(); // take ')'

        if(check(tokenType::open_curly)){ // if body is {}
            ifnode->body = parseBody();
        }
        else{ // if body is just 1 statement
            ifnode->body.push_back(parseStatement());
        }

        if(peek().type == tokenType::keyword && peek().value == "else"){
            advance(); // take 'else'
            if(check(tokenType::open_curly)){ // base else
                ifnode->elseBody = parseBody();
            }
            else if(peek().type == tokenType::keyword && peek().value == "if"){
                ifnode->elseBody.push_back( parseIf());
            }
            else{
                ifnode->elseBody.push_back(parseStatement());
            }
        }
        

        return ifnode;
    }

    static std::unique_ptr<node> parseWhile(){
        std::unique_ptr<whileNode> While = std::make_unique<whileNode>();
        advance(); // 'take while'
        if(!check(tokenType::open_bracket)) throw std::runtime_error("line " + std::to_string(peek().line) + "| expected a '(' after the use of the 'while' keyword");
        advance(); // take '('

        While->condition = parseExpression();

        if(!check(tokenType::close_bracket)) throw std::runtime_error("line " + std::to_string(peek().line) + "| expected a ')' to close 'while' expression");
        advance(); // take ')'

        if(check(tokenType::open_curly)){ 
            While->body = parseBody();
        }
        else{ 
            While->body.push_back(parseStatement());
        }

        return While;   
    } 

    // hyper overengineered but larp worthy
    static inline std::unordered_map<std::string, std::unique_ptr<node>(*)()> parsableKeywords = {
        {"if", parseIf},
        {"while", parseWhile}
    };

    static std::unique_ptr<node> parseKeyword(){
        auto it = parsableKeywords.find(peek().value);
        if(it == parsableKeywords.end()){
            throw std::runtime_error("line " + std::to_string(peek().line) + "| keyword not supported by parser");
        }
        return it->second(); // a string as an id is so... beautiful..
    }

    static std::unique_ptr<node> parseMethodCall(std::string& name){
        throw std::runtime_error("method calling is not supported");
    }

    static std::unique_ptr<node> parseVariableReassignment(std::string& name){
        std::unique_ptr<variableResNode> reassign = std::make_unique<variableResNode>();
        advance(); // take '='
        reassign->expression = parseExpression();
        reassign->name = name;

        if(!check(tokenType::semi_colon)) throw std::runtime_error("line " + std::to_string(peek().line) + "| expected a ';' after variable reassignment");
        advance(); // take ';'
        return reassign;
    }

    static std::unique_ptr<node> parseIdentifyerUse(){
        std::string name = advance().value;

        if(check(tokenType::open_bracket)){
            return parseMethodCall(name);
        }
        else if(check(tokenType::equals)){
            return parseVariableReassignment(name);
        }
        else{
            throw std::runtime_error("line " + std::to_string(peek().line) + "| unexpected token after identifyer: " + peek().string());
        }

    }

    // determins how to process the tokens,
    // as a method declaration? or keyword statment? etc..
    static std::unique_ptr<node> parseStatement(){
        switch(peek().type){
            case tokenType::type:
                return parseTypeDecl();
            case tokenType::identifyer:
                return parseIdentifyerUse();
            case tokenType::keyword:
                return parseKeyword();
            default:
                throw std::runtime_error("line " + std::to_string(peek().line) + "| invalid or unsupported token starting in a statement parser: '" + peek().string() + "'\n");
        }
    }

public:
    static std::vector<std::unique_ptr<node>> parse(std::vector<token>& _toks, bool log = false){
        std::vector<std::unique_ptr<node>> nodes;
        tokens = &_toks;
        count = 0;

        while(tokens->size() > static_cast<size_t>(count) && peek().type != tokenType::end){
            nodes.push_back(parseStatement());
        }

        return nodes;
    }

};