/*license*/
#pragma once

#include "../../parse/parser/parse.h"
#include "../output_context.h"
#include "../../calc/get_const.h"
#include "../../calc/trace_expr.h"
#include "../common/get_alias_type.h"

namespace binred {
    struct AliasToCppEnum {
        static std::string get_type(std::int64_t maxvalue, std::int64_t minvalue) {
            return get_alias_type(maxvalue, minvalue, [](int by, bool unsig) {
                switch (by) {
                    case 1:
                        return unsig ? "std::uint8_t" : "std::int8_t";
                    case 2:
                        return unsig ? "std::uint16_t" : "std::int16_t";
                    case 4:
                        return unsig ? "std::uint32_t" : "std::int32_t";
                    case 8:
                        return unsig ? "std::uint64_t" : "std::int64_t";
                    default:
                        return "";
                }
            });
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
