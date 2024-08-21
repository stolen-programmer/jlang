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
#include "token.h"

using namespace rttr;

std::unique_ptr<llvm::LLVMContext> TheContext;
std::unique_ptr<llvm::Module> TheModule;
std::unique_ptr<llvm::IRBuilder<>> Builder;
std::map<std::string, llvm::Value*> NamedValues;

std::vector<Token> tokens;

std::set<std::string> keywords = {
    "def",
    "extern"
};

std::set<std::string> builtin = {
    "int",
    "char"
};

std::set<std::string> binary_op = {
    "+", "-", "*", "/", "%", "="
};

void initializeModule()
{
    TheContext = std::make_unique<llvm::LLVMContext>();
    TheModule = std::make_unique<llvm::Module>("juvenile jit", *TheContext);

    Builder = std::make_unique<llvm::IRBuilder<>>(*TheContext);
}

void skip(int n, ...)
{
    va_list args;
    va_start(args, n);

    std::vector<char> space;

    for (auto i = 0; i < n; i++) {
        space.emplace_back(va_arg(args, char));
    }

    char curr;
    while ((curr = getchar()) != EOF) {
        if (std::ranges::find(space, curr) != space.end()) {
            continue;
        }
        ungetc(curr, stdin);
        break;
    }
}

void skip_space(uint32_t& row, uint32_t& col)
{

    char curr;
    while ((curr = getchar()) != EOF) {
        col++;
        if (isspace(curr)) {
            if (curr == '\n') {
                row++;
                col = 0;
            }
            continue;
        }
        ungetc(curr, stdin);
        col--;
        break;
    }
}

Token next_token()
{
    if (!tokens.empty()) {
        auto token = *tokens.begin();
        tokens.erase(tokens.begin());

        return token;
    }

    // 通常 标识符不会超过20个  已经很大了
    std::string content {};
    content.reserve(20);

    static uint32_t row = 0, col = 0;

    skip_space(row, col);
    SourceLocation source_location {
        .start_row = row,
        .start_col = col - static_cast<uint32_t>(content.length()),
        .end_row = row,
        .end_col = col,
    };
    char curr;
    while ((curr = getchar()) != EOF) {
        col++;
        content += (curr);

        // 字母 字母数字 标识符 保留字
        if (isalpha(curr)) {

            while (isalnum((curr = getchar())))
                content += (curr);
            col += content.length() - 1;

            ungetc(curr, stdin);

            // 保留字
            if (keywords.contains(content)) {
                source_location.end_row = row;
                source_location.end_col = col;
                return Token {
                    .type = TokenType::TOK_KEYWORD,
                    .content = content,
                    .location = source_location
                };
            }

            source_location.end_row = row;
            source_location.end_col = col;

            return Token { TokenType::TOK_IDENTIFIER, content, source_location };
        }

        // 数字 浮点
        if (isdigit(curr)) {
            auto n = 0;
            // 数字
            while (isdigit((curr = getchar()))) {
                content += curr;
            }
            n = content.length() - 1;
            col += n;
            if (curr != '.') {
                ungetc(curr, stdin);
                source_location.end_row = row;
                source_location.end_col = col;
                return { TokenType::TOK_INTEGER_LITERAL, content, source_location };
            }

            content += curr;

            while (isdigit((curr = getchar()))) {
                content += curr;
            }

            col += content.length() - n;

            ungetc(curr, stdin);

            source_location.end_row = row;
            source_location.end_col = col;
            return { TokenType::TOK_FLOAT_LITERAL, content, source_location };
        }

        // 字符串
        if (curr == '\"') {
        escape:
            while ((curr = getchar()) != EOF) {
                content += curr;

                if (curr == '\n') {
                    row++;
                    col = 0;
                }

                if (curr == '\"') {

                    break;
                }
            }
            if (content.length() == 1) {
                return { TokenType::TOK_ERR, "字符串解析出错" };
            }
            // 转义
            if (*(content.end() - 2) == '\\') {
                goto escape;
            }

            source_location.end_row = row;
            source_location.end_col = col;

            return { TokenType::TOK_STRING_LITERAL, content, source_location };
        }

        // 字符
        if (curr == '\'') {
            if ((curr = getchar()) == '\'') {
                return { TokenType::TOK_ERR, "字符解析出错" };
            }

            content += curr;

            if ((curr = getchar()) != '\'') {
                return { TokenType::TOK_ERR, "字符解析出错" };
            }

            content += curr;

            source_location.end_row = row;
            source_location.end_col = col;

            return { TokenType::TOK_CHARACTER_LITERAL, content, source_location };
        }

        // 运算符
        if (binary_op.contains(content)) {
            source_location.end_col = col;
            source_location.end_row = row;
            return { TokenType::TOK_BINARY_OP, content, source_location };
        }

        std::set<char> bracket = { '(', ')', '[', ']', '{', '}' };
        std::map<char, TokenType> typeMap = {
            { '(', TokenType::TOK_SMALL_BRACKET },
            { ')', TokenType::TOK_SMALL_BRACKET },
            { '[', TokenType::TOK_SQUARE_BRACKET },
            { ']', TokenType::TOK_SQUARE_BRACKET },
            { '{', TokenType::TOK_CURLY_BRACE },
            { '}', TokenType::TOK_CURLY_BRACE },
        };
        // 括号
        if (bracket.contains(curr)) {
            source_location.end_col = col;
            source_location.end_row = row;

            return { typeMap[curr], content, source_location };
        }
    }

    return {};
}

