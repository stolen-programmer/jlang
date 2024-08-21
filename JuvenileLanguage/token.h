#pragma once

#define _SILENCE_CXX23_ALIGNED_STORAGE_DEPRECATION_WARNING
#define _SILENCE_CXX23_ALIGNED_UNION_DEPRECATION_WARNING

#include "token.h"

#include <cstdarg>
#include <format>
#include <iostream>
#include <ranges>
#include <rttr/registration>
#include <set>
#include <vector>

#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/Debug.h"

enum class TokenError;
struct Token;

extern std::unique_ptr<llvm::LLVMContext> TheContext;
extern std::unique_ptr<llvm::Module> TheModule;
extern std::unique_ptr<llvm::IRBuilder<>> Builder;
extern std::map<std::string, llvm::Value*> NamedValues;
extern std::vector<Token> tokens;

using namespace rttr;

enum class TokenType {
    TOK_ERR = -2, // 错误
    TOK_EOF = -1, // 文件结束
    TOK_KEYWORD = 0, // 保留字
    TOK_BUILTIN_TYPE, // 内置类型 比如int
    TOK_IDENTIFIER, // 标识符
    TOK_INTEGER_LITERAL, // int字面量
    TOK_FLOAT_LITERAL, // 浮点字面量
    TOK_STRING_LITERAL, // 字符串字面量
    TOK_CHARACTER_LITERAL, // 字符字面量
    TOK_BINARY_OP, // 二元运算符
    TOK_SMALL_BRACKET, // ()
    TOK_SQUARE_BRACKET, // []
    TOK_CURLY_BRACE, // {}
    TOK_COMMA, // , 逗号
};

enum class TokenError {
    Expected = -1, // 需要什么
};

RTTR_REGISTRATION
{
    registration::enumeration<TokenType>("TokenType") //
        (
            value("tok_err", TokenType::TOK_ERR),
            value("tok_eof", TokenType::TOK_EOF),
            value("tok_keyword", TokenType::TOK_KEYWORD),
            value("tok_builtin_type", TokenType::TOK_BUILTIN_TYPE),
            value("tok_identifier", TokenType::TOK_IDENTIFIER),
            value("tok_integer_literal", TokenType::TOK_INTEGER_LITERAL),
            value("tok_float_literal", TokenType::TOK_FLOAT_LITERAL),
            value("tok_string_literal", TokenType::TOK_STRING_LITERAL),
            value("tok_binary_op", TokenType::TOK_BINARY_OP),
            value("(tok_small_bracket)", TokenType::TOK_SMALL_BRACKET),
            value("[tok_square_bracket]", TokenType::TOK_SQUARE_BRACKET),
            value("{tok_curly_brace}", TokenType::TOK_CURLY_BRACE),
            value("tok_comma", TokenType::TOK_COMMA) //
        );

    registration::enumeration<TokenError>("TokenError") //
        (
            value("Expected", TokenError::Expected) //
        );
}

struct SourceLocation {
    uint32_t start_row = 0, start_col = 0;
    uint32_t end_row = 0, end_col = 0;
};

struct Token {

    TokenType type = TokenType::TOK_EOF;
    std::string content;

    SourceLocation location;

    operator bool() const;
};

struct AST {
    virtual ~AST() = default;
    virtual llvm::Value* codegen() = 0;
};

struct IntegerAST : public AST {

    uint32_t value = 0;

    llvm::Value* codegen() override;
};

struct VariableAST {
};

struct StatementAST {
};

struct BlockAST {
    std::vector<AST> statements;
};

struct FunctionAST : public AST {
    Token result = Token {}; // 返回值类型
    std::string name; // 函数名
    std::vector<std::pair<Token, Token>> args; // 参数表

    llvm::Value* codegen() override;

    FunctionAST(std::string name, const std::vector<std::pair<Token, Token>>& args = {}, Token result = {});
};
