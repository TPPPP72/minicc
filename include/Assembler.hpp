#pragma once

#include <iostream>
#include <string_view>
#include <type_traits>

class Assembler
{
public:
    static void push()
    {
        std::cout << "  push %rax\n";
    }

    static void pop(std::string_view dst)
    {
        std::cout << "  pop %" << dst << '\n';
    }

    static void label(std::string label)
    {
        std::cout << label << ":\n";
    }

    template <typename Val>
    static auto mov(Val src, std::string_view dst) -> std::enable_if_t<std::is_arithmetic_v<Val>>
    {
        std::cout << "  mov $" << src << ", %" << dst << '\n';
    }

    static void mov(std::string_view src, std::string_view dst)
    {
        std::cout << "  mov %" << src << ", %" << dst << '\n';
    }

    static void neg(std::string_view dst)
    {
        std::cout << "neg %" << dst << '\n';
    }

    template <typename Val>
    static auto add(Val src, std::string_view dst) -> std::enable_if_t<std::is_arithmetic_v<Val>>
    {
        std::cout << "  add $" << src << ", %" << dst << '\n';
    }

    static void add(std::string_view src, std::string_view dst)
    {
        std::cout << "  add %" << src << ", %" << dst << '\n';
    }

    template <typename Val>
    static auto sub(Val src, std::string_view dst) -> std::enable_if_t<std::is_arithmetic_v<Val>>
    {
        std::cout << "  sub $" << src << ", %" << dst << '\n';
    }

    static void sub(std::string_view src, std::string_view dst)
    {
        std::cout << "  sub %" << src << ", %" << dst << '\n';
    }

    template <typename Val>
    static auto imul(Val src, std::string_view dst) -> std::enable_if_t<std::is_arithmetic_v<Val>>
    {
        std::cout << "  imul $" << src << ", %" << dst << '\n';
    }

    static void imul(std::string_view src, std::string_view dst)
    {
        std::cout << "  imul %" << src << ", %" << dst << '\n';
    }

    template <typename Val>
    static auto idiv(Val src, std::string_view dst) -> std::enable_if_t<std::is_arithmetic_v<Val>>
    {
        std::cout << "  cqo\n";
        std::cout << "  idiv $" << src << ", %" << dst << '\n';
    }

    static void idiv(std::string_view src, std::string_view dst)
    {
        std::cout << "  cqo\n";
        std::cout << "  idiv %" << src << ", %" << dst << '\n';
    }

    static void ret()
    {
        std::cout << "  ret\n";
    }
};