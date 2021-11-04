/*license*/
#pragma once

#include "../../parse/parse.h"
#include "../output_context.h"
#include "../../calc/get_const.h"
#include "../../calc/trace_expr.h"

namespace binred {
    struct CargoToCppStruct {
        static bool convert(OutContext& ctx, Cargo& cargo) {
            ctx.write("\nstruct ");
            ctx.write(cargo.name);
            ctx.write(" {\n");
            for (auto i = 0; i < cargo.params.size(); i++) {
                std::string tyname;
                auto& param = cargo.params[i];
                auto type = param->type;
                if (type == ParamType::integer || type == ParamType::uint || type == ParamType::bit) {
                    auto bin = static_cast<Builtin*>(&*param);
                    auto lenp = static_cast<ExprLength*>(&*bin->length);
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
                    tyname += ctx.buffer_type();
                }
                else {
                    return false;
                }
                auto& name = param->name;
                ctx.write("private:\n");
                ctx.write(tyname);
                ctx.write(" ");
                ctx.write(name);
                ctx.write(";\n\n");
                ctx.write("public:\n");
                ctx.write(tyname);
                ctx.write(" get_");
                ctx.write(name);
                ctx.write("() const {\nreturn ");
                ctx.write(name);
                ctx.write(";\n}\n\n");
                ctx.write(ctx.error_enum());
                ctx.write(" set_");
                ctx.write(name);
                ctx.write("(const ");
                ctx.write(tyname);
                ctx.write("& __v_input");
                ctx.write(") {\n");
                std::string err = cargo.name + "_" + name;
                if (type == ParamType::byte) {
                    ctx.set_error_enum(err + "_length");
                    ctx.write("if ((");
                    ctx.write(ctx.length_of_byte("__v_input"));
                    ctx.write(")!=(");
                    auto bin = static_cast<Builtin*>(&*param);
                    auto lenp = static_cast<ExprLength*>(&*bin->length);
                    ctx.write(trace_expr(lenp->expr));
                    ctx.write(")){\nreturn ");
                    ctx.write(ctx.error_enum());
                    ctx.write("::" + err + "_length");
                    ctx.write(";\n}\n");
                }
                if (param->bind_c) {
                    ctx.set_error_enum(err + "_bind");
                    ctx.write("if (!(");
                    auto result = trace_expr(param->bind_c->expr);
                    result = std::regex_replace(result, std::regex(name), "__v_input");
                    ctx.write(result);
                    ctx.write(")){\nreturn ");
                    ctx.write(ctx.error_enum());
                    ctx.write("::" + err + "_bind");
                    ctx.write(";\n}\n");
                }
                if (param->if_c) {
                    ctx.set_error_enum(err + "_if");
                    ctx.write("if (!(");
                    auto result = trace_expr(param->if_c->expr);
                    result = std::regex_replace(result, std::regex(name), "__v_input");
                    ctx.write(result);
                    ctx.write(")){\nreturn ");
                    ctx.write(ctx.error_enum());
                    ctx.write("::" + err + "_if");
                    ctx.write(";\n}\n");
                }
                ctx.write("this->");
                ctx.write(name);
                ctx.write("=__v_input;\n");
                ctx.write("}\n");
            }
            ctx.write("};\n");
            return true;
        }
    };
}  // namespace binred