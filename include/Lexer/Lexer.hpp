#pragma once

#include <Diag/Diag.hpp>
#include <Lexer/Token.hpp>
#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <string_view>
#include <vector>

using namespace std::string_view_literals;

class TokenViewer
{
public:
    TokenViewer() = default;
    TokenViewer(const std::vector<Token> &tokens) : m_tokens(&tokens) {}

    const Token &getToken() const noexcept
    {
        return (*m_tokens)[m_index];
    }

    const Token prev() const noexcept
    {
        return (*m_tokens)[m_index - 1];
    }

    const Token next() const noexcept
    {
        return (*m_tokens)[m_index + 1];
    }

    void skipToken()
    {
        ++m_index;
    }

    void consumeToken(std::string_view content)
    {
        if (content == (*m_tokens)[m_index].getContent())
        {
            ++m_index;
            return;
        }

        DiagnosticEngine::errorOnTok((*m_tokens)[m_index], "expected '{}'", content);
    }

    bool tryConsumeToken(std::string_view content)
    {
        if (content == (*m_tokens)[m_index].getContent())
        {
            ++m_index;
            return true;
        }
        return false;
    }

    bool isToken(std::string_view content)
    {
        if (content == (*m_tokens)[m_index].getContent())
            return true;

        return false;
    }

private:
    const std::vector<Token> *m_tokens;
    std::uint32_t m_index{0};
    friend class RAIITokReverter;
};

class RAIITokReverter
{
public:
    RAIITokReverter(TokenViewer &tok) : m_tok(tok), m_index(tok.m_index)
    {
    }
    ~RAIITokReverter()
    {
        m_tok.m_index = m_index;
    }

private:
    TokenViewer &m_tok;
    std::uint32_t m_index;
};

class Lexer
{
public:
    Lexer() { tokens.reserve(4096); }
    TokenViewer tokenizeFile(char *path)
    {
        std::ifstream ifs(path, std::ios::in);
        if (!ifs.is_open())
            throw std::runtime_error("Failed to open file");

        std::ostringstream ss;
        ss << ifs.rdbuf();
        m_source = ss.str();
        return tokenize(m_source);
    }
    TokenViewer tokenize(std::string_view source)
    {
        std::uint32_t offset{};
        std::uint32_t errlen{};

        std::uint32_t maxlen = source.length();

        while (offset < maxlen)
        {
            if (source[offset] == ' ' || source[offset] == '\t' || source[offset] == '\n')
            {
                ++offset;
                continue;
            }

            // numbers
            if (std::isdigit(source[offset]))
            {
                std::uint32_t len{1};
                while (offset + len < maxlen && std::isdigit(source[offset + len]))
                    ++len;
                tokens.emplace_back(source, TokenKind::NUM, offset, len);
                offset += len;
                continue;
            }

            // escape
            if (source[offset] == '\"')
            {
                std::uint32_t len{1};
                std::string buf;
                while (offset + len < maxlen && source[offset + len] != '\"')
                {
                    char ch = source[offset + len];
                    if (ch == '\0' || ch == '\n')
                    {
                        tokens.emplace_back(source, TokenKind::STR, offset, len - 1);
                        DiagnosticEngine::errorOnTok(tokens.back(), "unclosed string literial");
                    }
                    if (ch == '\\')
                    {
                        ++len;
                        ch = readEscapedChar(source, offset, len);
                        buf += ch;
                        continue;
                    }
                    buf += ch;
                    ++len;
                }
                tokens.emplace_back(source, TokenKind::STR, offset, len + 1);
                tokens.back().string_val = buf;
                offset += (len + 1);
                continue;
            }

            // line comments
            if (source.substr(offset, 2) == "//")
            {
                std::uint32_t len{1};
                while (offset + len < maxlen && source[offset + len] != '\n')
                    ++len;
                offset += len + 1;
                continue;
            }

            // block comments
            if (source.substr(offset, 2) == "/*")
            {
                std::uint32_t len{1};
                while (offset + len < maxlen && source.substr(offset + len, 2) != "*/")
                    ++len;
                offset += len + 2;
                continue;
            }

            // identifier
            if (isIdentifier1(source[offset]))
            {
                std::uint32_t len{1};
                while (offset + len < maxlen && isIdentifier2(source[offset + len]))
                    ++len;

                if (isKeyword(source.substr(offset, len)))
                    tokens.emplace_back(source, TokenKind::KEYWORD, offset, len);
                else
                    tokens.emplace_back(source, TokenKind::IDENT, offset, len);

                offset += len;
                continue;
            }

            if (offset + 2 < maxlen && isTwoCharOperator(source.substr(offset, 2)))
            {
                tokens.emplace_back(source, TokenKind::OPERATOR, offset, 2);
                offset += 2;
                continue;
            }

            if (isOneCharOperator(source.substr(offset, 1)))
            {
                tokens.emplace_back(source, TokenKind::OPERATOR, offset, 1);
                offset += 1;
                continue;
            }

            if (isPunctator(source.substr(offset, 1)))
            {
                tokens.emplace_back(source, TokenKind::PUNCT, offset, 1);
                offset += 1;
                continue;
            }

            ++offset;
        }

        tokens.emplace_back(source, TokenKind::ENDF, offset, 0);
        return TokenViewer(tokens);
    }

private:
    uint8_t readEscapedChar(std::string_view source, std::uint32_t offset, std::uint32_t &len)
    {
        char ch = source[offset + len];

        if (ch == 'x')
        {
            ++len;
            if (!isxdigit(source[offset + len]))
                DiagnosticEngine::error("{} is not a hex escape sequence", source[offset + len]);

            uint8_t temp{};
            while (isxdigit(source[offset + len]))
            {
                temp = (temp << 4) + fromHex(source[offset + len]);
                ++len;
            }
            return temp;
        }

        if (ch >= '0' && ch <= '7')
        {
            int c = ch - '0';
            ++len;

            if (offset + len < source.length() && source[offset + len] >= '0' && source[offset + len] <= '7')
            {
                c = (c << 3) + (source[offset + len] - '0');
                ++len;

                if (offset + len < source.length() && source[offset + len] >= '0' && source[offset + len] <= '7')
                {
                    c = (c << 3) + (source[offset + len] - '0');
                    ++len;
                }
            }
            return static_cast<uint8_t>(c);
        }

        ++len;
        switch (ch)
        {
        case 'a':
            return '\a';
        case 'b':
            return '\b';
        case 't':
            return '\t';
        case 'n':
            return '\n';
        case 'v':
            return '\v';
        case 'f':
            return '\f';
        case 'r':
            return '\r';
        case 'e':
            return 27;
        default:
            return ch;
        }
    }

