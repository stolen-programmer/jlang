// JuvenileLanguage.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#define _SILENCE_CXX23_ALIGNED_STORAGE_DEPRECATION_WARNING
#define _SILENCE_CXX23_ALIGNED_UNION_DEPRECATION_WARNING

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

#include "formatter.h"
#include "parser.h"
#include "token.h"

using namespace rttr;

std::unique_ptr<llvm::LLVMContext> TheContext;
std::unique_ptr<llvm::Module> TheModule;
std::unique_ptr<llvm::IRBuilder<>> Builder;
std::map<std::string, llvm::Value*> NamedValues;

std::vector<Token> tokens;

void initializeModule()
{
    TheContext = std::make_unique<llvm::LLVMContext>();
    TheModule = std::make_unique<llvm::Module>("juvenile jit", *TheContext);

    Builder = std::make_unique<llvm::IRBuilder<>>(*TheContext);
}

void handle_parameters(Parser& p, std::vector<std::pair<Token, Token>>& args)
{
    auto bracket = p.assert_token(p.match(TokenType::TOK_SMALL_BRACKET, "("),
        TokenError::Expected, TokenType::TOK_SMALL_BRACKET);
    // 非小括号
    Token br;
    // def hello(name String) Result {}

    args.reserve(10);

    do {

        auto name = p.assert_token(p.match(TokenType::TOK_IDENTIFIER), TokenError::Expected, TokenType::TOK_IDENTIFIER);
        auto ty = p.assert_token(p.match(TokenType::TOK_IDENTIFIER), TokenError::Expected, TokenType::TOK_IDENTIFIER);

        args.emplace_back(name, ty);
    } while (!(br = p.match(TokenType::TOK_SMALL_BRACKET, ")")) && p.match(TokenType::TOK_COMMA));

    br = p.assert_token(br, TokenError::Expected, TokenType::TOK_SMALL_BRACKET);
    if (br.content != ")") {
        p.log_error(TokenError::Expected, br);
        exit(static_cast<int>(br.type));
    }
}

std::unique_ptr<AST> handleExpr(Parser& p)
{
    return {};
}

std::unique_ptr<AST> handleBlock(Parser& p)
{
    // { 语句,... }

    auto br = p.assert_token(p.match(TokenType::TOK_CURLY_BRACE, "{"),
        TokenError::Expected, TokenType::TOK_CURLY_BRACE);

    // 变量定义
    if (auto let = p.match(TokenType::TOK_KEYWORD, "let")) {

        auto identifier = p.assert_token(p.match(TokenType::TOK_IDENTIFIER),
            TokenError::Expected, TokenType::TOK_IDENTIFIER);

        p.assert_token(p.match(TokenType::TOK_ASSIGN), TokenError::Expected, TokenType::TOK_ASSIGN);
        VariableAST v = {
            .name = identifier.content,
            .expr = handleExpr(p),
        };
    }
    // if 语句
    if (auto _if = p.match(TokenType::TOK_KEYWORD, "if")) {
    }
}

FunctionAST handleDefineFunction(Parser& p)
{
    // def name(params) Result {}
    auto identifier = p.assert_token(p.match(TokenType::TOK_IDENTIFIER),
        TokenError::Expected, TokenType::TOK_IDENTIFIER);

    std::vector<std::pair<Token, Token>> args;
    handle_parameters(p, args);

    FunctionAST fun(identifier.content, args, p.match(TokenType::TOK_IDENTIFIER));

    return fun;
}

int main()
{
    auto f = llvm::DebugFlag;
    initializeModule();
    auto t = type::get<TokenType>().get_enumeration();

    auto fp = stdin;

    Parser p { .fp = fp };

    while (auto token = p.next_token()) {

        switch (token.type) {

        case TokenType::TOK_ERR:
            break;
        case TokenType::TOK_EOF:
            break;
        case TokenType::TOK_KEYWORD:
            if (p.match_token(token, "def")) {
                auto fun = handleDefineFunction(p);
                std::cout << std::format("{}", fun) << '\n';
            }
            break;
        case TokenType::TOK_IDENTIFIER:
            break;
        case TokenType::TOK_INTEGER_LITERAL:
            break;
        case TokenType::TOK_FLOAT_LITERAL:
            break;
        case TokenType::TOK_STRING_LITERAL:
            break;
        case TokenType::TOK_CHARACTER_LITERAL:
            break;
        case TokenType::TOK_BINARY_OP:
            break;
        case TokenType::TOK_BUILTIN_TYPE:
            break;
        case TokenType::TOK_SMALL_BRACKET:
            break;
        case TokenType::TOK_SQUARE_BRACKET:
            break;
        case TokenType::TOK_CURLY_BRACE:
            break;
        case TokenType::TOK_COMMA:
            break;
        }
    }
    std::cout << "Hello World!\n";
}

// 运行程序: Ctrl + F5 或调试 >“开始执行(不调试)”菜单
// 调试程序: F5 或调试 >“开始调试”菜单

// 入门使用技巧:
//   1. 使用解决方案资源管理器窗口添加/管理文件
//   2. 使用团队资源管理器窗口连接到源代码管理
//   3. 使用输出窗口查看生成输出和其他消息
//   4. 使用错误列表窗口查看错误
//   5. 转到“项目”>“添加新项”以创建新的代码文件，或转到“项目”>“添加现有项”以将现有代码文件添加到项目
//   6. 将来，若要再次打开此项目，请转到“文件”>“打开”>“项目”并选择 .sln 文件
