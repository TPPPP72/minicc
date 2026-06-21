#pragma once

#include <Lexer/Token.hpp>
#include <format>
#include <iostream>

namespace Color
{
constexpr std::string_view RESET      = "\033[0m";
constexpr std::string_view BOLD       = "\033[1m";
constexpr std::string_view RED        = "\033[31m";
constexpr std::string_view GREEN      = "\033[32m";
constexpr std::string_view CYAN       = "\033[36m";
constexpr std::string_view GRAY       = "\033[90m";
constexpr std::string_view BOLD_RED   = "\033[1;31m";
constexpr std::string_view BOLD_WHITE = "\033[1;37m";
} // namespace Color

class DiagnosticEngine
{
public:
    template <typename... Args>
    [[noreturn]] static void error(std::string_view fmt, Args &&...args)
    {
        std::string message = std::vformat(fmt, std::make_format_args(args...));
        std::cerr << std::format("{}{}: {}{}{}\n", Color::BOLD_RED, "Error",
                                 Color::BOLD_WHITE, message, Color::RESET);
        std::exit(1);
    }

    template <typename... Args>
    [[noreturn]] static void errorOnTok(const Token &token, std::string_view fmt,
                                        Args &&...args)
    {
        std::string_view source = token.source;
        std::uint32_t loc       = token.loc;
        std::uint32_t len       = token.len;

        std::uint32_t line_num = token.line_num;
        std::uint32_t col_num  = token.col_num;

        size_t line_start = source.rfind('\n', loc);
        line_start        = (line_start == std::string_view::npos) ? 0 : line_start + 1;

        size_t line_end = source.find('\n', loc);
        if (line_end == std::string_view::npos)
            line_end = source.length();

        std::string_view current_line = source.substr(line_start, line_end - line_start);

        std::string error_msg = std::vformat(fmt, std::make_format_args(args...));

        std::uint32_t caret_len = (len > 0) ? len : 1;
        std::string carets      = std::format("{}{}{}", Color::BOLD_RED,
                                              std::string(caret_len, '^'), Color::RESET);

        std::cerr << std::format("{}{}{} at line {}, col {}: {}{}{}\n",
                                 Color::BOLD_RED, "Error", Color::RESET, line_num,
                                 col_num, Color::BOLD_WHITE, error_msg,
                                 Color::RESET);

        std::cerr << std::format("{}{:4} | {}{}\n", Color::GRAY, line_num,
                                 Color::RESET, current_line);

        std::cerr << std::format("{}{:4} | {}{:>{}}{}\n", Color::GRAY, "",
                                 Color::RESET, "", col_num - 1, carets);

        std::exit(1);
    }
};