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
    static inline std::string methods;

    

    static inline bool redirect_to_method;

    static inline std::string text;
    static inline bool tabbed;

    static inline uint32_t labelCount;

    static inline bool logs;

    static void print(std::string x){
        if(logs) std::cout << x << "\n";
    }

    // helper: quicker way to type std::to_string(x)
    template <typename T>
    static std::string str(T x){
        return std::to_string(x);
    }

    static uint32_t getLabelId(){
        return labelCount++;
    }

    static void emit(std::string _nasm){
        if(redirect_to_method){
            if(tabbed) methods+="    ";
            methods += _nasm + "\n";
        }
        else{
            if(tabbed) text+="    ";
            text+=_nasm + "\n";
        }
    }

    static void label(std::string _lbl){
        if(redirect_to_method){
            methods += _lbl + ":\n";
        }
        else{
            text+= _lbl + ":\n";
        }
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

    static void generateVarReassignment(variableResNode* res){
        emit("; " + res->name + " reassignment expression");
        
        generateExpression(res->expression.get(), true);
        // the efficiency is.. amazing..
        // (std::string cmd = float ? "movss" : "mov")
        // i just cant be asked.. sorry...

        emit("; " + res->name + " reassign to stack");

        if(res->type.isReference){
            emit("mov rdx, [rbp - " + str(res->type.address) + "]");
            emit(std::string(res->type.dataType == variableType::Float ? "movss" : "mov") + " [rdx], " + abstr::getRegister(res->type, true));
        }
        else if(res->type.dataType == variableType::Float){
            emit("movss " + abstr::getDataType(res->type.totalBytes) + " [rbp - " + str(res->type.address) + "], " + abstr::getRegister(res->type, true));
        }
        else if (res->type.dataType == variableType::Int || res->type.dataType == variableType::Uint) {
            emit("mov " + abstr::getDataType(res->type.totalBytes) + " [rbp - " + str(res->type.address) + "], " + abstr::getRegister(res->type, true));
        }
    }

    static void generateVariableDecl(variableDeclNode* node){

        // expression

        int bytes = 0;

        if(node->type.isReference){
            if(node->isParameter){
                emit("mov rax, qword[rbp + " + str(node->parameterIndex * 8 + 8)+"]");
            }
            else{
                emit( abstr::identifyerAddress( dynamic_cast<identifyerNode*> ( node->expression.get() ) ) );
            }
            emit("mov qword [rbp - " + str(node->type.address) + "], rax");
            return;
        }

        if(node->isParameter){
            emit("; (is parameter)");
            emit("mov " + abstr::getRegister(node->type, true) + ", " + abstr::getDataType(node->type.totalBytes) + "[rbp + " + str(node->parameterIndex * 8 + 8) + "]");
            bytes = node->type.totalBytes;
        }
        else{
            if(logs) emit("; " + node->name + "'s expression");
            generateExpression(node->expression.get(), true);

            // any casting
            auto& from = node->expression->resolvedType;
            auto& to = node->type;
            std::string caster = abstr::getResize(from, to, node->expression->resolvedType.dataType);
            if(logs && caster.length() > 0) emit("; cast value from " + str(from.totalBytes*8) + "-bit to " + str(to.totalBytes*8) + "-bit");
            emit(caster);
            bytes = to.totalBytes;
        }

        // saving onto stack
        if(logs) emit("; " + node->name + " saved in stack");
        if(node->type.dataType == variableType::Float) emit("movss " + abstr::getDataType(bytes) + " [rbp - " + str(node->type.address) + "], "+ abstr::getRegister(node->type, true));
        else emit("mov " + abstr::getDataType(bytes) + " [rbp - " + str(node->type.address) + "], "+ abstr::getRegister(node->type, true));
        if(logs) emit(""); // new line. makes assembly easier to read and nicer
    }

    static void generateMethodParameterDecl(variableDeclNode* n){
        // ignore expression: get from stack
        // can easily be changed to support default/optional parameters!
        // essential for QoL
        // just realised this isnt needed! yipee
    }

    static void generateBody(std::vector<std::unique_ptr<node>>& body){
        for(auto& Node: body){
            generateNode(Node.get());
        }
    }

    static void generateMethodBody(std::vector<std::unique_ptr<node>>& body){
        for(auto& n : body){
            if(auto* ptr = dynamic_cast<variableDeclNode*>(n.get())){
                if(ptr->isParameter){
                    generateMethodParameterDecl(ptr);
                }
                else{
                    generateNode(n.get());
                }
            }
            else{
                generateNode(n.get());
            }
        }
    }

    // generates if statement.
    static void generateIf(ifNode* If){
        std::string id = str(getLabelId());
        std::string bodyName = "if_body_"+id;
        std::string elseName = "if_else_"+id;
        std::string endName = "if_end_"+id;

        emit("; ("+id+") if expression" );
        generateExpression(If->condition.get(), true);

        emit("; ("+id+") if comparison and jumping" );
        emit("cmp al, 1");
        emit("je " + bodyName);
        emit("jmp " + elseName);

        emit(""); /// nicer looking spacing for easier readability

        label(bodyName);
        emit("; ("+id+") if body" );

        generateBody(If->body);

        emit("jmp " + endName);
        emit("");
        label(elseName);
        emit("; ("+id+") if else" );

        generateBody(If->elseBody);

        emit(""); // nice spacing

        label(endName);
        emit("; ("+id+") if end" );
    }

    static void generateWhile(whileNode* While){
        std::string id = str(getLabelId());
        std::string bodyName = "while_body_"+id;
        std::string endName = "while_end_"+id;

        emit("; ("+id+") while expression" );
        generateExpression(While->condition.get(), true);

        emit("; ("+id+") while comparison and jumping" );
        emit("cmp al, 1");
        emit("je " + bodyName);
        emit("jmp " + endName);

        emit(""); /// nicer looking spacing for easier readability

        label(bodyName);
        emit("; ("+id+") while body" );

        generateBody(While->body);

        emit("; ("+id+") while loopback check expression" );
        generateExpression(While->condition.get(), true);
        emit("; ("+id+") while loopback check action" );
        emit("cmp al, 1");
        emit("je " + bodyName);
        emit("jmp " + endName);

        emit(""); // nice spacing

        label(endName);
        emit("; ("+id+") while end" );
    }

    static void generateFor(forNode* For){
        std::string id = str(getLabelId());
        std::string bodyName = "for_body_"+id;
        std::string endName = "for_end_"+id;

        emit("; ("+id+") 'for' decl" );
        generateVariableDecl(dynamic_cast<variableDeclNode*>(For->decl.get()));

        emit("; ("+id+") 'for' condition" );
        generateExpression(For->condition.get(), true);
    
        emit("; ("+id+") 'for' comparison and jumping" );
        emit("cmp al, 1");
        emit("je " + bodyName);
        emit("jmp " + endName);
    
        emit("");

        label(bodyName);
        emit("; ("+id+") 'for' body" );
        
        generateBody(For->body);
        emit("; ("+id+") 'for' step");
        generateVarReassignment(dynamic_cast<variableResNode*>(For->step.get()));
        emit("; ("+id+") 'for' condition" );

        generateExpression(For->condition.get(), true);
        emit("cmp al, 1");
        emit("je " + bodyName);
        emit("jmp " + endName);

        emit("");

        label(endName);
        emit("; ("+id+") 'for' end" );

    }

    static void methodArgs(std::string& method, std::vector<std::unique_ptr<expressionNode>>& args){
    auto& params = mData[method].paremeters;
    for(int i = (int)args.size() - 1; i >= 0; i--){
        if(params[i].typeData.isReference){
            auto* id = dynamic_cast<identifyerNode*>(args[i].get());
            if(!id) throw std::runtime_error("codegen: a reference parameter needs a variable as its argument");
            emit(abstr::identifyerAddress(id));
        }
        else generateExpression(args[i].get(), true);
        emit("push rax");
    }
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
        else if(auto* ptr = dynamic_cast<pointerNode*>(expr)){
            variableData& vDat = ptr->targetData;
            if(logs) emit("; pointer value reading");
            emit("mov rax, [rbp - " + str(vDat.address) + "]");
            emit("lea " + abstr::getRegister(vDat) + ", [rax]");
        }
        else if(auto* ref = dynamic_cast<referenceNode*>(expr)){
            if(logs) emit("; stack reference node");
            emit("mov rax, [rbp - " + str(ref->targetData.address)+"]");
        }
        else if (auto* id = dynamic_cast<identifyerNode*>(expr)) {
            print("    identifyer node");
            if(id->resolvedType.address == 0) {throw std::runtime_error("codegen: non existant address for identifyer node in expression :" + id->name);}

            if(id->resolvedType.isReference){
                emit("mov rax, [rbp - " + str(id->resolvedType.address) + "]");
                emit(abstr::mov(id->resolvedType, abstr::getRegister(id->resolvedType, true), "[rax]"));
            }
            else{
                emit(abstr::mov(id->resolvedType, abstr::getRegister(id->resolvedType, true), "[rbp - " + str(id->resolvedType.address) + "]"));
            }
        }
        else if (auto* bin = dynamic_cast<binaryExpresssionNode*>(expr)) {
            print("    binary node");
            auto* l = bin->left.get();
            auto& t = l->resolvedType;
            bool isFloat = t.dataType == variableType::Float;

            generateExpression(bin->left.get(), true);
            if(isFloat) pushFloat(true);
            else {
                emit("push " + abstr::getRegister(t, true, false));
            }

            generateExpression(bin->right.get(), true);

            // right -> secondary, left -> primary
            if(isFloat){
                emit("movaps xmm1, xmm0");
                popFloat(true);
            }
            else{
                emit("mov rcx, rax");
                emit("pop rax");
            }

            std::string regPrimary = abstr::getRegister(l->resolvedType, true);
            std::string regSecondary = abstr::getRegister(l->resolvedType, false);

            std::string operation = abstr::handleFullOperation(bin->oper, l->resolvedType);

            if(operation != ""){
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
        else if(auto* call = dynamic_cast<methodNode*>(expr)){
            print("    method (call) node");
            // get parameter
            methodArgs(call->name, call->parameters);
            // call
            emit("call _" + call->name);
            // clean up
            emit("add rsp, " + str(call->parameters.size() * 8));
        }
        else {
            throw std::runtime_error("codegen: unknown expression node type");
        }
        print("    retruning from expr");
    }

    static void generateMethodDecl(methodDeclNode* methodDecl){
        uint64_t& stackSize = mData[methodDecl->name].maxStackSize;

        // calculate stack size with 16 multiple checking thingys
        if (stackSize % 16 != 0) {
            stackSize += 16 - (stackSize % 16);
        }

        std::string labelName;
        if(methodDecl->name != "main"){
            labelName = "_";
        }
        
        labelName += methodDecl->name;

        bool old_redirect = redirect_to_method;
        bool oldTabbed = tabbed;
        redirect_to_method = true;
        label(labelName);
        tabbed = true;

        // save old frame pointer
        emit("push rbp");
        // ancher new base pointer
        emit("mov rbp, rsp");
        // get stack space 'carve out' ig bro..
        emit("sub rsp, "+ str(stackSize));

        generateBody(methodDecl->body);

        emit("");
        tabbed = true;

        if(methodDecl->returnType.dataType == variableType::Void){
            emit("");
            emit("mov rsp, rbp");
            emit("pop rbp");
            emit("ret");
        }

        redirect_to_method = old_redirect;
        tabbed = oldTabbed;
    }

    static void generateReturn(returnNode* ret){
        generateExpression(ret->expression.get(), true);

        // frame pointer stuff
        emit("mov rsp, rbp");
        emit("pop rbp");

        // actual returning
        emit("ret");
    }

    static void generateMethodCall(methodCallNode* call){
        // parameter injection into the called method's stack (before it exists?!)
        // +32 because of per-method shadow space (windows i love you)
        methodArgs(call->methodName, call->parameters);
        

        // calling
        emit("call _" + call->methodName);

        // clean up
        emit("add rsp, " + str(call->parameters.size() * 8));
        
    }

    static void generateHeader(){
        

        // emit header stuff
        tabbed = false;
        header += "bits 64\n";
        header += "default rel\n";

        data("\nsection .data", false);
        data (R"(ifmt db "int: %d", 10, 0)");
        data (R"(ffmt db "float: %f", 10, 0)");

        emit("\nsection .text");
        emit("    extern printf\n");
        emit("    global main");

        emit("");

        emit(methods);  
    }

    static void generateNode(node* node){
        if(auto* varDecl = dynamic_cast<variableDeclNode*>(node)){
            generateVariableDecl(varDecl);
        }
        else if(auto* resVar = dynamic_cast<variableResNode*>(node)){
            generateVarReassignment(resVar);
        }
        else if(auto* If = dynamic_cast<ifNode*>(node)){
            generateIf(If);
        }
        else if(auto* While = dynamic_cast<whileNode*>(node)){
            generateWhile(While);
        }
        else if(auto* For = dynamic_cast<forNode*>(node)){
            generateFor(For);
        }
        else if(auto* methodDecl = dynamic_cast<methodDeclNode*>(node)){
            generateMethodDecl(methodDecl);
        }
        else if(auto* ret = dynamic_cast<returnNode*>(node)){
            generateReturn(ret);
        }
        else if(auto* call = dynamic_cast<methodCallNode*>(node)){
            generateMethodCall(call);
        }
        else if(auto* dir = dynamic_cast<directNode*>(node)){
            emit(dir->direct_asm);  // super hard!
        }
        else{
            throw std::runtime_error("codegen: unexpected node to be generated");
        }
    }
public:
    static inline std::unordered_map<std::string, methodData> mData;

    static std::string generate(std::vector<std::unique_ptr<node>>& nodes, std::unordered_map<std::string, methodData>& _methodData, bool log = false){
        header.clear();
        data_.clear();
        text.clear();

        mData = _methodData;

        redirect_to_method = false;

        labelCount = 0;

        logs = log;   

        for(auto& node : nodes){
            generateNode(node.get());
        }

        generateHeader();
        
        std::string assembly = header + data_ + text;

        return assembly;
    }
};