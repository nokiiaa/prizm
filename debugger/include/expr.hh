#pragma once

#include <map>
#include <string>
#include <link.hh>

class ExprParser
{
private:
    const char *Data;
    const std::map<std::string, i64> &Context;
public:
    inline void SkipWhitespace() {
        while (*Data && (*Data == ' ' || *Data == '\t' || *Data == '\r' || *Data == '\n'))
            Data++;
    }
    inline ExprParser(const std::map<std::string, i64>& context,
        const char *data) : Context(context), Data(data) {}
    inline bool Operator(std::string op) {
        if (*Data == '\0') return false;
        SkipWhitespace();
        if (op.size() == 1) {
            /* Pretend this is maximal munch by keeping
               an array of operators that may be continued
               after their first characters are matched,
               and checks if the second character also matches,
               in which case this is not the operator that is wanted */
            static const char LongOps[14 + 1] = ">=<=||&&==>><<";
            for (int i = 0; i < 7; i++)
                if (LongOps[i * 2] == op[0] &&
                    *Data == op[0] &&
                    Data[1] == LongOps[i * 2 + 1])
                    return false;
        }
        for (int i = 0; i < op.size(); i++)
            if (Data[i] != op[i])
                return false;
        Data += op.size();
        return true;
    }
    inline bool Name(std::string &out) {
        SkipWhitespace();
        int i;
        for (out = "", i = 0;
            *Data == '_' ||
            std::tolower(*Data) >= 'a' && std::tolower(*Data) <= 'z' ||
            i && *Data >= '0' && *Data <= '9'; i++)
            out += *Data++;
        return i;
    }
    inline bool Int(u64 &out) {
        SkipWhitespace();
        out = 0;
        int i;
        /* Hex number */
        if (Data[0] == '0' && std::tolower(Data[1]) == 'x') {
            Data += 2;
            char ch;
            /* (x) >= 'a' == 1 => (x) - '0' - ('a' - '0' - 10) = (x) - 'a' + 10 */
            /* (x) >= 'a' == 0 => (x) - '0' */
            #define IS_HEX(x) ((x) >= '0' && (x) <= '9' || (x) >= 'a' && (x) <= 'f')
            #define HEX_TO_INT(x) ((x) - '0' - ((x) >= 'a') * ('a' - '0' - 10))
            for (i = 0; ch = std::tolower(*Data), IS_HEX(ch); i++, Data++)
                out <<= 4, out |= HEX_TO_INT(ch);
            #undef HEX_TO_INT
            #undef IS_HEX
        }
        /* Binary number */
        else if (Data[0] == '0' && std::tolower(Data[1]) == 'b') {
            Data += 2;
            for (i = 0; *Data == '0' || *Data == '1'; i++)
                out <<= 1, out |= *Data++ - '0';
        }
        else for (i = 0; *Data >= '0' && *Data <= '9'; i++)
            out *= 10, out += *Data++ - '0';
        return i;
    }
    inline i64 LiteralExpr() {
        if (Operator("(")) {
            i64 val = BinaryExpr();
            if (!Operator(")"))
                throw new std::runtime_error("Syntax error: expected ')'");
            return val;
        }
        std::string name; u64 out;
        if (Name(name)) {
            /* Read/write memory functions */
            if (name == "read8"  || name == "read16"  || name == "read32"  || name == "read64"  ||
                name == "write8" || name == "write16" || name == "write32" || name == "write64") {
                if (!Operator("(")) throw new std::runtime_error("Syntax error: expected '('");
                i64 addr = BinaryExpr();
                i64 val = 0;
                /* write function requires another argument */
                if (name[0] == 'w') {
                    if (!Operator(",")) throw new std::runtime_error("Syntax error: expected ','");
                    val = BinaryExpr();
                }
                if (!Operator(")")) throw new std::runtime_error("Syntax error: expected ')'");
                /* Just a dumb compact way of extracting the size from the last digit of the name */
                int sizes[] = { 4, 8, 2, 1 };
                int size = sizes[(name[name.size() - 1] - '2') / 2];
                if (name[0] == 'r')
                    Link::ReadVirtualMem((uptr)addr, &val, size);
                else
                    Link::WriteVirtualMem((uptr)addr, &val, size);
                return val;
            }
            /* Byte swap functions */
            else if (name == "bswap16" || name == "bswap32" || name == "bswap64") {
                if (!Operator("(")) throw new std::runtime_error("Syntax error: expected '('");
                i64 val = BinaryExpr();
                if (!Operator(")")) throw new std::runtime_error("Syntax error: expected ')'");
                char sz = name[name.size() - 1];
                /* 16-bit byteswap */
                if (sz == '6')
                    return (val >> 8) & 0xFF | (val & 0xFF) << 8;
                /* 32-bit byteswap */
                else if (sz == '2') {
                    val = (val & 0x0000FFFF) << 16 | (val & 0xFFFF0000) >> 16;
                    val = (val & 0x00FF00FF) << 8  | (val & 0xFF00FF00) >> 8;
                    return val;
                }
                /* 64-bit byteswap */
                else if (sz == '4') {
                    val = (val & 0x00000000FFFFFFFF) << 32 | (val & 0xFFFFFFFF00000000) >> 32;
                    val = (val & 0x0000FFFF0000FFFF) << 16 | (val & 0xFFFF0000FFFF0000) >> 16;
                    val = (val & 0x00FF00FF00FF00FF) << 8  | (val & 0xFF00FF00FF00FF00) >> 8;
                    return val;
                }
            }
            auto it = Context.find(name);
            if (it == Context.end())
                throw new std::runtime_error("Error: variable not defined");
            return it->second;
        }
        else if (Int(out))
            return out;
        throw new std::runtime_error("Error: unexpected token");
    }
    inline i64 UnaryExpr() {
        if (Operator("-")) return -UnaryExpr();
        if (Operator("!")) return !UnaryExpr();
        if (Operator("~")) return ~UnaryExpr();
        else if (Operator("+")) return UnaryExpr();
        return LiteralExpr();
    }
    inline i64 BinaryExpr(int level = 0) {
        const int NumberLevels = 10;
        static std::vector<std::pair<int, std::string>> BinaryLevels[NumberLevels] = {
            {{0, "||"}},
            {{1, "&&"}},
            {{2, "|"}},
            {{3, "^"}},
            {{4, "&"}},
            {{5, "=="}, {6, "!="}},
            {{7, "<"}, {8, "<="}, {9, ">"}, {10, ">="}},
            {{11, "<<"}, {12, ">>"}},
            {{13, "+"}, {14, "-"}},
            {{15, "*"}, {16, "/"}, {17, "%"}}
        };

        if (level >= NumberLevels)
            return UnaryExpr();
        i64 left = BinaryExpr(level + 1);
        bool matched;
        do {
            matched = false;
            for (const auto &op : BinaryLevels[level]) {
                if (Operator(op.second)) {
                    i64 right = BinaryExpr(level + 1);
                    matched = true;
                    if ((op.first == 16 || op.first == 17) && !right)
                        throw new std::runtime_error("Error: attempt to divide by zero");
                    switch (op.first) {
                    /* logical or  */ case 0:  left = left || right; break;
                    /* logical and */ case 1:  left = left && right; break;
                    /* bitwise or  */ case 2:  left |= right; break;
                    /* bitwise xor */ case 3:  left ^= right; break;
                    /* logical and */ case 4:  left &= right; break;
                    /* equal       */ case 5:  left = left == right; break;
                    /* not equal   */ case 6:  left = left != right; break;
                    /* less than   */ case 7:  left = left <  right; break;
                    /* lt or equal */ case 8:  left = left <= right; break;
                    /* greater than*/ case 9:  left = left >  right; break;
                    /* gt or equal */ case 10: left = left >= right; break;
                    /* shift left  */ case 11: left <<= right; break;
                    /* shift right */ case 12: left >>= right; break;
                    /* add         */ case 13: left += right; break;
                    /* subtract    */ case 14: left -= right; break;
                    /* multiply    */ case 15: left *= right; break;
                    /* divide      */ case 16: left /= right; break;
                    /* modulo      */ case 17: left %= right; break;
                    }
                }
            }
        } while (matched);
        return left;
    }
    inline i64 Evaluate() {
        i64 val = BinaryExpr();
        if (*Data)
            throw new std::runtime_error("Error: garbage at end of expression");
        return val;
    }
};