void rollback(const Token& token)
{
    tokens.insert(tokens.begin(), token);
}

bool match(const std::string_view token)
{
    return next_token().content == token;
}

bool matchToken(const Token& token, const std::string_view content)
{
    return token.content == content;
}

void logError(TokenError error, Token token)
{
    auto error_ty = type::get(error);
    auto error_en = error_ty.get_enumeration();

    auto type_ty = type::get(token.type);
    auto type_en = type_ty.get_enumeration();

    auto error_name = error_en.value_to_name(error);
    auto error_type = type_en.value_to_name(token.type);

    std::cout << std::format("{} {} start_col: {}", error_name.data(), error_type.data(), token.location.start_col);
}

Token assert_token(Token token, TokenError error, TokenType ty)
{
    if (token) {
        return std::move(token);
    }

    logError(error, { .type = ty, .location = token.location });

    exit(static_cast<int>(error));
}

Token match(const TokenType type)
{
    auto token = next_token();

    if (token.type != type) {
        rollback(token);
        return { .location = token.location };
    }
    return token;
}

void handle_parameters(std::vector<std::pair<Token, Token>>& args)
{
    auto bracket = assert_token(match(TokenType::TOK_SMALL_BRACKET),
        TokenError::Expected, TokenType::TOK_SMALL_BRACKET);
    // 非小括号
    Token br;
    // def hello(name String) Result {}

    args.reserve(10);

    do {

        auto name = assert_token(match(TokenType::TOK_IDENTIFIER), TokenError::Expected, TokenType::TOK_IDENTIFIER);
        auto ty = assert_token(match(TokenType::TOK_IDENTIFIER), TokenError::Expected, TokenType::TOK_IDENTIFIER);

        args.emplace_back(name, ty);
    } while (!(br = match(TokenType::TOK_SMALL_BRACKET)) && match(TokenType::TOK_COMMA));

    br = assert_token(br, TokenError::Expected, TokenType::TOK_SMALL_BRACKET);
    if (br.content != ")") {
        logError(TokenError::Expected, br);
        exit(static_cast<int>(br.type));
    }
}

FunctionAST handleDefineFunction()
{
    // def name(params) Result {}
    auto identifier = assert_token(match(TokenType::TOK_IDENTIFIER),
        TokenError::Expected, TokenType::TOK_IDENTIFIER);

    std::vector<std::pair<Token, Token>> args;
    handle_parameters(args);

    FunctionAST fun(identifier.content, args, match(TokenType::TOK_IDENTIFIER));

    return fun;
}

int main()
{
    auto f = llvm::DebugFlag;
    initializeModule();
    auto t = type::get<TokenType>().get_enumeration();

    while (auto token = next_token()) {

        switch (token.type) {

        case TokenType::TOK_ERR:
            break;
        case TokenType::TOK_EOF:
            break;
        case TokenType::TOK_KEYWORD:
            if (matchToken(token, "def")) {
                auto fun = handleDefineFunction();
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
