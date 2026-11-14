#pragma once
#include <string>

// Token Type Enum
enum tokenType{

    keyword,
    identifyer,
    oper,
    semi_colon,
    type,
    equals,
    integer,
    floating,

    open_bracket,
    close_bracket,

    open_curly,
    close_curly,

    end
};

std::string tokenType_to_string(tokenType type) {
    switch (type) {
        case tokenType::keyword:       return "Keyword";
        case tokenType::identifyer:    return "Identifyer";
        case tokenType::oper:          return "Operator";
        case tokenType::semi_colon:    return "SemiColon";
        case tokenType::type:          return "Type";
        case tokenType::equals:        return "Equals";
        case tokenType::integer:       return "Integer";
        case tokenType::floating:      return "Floating";
        case tokenType::open_bracket:  return "OpenBracket";
        case tokenType::close_bracket: return "CloseBracket";
        case tokenType::open_curly:    return "OpenCurly";
        case tokenType::close_curly:   return "CloseCurly";
        case tokenType::end:           return "End";
        default:
            throw std::runtime_error("LACK SUPPORT FOR TOKEN TYPE TO STRING");
    }
}

// Token Struct
struct token{
    std::string value;
    tokenType type;
    int line = -1;

    std::string string(){
        return tokenType_to_string(type) + "(" + value + ")";
    }
};