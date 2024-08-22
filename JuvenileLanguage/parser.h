#pragma once

#include "token.h"

#include <cctype>
#include <cstdarg>
#include <cstdio>
#include <vector>

struct Parser {

    FILE* fp = nullptr;

    void skip(int n, ...) const;

    void skip_space(uint32_t& row, uint32_t& col) const;

    Token next_token() const;

    Token peek_token();

    void rollback(const Token& token);

    bool match(const std::string_view token);

    bool match_token(const Token& token, const std::string_view content);

    void log_error(TokenError error, Token token);

    Token assert_token(Token token, TokenError error, TokenType ty);

    Token match(const TokenType type);

    Token match(const TokenType type, const std::string_view content);
};
