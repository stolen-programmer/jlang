#include "parser.h"

void Parser::skip(int n, ...) const
{
    va_list args;
    va_start(args, n);

    std::vector<char> space;

    for (auto i = 0; i < n; i++) {
        space.emplace_back(va_arg(args, char));
    }

    char curr;
    while ((curr = fgetc(fp)) != EOF) {
        if (std::ranges::find(space, curr) != space.end()) {
            continue;
        }
        ungetc(curr, fp);
        break;
    }
}

void Parser::skip_space(uint32_t& row, uint32_t& col) const
{

    char curr;
    while ((curr = fgetc(fp)) != EOF) {
        col++;
        if (isspace(curr)) {
            if (curr == '\n') {
                row++;
                col = 0;
            }
            continue;
        }
        ungetc(curr, fp);
        col--;
        break;
    }
}

Token Parser::next_token() const
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
    while ((curr = fgetc(fp)) != EOF) {
        col++;
        content += (curr);

        // 字母 字母数字 标识符 保留字
        if (isalpha(curr)) {

            while (isalnum((curr = fgetc(fp))))
                content += (curr);
            col += content.length() - 1;

            ungetc(curr, fp);

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
            while (isdigit((curr = fgetc(fp)))) {
                content += curr;
            }
            n = content.length() - 1;
            col += n;
            if (curr != '.') {
                ungetc(curr, fp);
                source_location.end_row = row;
                source_location.end_col = col;
                return { TokenType::TOK_INTEGER_LITERAL, content, source_location };
            }

            content += curr;

            while (isdigit((curr = fgetc(fp)))) {
                content += curr;
            }

            col += content.length() - n;

            ungetc(curr, fp);

            source_location.end_row = row;
            source_location.end_col = col;
            return { TokenType::TOK_FLOAT_LITERAL, content, source_location };
        }

        // 字符串
        if (curr == '\"') {
        escape:
            while ((curr = fgetc(fp)) != EOF) {
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
            if ((curr = fgetc(fp)) == '\'') {
                return { TokenType::TOK_ERR, "字符解析出错" };
            }

            content += curr;

            if ((curr = fgetc(fp)) != '\'') {
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

Token Parser::peek_token()
{

    if (!tokens.empty()) {
        return *tokens.begin();
    }

    auto r = next_token();
    tokens.emplace_back(r);

    return r;
}

void Parser::rollback(const Token& token)
{
    tokens.insert(tokens.begin(), token);
}

bool Parser::match(const std::string_view token)
{
    return next_token().content == token;
}

bool Parser::match_token(const Token& token, const std::string_view content)
{
    return token.content == content;
}

void Parser::log_error(TokenError error, Token token)
{
    auto error_ty = type::get(error);
    auto error_en = error_ty.get_enumeration();

    auto type_ty = type::get(token.type);
    auto type_en = type_ty.get_enumeration();

    auto error_name = error_en.value_to_name(error);
    auto error_type = type_en.value_to_name(token.type);

    std::cout << std::format("{} {} start_col: {}", error_name.data(), error_type.data(), token.location.start_col);
}

Token Parser::assert_token(Token token, TokenError error, TokenType ty)
{
    if (token) {
        return std::move(token);
    }

    log_error(error, { .type = ty, .location = token.location });

    exit(static_cast<int>(error));
}

Token Parser::match(const TokenType type)
{
    auto token = next_token();

    if (token.type != type) {
        rollback(token);
        return { .location = token.location };
    }
    return token;
}

Token Parser::match(const TokenType type, const std::string_view content)
{
    auto token = match(type);

    if (token && token.content == content)
        return token;

    rollback(token);
    return { .location = token.location };
}
