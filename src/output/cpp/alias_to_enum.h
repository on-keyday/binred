#pragma once

#include "../../parse/parse.h"
#include "../output_context.h"
#include "../../calc/get_const.h"
#include "../../calc/trace_expr.h"

namespace binred {
    struct AliasToCppEnum {
        static std::string get_type(std::int64_t maxvalue, std::int64_t minvalue) {
            if (minvalue >= 0) {
                if (maxvalue <= 0xff) {
                    return "std::uint8_t";
                }
                else if (maxvalue <= 0xffff) {
                    return "std::uint16_t";
                }
                else if (maxvalue <= 0xffffffff) {
                    return "std::uint32_t";
                }
                else {
                    return "std::uint64_t";
                }
            }
            else {
                if (maxvalue <= 127 && minvalue >= -128) {
                    return "std::int8_t";
                }
                else if (maxvalue <= 32767 && minvalue >= -32767) {
                    return "std::int16_t";
                }
                else if (maxvalue <= 2147483647 && minvalue >= -2147483648) {
                    return "std::int32_t";
                }
                else {
                    return "std::int64_t";
                }
            }
        }

        static bool convert(OutContext& ctx, Alias& als) {
            std::int64_t maxvalue = 0;
            std::int64_t minvalue = 0;
            std::string tmpwrite;
            for (auto& i : als.alias) {
                std::int64_t v = 0;
                auto e = get_const_int<std::int64_t>(i.second->expr);
                if (!e.second) {
                    return false;
                }
                tmpwrite += i.first;
                tmpwrite += " = ";
                tmpwrite += std::to_string(e.first);
                tmpwrite += ",\n";
                if (e.first > maxvalue) {
                    maxvalue = e.first;
                }
                if (e.first < minvalue) {
                    minvalue = e.first;
                }
            }
            ctx.write("\nenum class ");
            ctx.write(als.name);
            ctx.write(" : ");
            auto type = get_type(maxvalue, minvalue);
            ctx.write(type);
            ctx.write("{\n");
            ctx.write(tmpwrite);
            ctx.write("};\n");
            als.baseclass = type;
            return true;
        }
    };
}  // namespace binred
