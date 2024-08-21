#pragma once

#include "token.h"

template <>
struct std::formatter<SourceLocation> {
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.end();
    }

    template <typename FormatContext>
    auto format(const SourceLocation& location, FormatContext& ctx) const
    {
        return std::format_to(ctx.out(), R"(SourceLocation ( start_row: {}, start_col: {}, end_row: {}, end_col: {} ))",
            location.start_row, location.start_col, location.end_row, location.end_col);
    }
};

template <>
struct std::formatter<TokenError> {
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const TokenError& token, FormatContext& ctx) const
    {
        auto t = type::get<TokenError>().get_enumeration();
        return std::format_to(ctx.out(), R"(TokenError ( {} ))",
            t.value_to_name(token).data());
    }
};

template <>
struct std::formatter<TokenType> {
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const TokenType& token, FormatContext& ctx) const
    {
        auto t = type::get<TokenType>().get_enumeration();
        return std::format_to(ctx.out(), R"(TokenType ( {} ))",
            t.value_to_name(token).data());
    }
};

template <>
struct std::formatter<Token> {
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const Token& token, FormatContext& ctx) const
    {
        auto t = type::get<TokenType>().get_enumeration();
        return std::format_to(ctx.out(), R"(Token ( type: {}, content: {}, location: {} ))",
            token.type, token.content, token.location);
    }
};

template <>
struct std::formatter<FunctionAST> {
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const FunctionAST& ast, FormatContext& ctx) const
    {

        return std::format_to(ctx.out(), R"(def {}({}) {} )",
            ast.name, ast.args, ast.result.content);
    }
};

template <>
struct std::formatter<std::vector<std::pair<Token, Token>>> {
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const std::vector<std::pair<Token, Token>>& args, FormatContext& ctx) const
    {

        for (auto iter = args.begin(); iter != args.end() - 1; ++iter) {
            std::format_to(ctx.out(), "{} {},", iter->first.content, iter->second.content);
        }

        return std::format_to(ctx.out(), R"({} {})",
            args.rbegin()->first.content, args.rbegin()->second.content);
    }
};