    uint8_t fromHex(char ch)
    {
        if (isdigit(ch))
            return ch - '0';
        else if (islower(ch))
            return ch - 'a' + 10;
        return ch - 'A' + 10;
    }

private:
    bool isTwoCharOperator(std::string_view content) const
    {
        return std::find(twochar_operator.begin(), twochar_operator.end(), content) != twochar_operator.end();
    }
    bool isOneCharOperator(std::string_view content) const
    {
        return std::find(onechar_operator.begin(), onechar_operator.end(), content) != onechar_operator.end();
    }
    bool isPunctator(std::string_view content) const
    {
        return std::find(punctator.begin(), punctator.end(), content) != punctator.end();
    }
    bool isKeyword(std::string_view content) const
    {
        return std::find(keywords.begin(), keywords.end(), content) != keywords.end();
    }
    bool isIdentifier1(char ch) const
    {
        return std::isalpha(ch) || ch == '_';
    }
    bool isIdentifier2(char ch) const
    {
        return isIdentifier1(ch) || std::isdigit(ch);
    }

private:
    std::array<std::string_view, 16> twochar_operator{"=="sv, "!="sv, ">="sv, "<="sv, "&&"sv, "||"sv, ">>"sv, "<<"sv, "+="sv, "-="sv, "*="sv, "/="sv, "%="sv, "&="sv, "|="sv, "^="sv};
    std::array<std::string_view, 13> onechar_operator{"="sv, "!"sv, ">"sv, "<"sv, "&"sv, "|"sv, "^"sv, "-"sv, "+"sv, "*"sv, "/"sv, "%"sv, "~"sv};
    std::array<std::string_view, 8> punctator{","sv, ";"sv, "{"sv, "}"sv, "("sv, ")"sv, "["sv, "]"sv};
    std::array<std::string_view, 11> keywords{"char"sv, "short"sv, "int"sv, "float"sv, "double"sv, "main"sv, "if"sv, "while"sv, "for"sv, "return"sv, "sizeof"sv};
    std::string m_source;
    std::vector<Token> tokens;
};