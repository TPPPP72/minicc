#pragma once

#include <Diag.hpp>
#include <minicc.hpp>
#include <algorithm>
#include <cctype>
#include <vector>

using namespace std::string_view_literals;

class TokenViewer
{
public:
    TokenViewer(const std::vector<Token> &tokens) : m_tokens(tokens) {}

    const Token getToken() const noexcept
    {
        return m_tokens[index];
    }

    void skipToken()
    {
        ++index;
    }

    void consumeToken(std::string_view content)
    {
        if (content == m_tokens[index].getContent())
        {
            ++index;
            return;
        }

        DiagnosticEngine::errorOnTok(m_tokens[index], "expected '{}'", content);
    }

    bool tryConsumeToken(std::string_view content)
    {
        if (content == m_tokens[index].getContent())
        {
            ++index;
            return true;
        }
        return false;
    }

private:
    const std::vector<Token> &m_tokens;
    std::uint32_t index{0};
};

class Lexer
{
public:
    Lexer() { tokens.reserve(4098); }
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

            if (std::isdigit(source[offset]))
            {
                std::uint32_t len{1};
                while (offset + len < maxlen && std::isdigit(source[offset + len]))
                    ++len;
                tokens.emplace_back(source, TokenType::NUM, offset, len);
                offset += len;
                continue;
            }

            if (isIdentifier1(source[offset]))
            {
                std::uint32_t len{1};
                while (offset + len < maxlen && isIdentifier2(source[offset + len]))
                    ++len;

                if (isKeyword(source.substr(offset, len)))
                    tokens.emplace_back(source, TokenType::KEYWORD, offset, len);
                else
                    tokens.emplace_back(source, TokenType::VAR, offset, len);

                offset += len;
                continue;
            }

            if (offset + 2 < maxlen && isTwoCharOperator(source.substr(offset, 2)))
            {
                tokens.emplace_back(source, TokenType::OPERATOR, offset, 2);
                offset += 2;
                continue;
            }

            if (isOneCharOperator(source.substr(offset, 1)))
            {
                tokens.emplace_back(source, TokenType::OPERATOR, offset, 1);
                offset += 1;
                continue;
            }

            if (isPunctator(source.substr(offset, 1)))
            {
                tokens.emplace_back(source, TokenType::PUNCT, offset, 1);
                offset += 1;
                continue;
            }

            ++offset;
        }

        tokens.emplace_back(source, TokenType::ENDF, offset, 0);
        return TokenViewer(tokens);
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
    std::array<std::string_view, 9> keywords{"char"sv, "short"sv, "int"sv, "float"sv, "double"sv, "main"sv, "if"sv, "while"sv, "for"sv};
    std::vector<Token> tokens;
};