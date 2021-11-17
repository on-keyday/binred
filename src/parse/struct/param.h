/*license*/
#pragma once
#include "element.h"
#include <memory>

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
        std::shared_ptr<token_t> token;
        bool expand = false;
        bool nilable = false;
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

    struct Cargo;

    struct Custom : Param {
        Custom()
            : Param(ParamType::custom) {}
        std::string cargoname;
        std::weak_ptr<Cargo> cargo;
    };
}  // namespace binred
