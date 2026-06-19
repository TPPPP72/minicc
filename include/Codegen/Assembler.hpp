#pragma once

#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <variant>
#include <stdexcept>
#include <format>

struct Reg { std::string_view name; };
struct Imm { int64_t val; };
struct Mem {
    std::string_view base;
    std::variant<int32_t, std::string_view> offset{0};
};

class Assembler
{
private:
    inline static std::ofstream out_;

    static std::string format_op(Reg r) { return std::format("%{}", r.name); }
    static std::string format_op(Imm i) { return std::format("${}", i.val); }
    static std::string format_op(Mem m) {
        if (std::holds_alternative<int32_t>(m.offset)) {
            int32_t off = std::get<int32_t>(m.offset);
            return off == 0 ? std::format("(%{})", m.base) : std::format("{}(%{})", off, m.base);
        } else {
            return std::format("{}(%{})", std::get<std::string_view>(m.offset), m.base);
        }
    }

public:
    Assembler() = delete;

    static void initialize(std::string_view path) {
        out_.open(std::string(path));
        if (!out_.is_open()) throw std::runtime_error("Failed to open file");
    }

    static void close() { if (out_.is_open()) out_.close(); }

    static std::ostream& stream() { return out_.is_open() ? out_ : std::cout; }

    static void directive(std::string_view dir) { stream() << std::format("  .{}\n", dir); }
    static void globl(std::string_view name) { stream() << std::format("  .globl {}\n", name); }
    static void label(std::string_view name) { stream() << std::format("{}:\n", name); }
    static void quad(int64_t val) { stream() << std::format("  .quad {}\n", val); }
    static void long_val(int32_t val) { stream() << std::format("  .long {}\n", val); }
    static void zero(size_t size) { stream() << std::format("  .zero {}\n", size); }
    static void string_lit(std::string_view escaped) { stream() << std::format("  .string \"{}\"\n", escaped); }

    template<typename T> static void push(T op) { stream() << std::format("  push {}\n", format_op(op)); }
    template<typename T> static void pop(T op)  { stream() << std::format("  pop {}\n", format_op(op)); }
    template<typename T> static void neg(T op)  { stream() << std::format("  neg {}\n", format_op(op)); }
    template<typename T> static void sete(T op) { stream() << std::format("  sete {}\n", format_op(op)); }
    template<typename T> static void setne(T op){ stream() << std::format("  setne {}\n", format_op(op)); }
    template<typename T> static void setl(T op) { stream() << std::format("  setl {}\n", format_op(op)); }
    template<typename T> static void setle(T op){ stream() << std::format("  setle {}\n", format_op(op)); }
    template<typename T> static void setg(T op) { stream() << std::format("  setg {}\n", format_op(op)); }
    template<typename T> static void setge(T op){ stream() << std::format("  setge {}\n", format_op(op)); }

    template<typename Src, typename Dst>
    static void mov(Src src, Dst dst) { stream() << std::format("  mov {}, {}\n", format_op(src), format_op(dst)); }

    template<typename Src, typename Dst>
    static void movzb(Src src, Dst dst) { stream() << std::format("  movzb {}, {}\n", format_op(src), format_op(dst)); }
    
    template<typename Src, typename Dst>
    static void add(Src src, Dst dst) { stream() << std::format("  add {}, {}\n", format_op(src), format_op(dst)); }

    template<typename Src, typename Dst>
    static void sub(Src src, Dst dst) { stream() << std::format("  sub {}, {}\n", format_op(src), format_op(dst)); }

    template<typename Src, typename Dst>
    static void imul(Src src, Dst dst) { stream() << std::format("  imul {}, {}\n", format_op(src), format_op(dst)); }

    template<typename Src, typename Dst>
    static void cmp(Src src, Dst dst) { stream() << std::format("  cmp {}, {}\n", format_op(src), format_op(dst)); }

    template<typename Src, typename Dst>
    static void lea(Src src, Dst dst) { stream() << std::format("  lea {}, {}\n", format_op(src), format_op(dst)); }

    static void movsbq(Mem src, Reg dst) { stream() << std::format("  movsbq {}, {}\n", format_op(src), format_op(dst)); }

    static void idiv(Reg op) { stream() << "  cqo\n" << std::format("  idiv {}\n", format_op(op)); }
    static void jmp(std::string_view label)  { stream() << std::format("  jmp {}\n", label); }
    static void je(std::string_view label)   { stream() << std::format("  je {}\n", label); }
    static void jne(std::string_view label)  { stream() << std::format("  jne {}\n", label); }
    static void call(std::string_view label) { stream() << std::format("  call {}\n", label); }
    static void ret() { stream() << "  ret\n"; }
};