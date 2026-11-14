#pragma once

#include <string>
#include <vector>
#include <charconv>
#include <algorithm>

#include "token.hpp"
#include "data.hpp"


class tok{
private:
    static inline std::vector<token> toks;
    static inline std::string word;
    
    static inline int lineCount;

    static inline int iVal;
    static inline float fVal;

    static void add(tokenType t, std::string val = "", bool wipe = true){
        token n;
        n.type = t;
        n.value = val;
        n.line =  lineCount;
        toks.push_back(n);

        if(wipe) word = "";
    }

    // clear current word and add new corresponding token
    static void flush(){
        if(word.length() < 1 || word == ""){
            word = "";
            return;
        }
        std::string sub = "";
        if(word.length() > 1 && word[word.length()-1] == 'f'){
            sub = word.substr(0, word.length()-1);
        }

        if(data::primatives.find(word) != data::primatives.end() || data::objects.find(word) != data::objects.end()){
            add(tokenType::type, word);
        }
        else if(auto [ptr, ec] = std::from_chars(sub.data(), sub.data() + sub.size(), fVal); ec == std::errc() && sub.length() > 0){
            add(tokenType::floating, std::to_string(fVal));
        }
        else if(auto [ptr, ec] = std::from_chars(word.data(), word.data() + word.size(), iVal); ec == std::errc()){
            add(tokenType::integer, std::to_string(iVal));
        }
        else if(std::find(data::keywords.begin(), data::keywords.end(), word) != data::keywords.end()){
            add(tokenType::keyword, word);
        }
        else if(word == "true" || word == "false"){
            add(tokenType::integer, word == "true" ? "1" : "0");
        }
        else{
            add(tokenType::identifyer, word);
        }
    }

public:
    static token& last(){
        return toks[toks.size()-1];
    }

    // tokenises source code into tokens for the parser
    static std::vector<token> tokenise(std::string& source, bool log = false){
        toks.clear();
        word = "";
        
        lineCount = 1;

        for(int i = 0; i < source.length(); i++){
            char ch = source[i];

            switch(ch){
                case '\r':
                case '\n':
                    lineCount++;
                    break;

                case ' ':
                    flush();
                    break;

                case ';':
                    flush();
                    add(tokenType::semi_colon, ";");
                    break; 
                    
                case '=':
                    flush();
                    if(last().type == tokenType::oper || last().type == tokenType::equals){
                        last().value += "=";
                        last().type = tokenType::oper;
                    }
                    else{
                        add(tokenType::equals, "=");
                    }
                    break;
                    
                case '!':
                case '<':
                case '>':
                case '+':
                case '-':
                case '*':
                case '/':
                    flush();
                    if(last().type == tokenType::oper || last().type == tokenType::equals){
                        last().value += std::string(1, ch);
                    }
                    else{
                        add(tokenType::oper, std::string(1, ch));
                    }
                    break;
                    
                case '(':
                    flush();
                    add(tokenType::open_bracket, std::string(1, ch));
                    break;
                case ')':
                    flush();
                    add(tokenType::close_bracket, std::string(0, ch));
                    break;

                case '{':
                    flush();
                    add(tokenType::open_curly, std::string(1, ch));
                    break;
                case '}':
                    flush();
                    add(tokenType::close_curly, std::string(0, ch));
                    break;


                default:
                    word += ch;
                    break;
            }
        }

        add(tokenType::end);

        return toks;
    }
};