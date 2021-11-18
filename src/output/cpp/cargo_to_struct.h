/*
    binred - binary I/O code generator
    Copyright (c) 2021 on-keyday (https://github.com/on-keyday)
    Released under the MIT license
    https://opensource.org/licenses/mit-license.php
*/

#pragma once

#include "../../parse/parser/parse.h"
#include "output_context.h"
#include "../../calc/get_const.h"
#include "../../calc/trace_expr.h"
#include "format_alias_and_cargo.h"
#include "code_element.h"

namespace binred {
    namespace cpp {
        struct CargoToCppStruct {
            static ExprLength* get_exprlength(std::shared_ptr<Param>& param) {
                return castptr<ExprLength>(castptr<Builtin>(param)->length);
            }

            static bool get_typename(std::string& tyname, CppOutContext& ctx, std::shared_ptr<Param>& param, size_t& bylen, Record& record) {
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
                    auto cus = castptr<Custom>(param);
                    if (record.cargos.count(cus->cargoname)) {
                        tyname = cus->cargoname;
                    }
                    else if (auto found = record.complexes.find(cus->cargoname); found != record.complexes.end()) {
                        auto& pat = found->second->pattern;
                        if (auto cpp = pat.find("cpp"); cpp != pat.end()) {
                            tyname = cpp->second.pattern;
                        }
                        else {
                            return false;
                        }
                    }
                }
                else {
                    return false;
                }
                return true;
            }

            static bool set_default(std::string& def, std::shared_ptr<Param>& param, auto& formatter) {
                if (param->default_v) {
                    def += " = {" + trace_expr(param->default_v->expr, formatter) + "}";
                }
                return true;
            }

            static bool get_definitions(CppOutContext& ctx, std::string& def, std::string& getter, std::string& setter, std::shared_ptr<Param>& param, Cargo& cargo, Record& record) {
                std::string current;
                auto formatter = format_alias_and_cargo(record, current, cargo);
                std::string tyname;
                size_t bylen = 0;
                if (!get_typename(tyname, ctx, param, bylen, record)) {
                    return false;
                }
                auto& name = param->name;
                def += tyname + " " + name;
                if (bylen != 0) {
                    def += "[" + std::to_string(bylen) + "]";
                }
                if (!set_default(def, param, formatter)) {
                    return false;
                }
                def += ";\n\n";
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
                    write_if(setter, write_not(trace_expr(param->bind_c->expr, formatter)));
                    write_beginblock(setter);
                    write_return(setter, ctx.error_enum() + "::" + err + "_bind");
                    write_endblock(setter);
                }
                current.clear();
                if (param->type == ParamType::byte && bylen == 0) {
                    ctx.set_error_enum(err + "_length");
                    auto lenp = get_exprlength(param);
                    write_if(setter, write_noteq(ctx.length_of_byte("__v_input"), trace_expr(lenp->expr, formatter)));
                    write_beginblock(setter);
                    write_return(setter, ctx.error_enum() + "::" + err + "_length");
                    write_endblock(setter);
                }
                current = name;
                if (param->if_c) {
                    ctx.set_error_enum(err + "_if");
                    write_if(setter, write_not(trace_expr(param->if_c->expr, formatter)));
                    write_beginblock(setter);
                    write_return(setter, ctx.error_enum() + "::" + err + "_if");
                    write_endblock(setter);
                }
                if (bylen != 0) {
                    write_call(setter, "::memcpy", "this->" + name, "__v_input", std::to_string(bylen)) += ";\n";
                }
                else {
                    setter += "this->" + name +
                              "=__v_input;\n";
                }
                write_return(setter, ctx.error_enum() + "::none");
                write_endblock(setter);
                return true;
            }

            static bool convert(CppOutContext& ctx, Cargo& cargo, Record& record) {
                std::string def, getter, setter;
                for (auto i = 0; i < cargo.params.size(); i++) {
                    if (!get_definitions(ctx, def, getter, setter, cargo.params[i], cargo, record)) {
                        return false;
                    }
                }
                ctx.write("\nstruct ");
                ctx.write(cargo.name);
                if (cargo.base.basename.size()) {
                    ctx.write(" : ");
                    ctx.write(cargo.base.basename);
                }
                ctx.write(" {\nprivate:\n\n");
                ctx.write(def);
                ctx.write("\npublic:\n\n");
                ctx.write(getter);
                ctx.write(setter);
                ctx.write("};\n");
                return true;
            }
        };
    }  // namespace cpp
}  // namespace binred
