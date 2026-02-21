
#include "expressionParser.h"

auto ExpressionParser::parseSilent() -> bool {
    try {
        return parse();
    } catch (ExpressionParseError& e) {
        return false;
    }
}

auto ExpressionParser::parse() -> bool {
    pos = 0;
    uint32_t result = parseLogicalOr();
    skipSpaces();
    if (pos != input.size())
        throw ExpressionParseError("Unexpected input");
    return !!result;
}

inline auto ExpressionParser::skipSpaces() -> void {
    while (pos < input.size() && std::isspace(input[pos]))
        ++pos;
}

auto ExpressionParser::match(const std::string& token, const std::string& notToken) -> bool {
    skipSpaces();
    if (input.compare(pos, token.size(), token) == 0) {
        if (notToken.empty() || (input.substr(pos, notToken.size()) != notToken)) {
            pos += token.size();
            return true;
        }
    }
    return false;
}

auto ExpressionParser::parseLogicalOr() -> uint32_t {
    uint32_t left = parseLogicalAnd();
    while (match("||")) {
        uint32_t rhs = parseLogicalAnd();
        left = (left != 0 || rhs != 0) ? 1u : 0u;
    }
    return left;
}

 auto ExpressionParser::parseLogicalAnd() -> uint32_t {
    uint32_t left = parseBitwiseOr();
    while (match("&&")) {
        uint32_t rhs = parseBitwiseOr();
        left = (left != 0 && rhs != 0) ? 1u : 0u;
    }
    return left;
}

auto ExpressionParser::parseBitwiseOr() -> uint32_t {
    uint32_t left = parseBitwiseXor();
    while (match("|", "||"))
        left |= parseBitwiseXor();
    return left;
}

auto ExpressionParser::parseBitwiseXor() -> uint32_t {
    uint32_t left = parseBitwiseAnd();
    while (match("^"))
        left ^= parseBitwiseAnd();
    return left;
}

auto ExpressionParser::parseBitwiseAnd() -> uint32_t {
    uint32_t left = parseEquality();
    while (match("&", "&&"))
        left &= parseEquality();
    return left;
}

auto ExpressionParser::parseEquality() -> uint32_t {
    uint32_t left = parseRelational();
    while (true) {
        if (match("=="))
            left = (left == parseRelational()) ? 1u : 0u;
        else if (match("!="))
            left = (left != parseRelational()) ? 1u : 0u;
        else
            break;
    }
    return left;
}

auto ExpressionParser::parseRelational() -> uint32_t {
    uint32_t left = parseShift();
    while (true) {
        if (match("<="))
            left = (left <= parseShift()) ? 1u : 0u;
        else if (match(">="))
            left = (left >= parseShift()) ? 1u : 0u;
        else if (match("<"))
            left = (left < parseShift()) ? 1u : 0u;
        else if (match(">"))
            left = (left > parseShift()) ? 1u : 0u;
        else
            break;
    }
    return left;
}

auto ExpressionParser::parseShift() -> uint32_t {
    uint32_t left = parseAdditive();
    while (true) {
        if (match("<<"))
            left <<= parseAdditive();
        else if (match(">>"))
            left >>= parseAdditive();
        else
            break;
    }
    return left;
}

auto ExpressionParser::parseAdditive() -> uint32_t {
    uint32_t left = parseMultiplicative();
    while (true) {
        if (match("+"))
            left += parseMultiplicative();
        else if (match("-"))
            left -= parseMultiplicative();
        else
            break;
    }
    return left;
}

auto ExpressionParser::parseMultiplicative() -> uint32_t {
    uint32_t left = parseUnary();
    while (true) {
        if (match("*"))
            left *= parseUnary();
        else if (match("/")) {
            uint32_t rhs = parseUnary();
            if (rhs == 0)
                throw ExpressionParseError("Division by zero");
            left /= rhs;
        }
        else if (match("%")) {
            uint32_t rhs = parseUnary();
            if (rhs == 0)
                throw ExpressionParseError("Modulo by zero");
            left %= rhs;
        }
        else
            break;
    }
    return left;
}

auto ExpressionParser::parseUnary() -> uint32_t {
    if (match("!"))
        return parseUnary() == 0 ? 1u : 0u;
    if (match("~"))
        return ~parseUnary();
    if (match("+"))
        return parseUnary();
    if (match("-"))
        return static_cast<uint32_t>(-parseUnary());
    return parsePrimary();
}

auto ExpressionParser::parsePrimary() -> uint32_t {
    skipSpaces();

    if (match("(")) {
        uint32_t value = parseLogicalOr();
        if (!match(")"))
            throw ExpressionParseError("Missing ')'");
        return value;
    }

    return parseNumber();
}

auto ExpressionParser::parseNumber() -> uint32_t {
    if (match("true"))
        return 1;

    if (match("false"))
        return 0;

    bool hex = match( "$" );

    const char* start = input.c_str() + pos;
    char* end;

    uint32_t value = std::strtoul(start, &end, hex ? 16 : 10);
    if (start != end) {
        pos += (end - start);
        return value;
    }

    int posBefore = pos;
    if (callback) {
        value = callback( input, pos );
    }

    if (posBefore == pos) {
        throw ExpressionParseError("Expected number");
    }

    return value;
}
