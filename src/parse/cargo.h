/*license*/
#pragma once
#include "element.h"
#include <vector>
#include <memory>
#include <string>
namespace binred {
    enum class ParamType {
        integer,
        uint,
        bit,
        byte,
        custom,
    };

    struct Param {
        Param(ParamType t)
            : type(t) {}
        ParamType type;
        std::string name;
        std::shared_ptr<Condition> if_c;
        std::shared_ptr<Condition> bind_c;
        std::shared_ptr<Value> default_v;
        std::shared_ptr<token> token;
    };

    struct Cargo : Element {
        Cargo()
            : Element(ElementType::cargo) {}
        std::string name;
        std::vector<std::shared_ptr<Param>> params;
    };

    enum class LengthType {
        number,
        referemce,
        expr,
    };

    struct Length {
        Length(LengthType t)
            : type(t) {}
        LengthType type;
    };

    /* struct NumLength : Length {
        int length = 0;
    };

    struct RefLength : Length {
        std::shared_ptr<Cargo> cargo;
        std::string name;
    };*/

    struct ExprLength : Length {
        ExprLength()
            : Length(LengthType::expr) {}
        std::shared_ptr<Expr> expr;
    };

    struct Builtin : Param {
        Builtin(ParamType t)
            : Param(t) {}
        std::shared_ptr<Length> length;
    };

    struct Int : Builtin {
        Int()
            : Builtin(ParamType::integer) {}
    };

    struct UInt : Builtin {
        UInt()
            : Builtin(ParamType::uint) {}
    };

    struct Bit : Builtin {
        Bit()
            : Builtin(ParamType::bit) {}
    };

    struct Byte : Builtin {
        Byte()
            : Builtin(ParamType::byte) {}
    };

    struct Custom : Param {
        Custom()
            : Param(ParamType::custom) {}
        std::string cargoname;
        std::shared_ptr<Cargo> base;
    };
}  // namespace binred
