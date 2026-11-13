#pragma once

#include <memory>
#include <string>

#include <variant>
#include <cstdint>

#include "data.hpp"

struct node {
    int line = 0;
    virtual ~node() = default;
    node(){
        line = 0;
    }
};

struct null : public node {};

// expression nodes

struct expressionNode : public node{
    variableData resolvedType = variableData(0, variableType::Error);
    expressionNode() = default;
};

struct castNode : public expressionNode{
    std::unique_ptr<expressionNode> expression;
    variableData target;

    castNode() = default;

    castNode(std::unique_ptr<expressionNode> _expression, variableData _targetType){
        expression = std::move(_expression);
        target = _targetType;
        resolvedType = _targetType;
    }
};

struct identifyerNode : public expressionNode{
    std::string name;
    size_t address;
    variableData type;
};

using literalType = std::variant<int32_t, uint32_t, float>;

struct literalNode : public expressionNode{
    literalType value;

    literalNode() = default;
    
    literalNode(literalType _val, variableData type){
        value = _val;
        resolvedType = type;
    }
};

struct binaryExpresssionNode : public expressionNode{
    std::unique_ptr<expressionNode> left;
    std::string oper;
    std::unique_ptr<expressionNode> right;

    binaryExpresssionNode(std::unique_ptr<expressionNode> _left, std::string _oper, std::unique_ptr<expressionNode> _right){
        left = std::move(_left);
        oper = _oper;
        right = std::move(_right);
    }

};

// statement nodes

struct variableDeclNode : public node{
    std::string name;
    variableData type;

    variableDeclNode(std::string _name, variableData _type){
        name = _name;
        type = _type;
    }

    std::unique_ptr<expressionNode> expression;
};