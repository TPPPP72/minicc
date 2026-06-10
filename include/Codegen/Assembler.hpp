#pragma once

#include <cstdint>
#include <iostream>
#include <string_view>
#include <type_traits>

class Assembler
{
public:
    static void push(std::string_view dst)
    {
        std::cout << "  push %" << dst << "\n";
    }

    static void pop(std::string_view dst)
    {
        std::cout << "  pop %" << dst << '\n';
    }

    static void label(std::string_view label)
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

    template <typename Val>
    static auto movzb(Val src, std::string_view dst) -> std::enable_if_t<std::is_arithmetic_v<Val>>
    {
        std::cout << "  movzb $" << src << ", %" << dst << '\n';
    }

    static void movzb(std::string_view src, std::string_view dst)
    {
        std::cout << "  movzb %" << src << ", %" << dst << '\n';
    }

    static void neg(std::string_view dst)
    {
        std::cout << "  neg %" << dst << '\n';
    }

    static void sete(std::string_view dst)
    {
        std::cout << "  sete %" << dst << '\n';
    }

    static void setne(std::string_view dst)
    {
        std::cout << "  setne %" << dst << '\n';
    }

    static void setl(std::string_view dst)
    {
        std::cout << "  setl %" << dst << '\n';
    }

    static void setle(std::string_view dst)
    {
        std::cout << "  setle %" << dst << '\n';
    }

    static void setg(std::string_view dst)
    {
        std::cout << "  setg %" << dst << '\n';
    }

    static void setge(std::string_view dst)
    {
        std::cout << "  setge %" << dst << '\n';
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

    template <typename Val>
    static auto cmp(Val src, std::string_view dst) -> std::enable_if_t<std::is_arithmetic_v<Val>>
    {
        std::cout << "  cmp $" << src << ", %" << dst << '\n';
    }

    static void cmp(std::string_view src, std::string_view dst)
    {
        std::cout << "  cmp %" << src << ", %" << dst << '\n';
    }

    static void lea(std::int32_t offset, std::string_view src, std::string_view dst)
    {
        std::cout << "  lea " << offset << "(%" << src << ")" << ", %" << dst << '\n';
    }

    static void je(std::string_view label){
        std::cout << "  je " << label << "\n";
    }

    static void jne(std::string_view label){
        std::cout << "  jne " << label << "\n";
    }

    static void jmp(std::string_view label){
        std::cout << "  jmp " << label << "\n";
    }

    static void call(std::string_view label){
        std::cout << "  call " << label << "\n";
    }

    static void ret()
    {
        std::cout << "  ret\n";
    }
};