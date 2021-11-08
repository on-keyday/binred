/*license*/
#pragma once

#include "../../parse/parse.h"
#include "../output_context.h"
#include "../../calc/get_const.h"
#include "../../calc/trace_expr.h"
#include "format_alias_and_cargo.h"

namespace binred {
    struct CargoToCppStruct {
        static ExprLength* get_exprlength(std::shared_ptr<Param>& param) {
            auto bin = static_cast<Builtin*>(&*param);
            return static_cast<ExprLength*>(&*bin->length);
        }

        static bool get_typename(std::string& tyname, OutContext& ctx, std::shared_ptr<Param>& param, size_t& bylen) {
            auto type = param->type;
            if (type == ParamType::integer || type == ParamType::uint || type == ParamType::bit) {
                auto lenp = get_exprlength(param);
                auto res = get_const_int<size_t>(lenp->expr);
                if (!res.second) {
                    return false;
                }
                if (type == ParamType::integer || type == ParamType::uint) {
                    tyname = "std::";
                    if (type == ParamType::uint) {
                        tyname += "u";
                    }
                    switch (res.first) {
                        case 1:
                            tyname += "int8_t";
                            break;
                        case 2:
                            tyname += "int16_t";
                            break;
                        case 3:
                        case 4:
                            tyname += "int32_t";
                            break;
                        case 5:
                        case 6:
                        case 7:
                        case 8:
                            tyname += "int64_t";
                            break;
                        default:
                            return false;
                    }
                }
                else {
                    size_t base = res.first;
                    if (base <= 8) {
                        tyname += "std::uint8_t";
                    }
                    else if (base <= 16) {
                        tyname += "std::uint16_t";
                    }
                    else if (base <= 32) {
                        tyname += "std::uint32_t";
                    }
                    else if (base <= 64) {
                        tyname += "std::uint64_t";
                    }
                    else {
                        return false;
                    }
                }
            }
            else if (type == ParamType::byte) {
                auto lenp = get_exprlength(param);
                if (ctx.allow_fixed()) {
                    if (auto got = get_const_int<size_t>(lenp->expr); got.second && got.first != 0) {
                        tyname = "std::uint8_t";
                        bylen = got.first;
                    }
                    else {
                        tyname += ctx.buffer_type();
                    }
                }
                else {
                    tyname += ctx.buffer_type();
                }
            }
            else if (type == ParamType::custom) {
                auto cus = static_cast<Custom*>(&*param);
                tyname = cus->cargoname;
            }
            else {
                return false;
            }
            return true;
        }

        static bool set_default(OutContext& ctx, std::shared_ptr<Param>& param, auto& formatter) {
            if (param->default_v) {
                ctx.write(" = {");
                ctx.write(trace_expr(param->default_v->expr, formatter));
                ctx.write("}");
            }
            return true;
        }

        static bool convert(OutContext& ctx, Cargo& cargo, Record& record) {
            std::string current;
            auto formatter = format_alias_and_cargo(record, current, cargo);
            ctx.write("\nstruct ");
            ctx.write(cargo.name);
            ctx.write(" {\nprivate:\n\n");
            std::string getter, setter;
            for (auto i = 0; i < cargo.params.size(); i++) {
                std::string tyname;
                auto& param = cargo.params[i];
                size_t bylen = 0;
                if (!get_typename(tyname, ctx, param, bylen)) {
                    return false;
                }
                auto& name = param->name;
                current.clear();
                ctx.write(tyname);
                ctx.write(" ");
                ctx.write(name);
                if (bylen != 0) {
                    ctx.write("[");
                    ctx.write(std::to_string(bylen));
                    ctx.write("]");
                }
                if (!set_default(ctx, param, formatter)) {
                    return false;
                }
                ctx.write(";\n\n");
                //ctx.write("public:\n");
                if (param->type == ParamType::byte || param->type == ParamType::custom) {
                    getter += "const ";
                }
                getter += tyname;
                if (bylen != 0) {
                    getter += "*";
                }
                else if (param->type == ParamType::byte || param->type == ParamType::custom) {
                    getter += "&";
                }
                getter += (" get_" + name + "() const {\nreturn " +
                           name + ";\n}\n\n");
                setter += "\n";
                setter += ctx.error_enum();
                setter += " set_" + name + "(const " + tyname;
                if (bylen != 0) {
                    setter += "* __v_input";
                }
                else {
                    setter += "& __v_input";
                }
                setter += ") {\n";
                std::string err = cargo.name + "_" + name;
                current = name;
                if (param->bind_c) {
                    ctx.set_error_enum(err + "_bind");
                    setter += "if (!(" + trace_expr(param->bind_c->expr, formatter) +
                              ")){\nreturn ";
                    setter += ctx.error_enum();
                    setter += "::" + err + "_bind;\n}\n";
                }
                current.clear();
                if (param->type == ParamType::byte && bylen == 0) {
                    ctx.set_error_enum(err + "_length");
                    setter += "if ((" + ctx.length_of_byte("__v_input") + ")!=(";
                    auto bin = static_cast<Builtin*>(&*param);
                    auto lenp = static_cast<ExprLength*>(&*bin->length);
                    setter += trace_expr(lenp->expr, formatter) +
                              ")){\nreturn ";
                    setter += ctx.error_enum();
                    setter += "::" + err + "_length;\n}\n";
                }
                current = name;
                if (param->if_c) {
                    ctx.set_error_enum(err + "_if");
                    setter += "if (!(" + trace_expr(param->if_c->expr, formatter) +
                              ")){\nreturn ";
                    setter += ctx.error_enum();
                    setter += "::" + err + "_if;\n}\n";
                }
                if (bylen != 0) {
                    setter += "::memcpy(this->" +
                              name + ",__v_input," +
                              std::to_string(bylen) +
                              ");\n";
                }
                else {
                    setter += "this->" + name +
                              "=__v_input;\n";
                }
                setter += "return ";
                setter += ctx.error_enum();
                setter += "::none;\n}\n";
            }
            ctx.write("\npublic:\n\n");
            ctx.write(getter);
            ctx.write(setter);
            ctx.write("};\n");
            return true;
        }
    };
}  // namespace binred