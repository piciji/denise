
#pragma once

#include <string>
#include <cstdint>
#include <vector>
#include <functional>

struct ExpressionParseError : std::exception {
    std::string descr;

    explicit ExpressionParseError(const std::string& str) : descr(str)  {}

    auto what() const noexcept -> const char* {
        return descr.c_str();
    }
};


class ExpressionParser {
    using Callback = std::function< uint32_t (const std::string& input, int& pos)>;

public:
    auto setExpression(const std::string& str) -> void {
        input = str;
    }

    auto parse() -> bool;
    auto parseSilent() -> bool;

    Callback callback = nullptr;
private:
    std::string input;
    int pos = 0;

    auto skipSpaces() -> void;
    auto match(const std::string& token, const std::string& notToken = "") -> bool;

    auto parseLogicalOr() -> uint32_t;
    auto parseLogicalAnd() -> uint32_t;

    auto parseBitwiseOr() -> uint32_t;
    auto parseBitwiseXor() -> uint32_t;
    auto parseBitwiseAnd() -> uint32_t;

    auto parseEquality() -> uint32_t;
    auto parseRelational() -> uint32_t;
    auto parseShift() -> uint32_t;
    auto parseAdditive() -> uint32_t;
    auto parseMultiplicative() -> uint32_t;
    auto parseUnary() -> uint32_t;
    auto parsePrimary() -> uint32_t;
    auto parseNumber() -> uint32_t;
};
