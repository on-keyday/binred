/*
    binred - binary reader code generator
    Copyright (c) 2021 on-keyday (https://github.com/on-keyday)
    Released under the MIT license
    https://opensource.org/licenses/mit-license.php
*/

#pragma once
#include "output_context.h"

namespace binred {
    namespace cpp {
        std::string error_enum_class(CppOutContext& ctx) {
            std::string ret = "\nenum class ";
            ret += ctx.error_enum();
            ret += " {\n";
            ret += "none,\n";
            for (auto& i : ctx.enum_v) {
                ret += i;
                ret += ",\n";
            }
            ret += "};\n";
            return ret;
        }
    }  // namespace cpp
}  // namespace binred
