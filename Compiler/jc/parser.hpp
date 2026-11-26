#pragma once

#include <vector>
#include <iostream>
#include <memory>
#include <charconv>
#include <unordered_map>
#include <string>
#include <stdexcept>
#include <fstream>

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
    static token back() {return tokens->at(count--);}
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
                if(check(tokenType::open_bracket)){  // method call
                    advance(); // take '('
                    auto call = std::make_unique<methodNode>();
                    call->name = tok.value;

                    while(!check(tokenType::close_bracket) && !check(tokenType::end)){
                        call->parameters.push_back(parseExpression());
                        if(check(tokenType::comma)) advance();
                        else if(!check(tokenType::close_bracket))
                            throw std::runtime_error("line " + std::to_string(peek().line) + "| expected ',' or ')' in method call");
                    }
                    if(!check(tokenType::close_bracket))
                        throw std::runtime_error("line " + std::to_string(peek().line) + "| expected ')' to close method call");
                    advance(); // take ')'
                    return call;
                }
                auto ptr = std::make_unique<identifyerNode>();
                ptr->name = tok.value;
                return ptr;
            }
            case tokenType::oper:{
                bool wrongOperator = tok.value != "-";

                if(wrongOperator){
                    if(!(tok.value == "&" || tok.value == "*")) throw std::runtime_error("line " + std::to_string(peek().line) + "| expected a '-', '&' or '*' as a prefix. No other operators are allowed before identifyers or in expressions");
                    
                    if(tok.value == "&"){
                        auto ref = std::make_unique<referenceNode>();
                        if(!check(tokenType::identifyer)) throw std::runtime_error("line " + std::to_string(peek().line) + "| expected an identifyer after a prefix operator usage");
                        ref->target = peek().value;
                        advance();
                        return ref;
                    }
                    else{ // *
                        auto ptr = std::make_unique<pointerNode>();
                        if(!check(tokenType::identifyer)) throw std::runtime_error("line " + std::to_string(peek().line) + "| expected an identifyer after a prefix operator usage");
                        ptr->target = peek().value;
                        advance();
                        return ptr;
                    }
                    
                }
                else{
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
                        throw std::runtime_error("line " + std::to_string(tok.line) + "| expected a float or integer value after prefix operator used");
                    }
                }
            }
            default:
                throw std::runtime_error("line " + std::to_string(tok.line) + "| unknown tokentype and unsupported case to parse as primary :" + peek().string());
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
        advance(); // take '('

        std::unique_ptr<methodDeclNode> decl = std::make_unique<methodDeclNode>(name, typeData);

        if(check(tokenType::close_bracket)){    // no parameters
            advance(); // take ')'
            decl->body = parseBody();
        }
        else{   // yes parameters
            // parse all parameters
            while(!(check(tokenType::close_bracket) || check(tokenType::end))){
                parameterData parem;
                // read parameters
                if(!check(tokenType::type)) throw std::runtime_error("line " + std::to_string(peek().line) + "| expected a data type in the format 'methodName(TYPE NAME, TYPE2 NAME2, etc..)' instead of ->" + peek().value);
                if(data::primatives.find(peek().value) != data::primatives.end()){
                    parem.typeData = data::primatives[peek().value];
                }
                else if(data::objects.find(peek().value) != data::objects.end()){
                    throw std::runtime_error("line " + std::to_string(peek().line) + "| custom datatypes for parameters are not supported yet.");
                }
                else{
                    throw std::runtime_error("line " + std::to_string(peek().line) + "| unknown datatype for parameter: '" + peek().value + + "'");
                }

                advance(); // take 'TYPE'

                if(check(tokenType::oper)){
                    if(peek().value == "*"){
                        parem.typeData.isPointer = true;

                        parem.typeData.pointeeBytes = typeData.totalBytes;
                        parem.typeData.pointeeType = typeData.dataType;

                        parem.typeData.totalBytes = 8;
                        parem.typeData.dataType = variableType::Pointer;
                    }
                    else if(peek().value == "&"){
                        parem.typeData.isReference = true;
                    }
                    else{
                        throw std::runtime_error("line " + std::to_string(peek().line) + "| expected either a * or & after type for method/variable declaration");
                    }
                    advance();
                    
                    if(!check(tokenType::identifyer)) throw std::runtime_error("line " + std::to_string(peek().line) + "| expected an identifyer after type for variable declaration for method parameters");
                }

                if(!check(tokenType::identifyer))  throw std::runtime_error("line " + std::to_string(peek().line) + "| expected a identifyer in the format 'methodName(TYPE NAME, TYPE2 NAME2, etc..)' instead of ->" + peek().value);
                parem.name = peek().value;
                
                advance(); // take 'NAME'

                decl->parameters.push_back(parem);

                if(check(tokenType::close_bracket)){
                    break;
                }
                else if(check(tokenType::comma)){
                    advance(); // take ','
                    continue;
                }
                else{
                    throw std::runtime_error("line " + std::to_string(peek().line) + "| unrecognised token in method declaration paramters. it's ->"+ peek().value);
                }
            }

            // now parse body

            advance(); // take ')' for parseBody()
            decl->body = parseBody(); // easy ^-^.. kinda.......
        }

        return decl;
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
        //std::cout << "next tok to parse: " + peek().string() + "\n";  DEBUG PRINT (this is for me to do ctrl+h some time and update/fix ^-^)
        return decl;
    }

    // used for determining if a method or variable is being declared
    static std::unique_ptr<node> parseTypeDecl(){
        variableData typeData;

        typeData.isPointer = false;
        typeData.isReference = false;

        if(data::primatives.find(peek().value) != data::primatives.end()){
            typeData = data::primatives[peek().value];
        }
        else if(data::objects.find(peek().value) != data::objects.end()){
            throw std::runtime_error("line " + std::to_string(peek().line) + "| custom types are not supported '" + peek().value + "'.");
        }
        else{
            throw std::runtime_error("line " + std::to_string(peek().line) + "| unknown type for variable/method declaration: " + peek().string());
        }

        if(typeData.dataType == variableType::Error){
            throw std::runtime_error("line " + std::to_string(peek().line) + "| type error");
        }
        advance(); // consume data type
        if(!check(tokenType::identifyer) && !check(tokenType::oper)) throw std::runtime_error("line " + std::to_string(peek().line) + "| expected an identifyer (or operator for pointer/reference decl) after type in declaration of method or variable.");
        
        if(check(tokenType::oper)){
            if(peek().value == "*"){
                typeData.isPointer = true;

                typeData.pointeeBytes = typeData.totalBytes;
                typeData.pointeeType = typeData.dataType;

                typeData.totalBytes = 8;
                typeData.dataType = variableType::Pointer;
            }
            else if(peek().value == "&"){
                typeData.isReference = true;
            }
            else{
                throw std::runtime_error("line " + std::to_string(peek().line) + "| expected either a * or & after type for method/variable declaration");
            }
            advance();
            
            if(!check(tokenType::identifyer)) throw std::runtime_error("line " + std::to_string(peek().line) + "| expected an identifyer after type for method/variable declaration");
        }

        std::string name = advance().value;    // get identifyer (var name or method name)

        // why no pre-existing var/method checks? Thats in the analyser stage!

        if(check(tokenType::open_bracket)){ // method delcaration
            return parseMethodDecl(typeData, name);
        }
        return parseVariableDecl(typeData, name); // variable declaration
    }

    // parses {body here}
    // consumes both both curly brackets
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

    static std::unique_ptr<node> parseFor(){
        std::unique_ptr<forNode> For = std::make_unique<forNode>();
        advance(); // 'for'
        if(!check(tokenType::open_bracket)) throw std::runtime_error("line " + std::to_string(peek().line) + "| expected a '(' after the use of the 'for' keyword");
        advance(); // '('
        For->decl = std::move(parseTypeDecl());
        if(auto* _ = dynamic_cast<variableDeclNode*>(For->decl.get())){}else{
            throw std::runtime_error("line " + std::to_string(peek().line) + "| 'for' requires a variable declaration inside the brackets here: for(DECL; CONDITION; STEP)");
        }
        For->condition = std::move(parseExpression());
        advance(); // ';' from between condition and step
        For->step = std::move(parseIdentifyerUse(false));

        if(auto* _ = dynamic_cast<variableResNode*>(For->step.get())){}else{
            throw std::runtime_error("line " + std::to_string(peek().line) + "| 'for' requires a variable reassignment inside the brackets here: for(DECL; CONDITION; STEP)");
        }

        if(!check(tokenType::close_bracket)) throw std::runtime_error("line " + std::to_string(peek().line) + "| expected a ')' to close 'for'");
        advance(); // take ')'

        if(check(tokenType::open_curly)){ 
            For->body = parseBody();
        }
        else{ 
            For->body.push_back(parseStatement());
        }

        return For;
    }

    static std::unique_ptr<node> parseReturn(){
        advance(); // comsume 'return'
        std::unique_ptr<returnNode> Ret = std::make_unique<returnNode>();
        Ret->expression = parseExpression();
        if(!check(tokenType::semi_colon)) throw std::runtime_error("line " + std::to_string(peek().line) + "| expected a semi-colon after 'return X'");
        advance(); // consume ';
        return Ret;
    }

    static std::unique_ptr<node> parseDirect(){
        advance(); // take '__direct'
        std::unique_ptr<directNode> dir = std::make_unique<directNode>();
        if(!check(tokenType::open_bracket)) throw std::runtime_error("line " + std::to_string(peek().line) + "| incorrect use of '__direct' expected use: '__direct(\"assembly goes here...\")'");
        advance();
        if(!check(tokenType::string)) throw std::runtime_error("line " + std::to_string(peek().line) + "| incorrect use of '__direct' expected use: '__direct(\"assembly goes here...\")'");
        dir->direct_asm = peek().value;
        advance();
        if(!check(tokenType::close_bracket)) throw std::runtime_error("line " + std::to_string(peek().line) + "| incorrect use of '__direct' expected use: '__direct(\"assembly goes here...\")'");
        advance();
        if(!check(tokenType::semi_colon)) throw std::runtime_error("line " + std::to_string(peek().line) + "| expected a ';' after '__direct()' usage");
        advance();
        return dir;  
    }

    static std::unique_ptr<node> parseUsing(){
        advance();
        if(!check(tokenType::string)) throw std::runtime_error("line " + std::to_string(peek().line) + "| expected a string after 'using' for the file name");
        std::string fileName = peek().value;
        auto u = std::make_unique<usingNode>();
        advance(); // tskae string file anme token

        u->isStd = fileName.size() >= 5 && fileName.compare(fileName.size() - 5, 5, ".jarv") == 0;
        
        std::ifstream file(fileName);
        std::string source = "";
        std::string line;
        while(std::getline(file, line)){
            source += line;
        }

        std::vector<token> includedTokens = tok::tokenise(source, false, false);
        tokens->insert(tokens->begin() + count, includedTokens.begin(), includedTokens.end());
        return std::make_unique<ignoreNode>();
    }

    // hyper overengineered but larp worthy
    static inline std::unordered_map<std::string, std::unique_ptr<node>(*)()> parsableKeywords = {
        {"if", parseIf},
        {"while", parseWhile},
        {"for", parseFor},
        {"return", parseReturn},

        {"__direct", parseDirect},
        {"using", parseUsing}
    };

    static std::unique_ptr<node> parseKeyword(){
        auto it = parsableKeywords.find(peek().value);
        if(it == parsableKeywords.end()){
            throw std::runtime_error("line " + std::to_string(peek().line) + "| keyword not supported by parser");
        }
        return it->second(); // a string as an id is so... beautiful..
    }

    static std::unique_ptr<node> parseMethodCall(std::string& name){
        advance(); // take '('

        std::unique_ptr<methodCallNode> call = std::make_unique<methodCallNode>();
        call->methodName = name;

        if(check(tokenType::close_bracket)){
            
        }
        else{
            // parse expressions for each parameter
            while(peek().type != tokenType::close_bracket && peek().type != tokenType::end){
                call->parameters.push_back(parseExpression());
                if(check(tokenType::comma)){
                    advance();
                    continue;
                }
                else if(check(tokenType::close_bracket)){
                    break;
                }
                else{
                    throw std::runtime_error("line " + std::to_string(peek().line) + "| unexpected token in method call parameters");
                }
            }
        }

        advance(); // take ')'
        
        if(!check(tokenType::semi_colon)) throw std::runtime_error("line " + std::to_string(peek().line) + "| expected a ';' after method call");
        advance(); // take ';'

        return call;
    }

    static std::unique_ptr<node> parseVariableReassignment(std::string& name, bool needSemi = true){
            std::unique_ptr<variableResNode> reassign = std::make_unique<variableResNode>();
            advance(); // take '='
            reassign->expression = parseExpression();
            reassign->name = name;

            if(needSemi){
                if(!check(tokenType::semi_colon)) throw std::runtime_error("line " + std::to_string(peek().line) + "| expected a ';' after variable reassignment");
                advance(); // take ';'
            }
            return reassign;
        }

        static std::unique_ptr<node> parseShortVariableReassignment(std::string& name, bool needSemi = true){
        int line = peek().line;
        std::string oper = advance().value;
        if(oper.length() != 2) {
            throw std::runtime_error("line " + std::to_string(line) + "| bad short reassignment operator '" + oper + "'" + "prev :" + back().string() + back().string());
        }

        auto self = std::make_unique<identifyerNode>();
        self->name = name;

        auto reassign = std::make_unique<variableResNode>();

        if(oper[1] == '='){                       // += -= *= /=
            if(std::string("+-*/").find(oper[0]) == std::string::npos) throw std::runtime_error("line " + std::to_string(line) + "| '" + oper + "' is not a valid short reassignment");

            auto right = parseExpression();
            reassign->expression = std::make_unique<binaryExpresssionNode>(std::move(self), std::string(1, oper[0]), std::move(right));
        }
        else if(oper == "++" || oper == "--"){    // x++  x--
            auto lit = std::make_unique<literalNode>(1, data::primatives["int32"]);
            reassign->expression = std::make_unique<binaryExpresssionNode>(std::move(self), std::string(1, oper[0]), std::move(lit));
        }
        else{
            throw std::runtime_error("line " + std::to_string(line) + "| incorrect short reassignment '" + oper + "'");
        }

        if(needSemi){
            if(!check(tokenType::semi_colon)) throw std::runtime_error("line " + std::to_string(peek().line) + "| expected a ';'");
            advance();
        }

        reassign->name = name;
        variableData placeHolder;
        placeHolder.totalBytes = -1;
        reassign->expression->resolvedType = placeHolder;
        return reassign;
    }

    static std::unique_ptr<node> parseIdentifyerUse(bool needSemi = true){
        std::string name = advance().value;

        if(check(tokenType::open_bracket)){
            return parseMethodCall(name);
        }
        else if(check(tokenType::equals)){
            return parseVariableReassignment(name, needSemi);
        }
        else if(check(tokenType::oper)){
            return parseShortVariableReassignment(name, needSemi);
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