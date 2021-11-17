/*
    binred - binary reader code generator
    Copyright (c) 2021 on-keyday (https://github.com/on-keyday)
    Released under the MIT license
    https://opensource.org/licenses/mit-license.php
*/

#pragma once

#include "../../parse/parser/parse.h"
#include <extutil.h>
#include "../../calc/trace_expr.h"
#include "../../calc/cast_ptr.h"
#include "../common/make_lambda.h"

namespace binred {
    namespace cpp {
        auto format_alias_and_cargo(Record& rec, std::string& current, Cargo& cargo) {
            auto f = [&](auto self, std::string& ret, std::shared_ptr<Expr> p, TraceMode m) {
                if (p->kind == ExprKind::call) {
                    auto c = castptr<CallExpr>(p);
                    ret += c->v;
                    ret += "(";
                    for (size_t i = 0; i < c->args.size(); i++) {
                        if (i != 0) {
                            ret += ",";
                        }
                        ret += trace_expr(c->args[i], self, m);
                    }
                    ret += ")";
                }
                else if (p->kind == ExprKind::ref) {
                    auto splt = commonlib2::split(p->v, ".");
                    if (splt.size() == 1) {
                        if (current.size()) {
                            if (p->v == current) {
                                ret += "__v_input";
                                return true;
                            }
                        }
                    }
                    else {
                        if (splt.size() == 2) {
                            auto found = rec.aliases.find(splt[0]);
                            if (found != rec.aliases.end()) {
                                if (found->second->baseclass.size()) {
                                    ret += found->second->baseclass;
                                }
                                else {
                                    ret += "std::uint64_t";
                                }
                                ret += "(" + splt[0] + "::" + splt[1] + ")";
                                return true;
                            }
                        }
                        bool self = false;
                        if (splt[0] == cargo.base.selfname) {
                            ret += cargo.base.basename + "::";
                            self = true;
                        }
                        else {
                            ret += splt[0];
                        }
                        for (size_t i = 1; i < splt.size(); i++) {
                            if (!(self && i == 1)) {
                                ret += ".";
                            }
                            ret += "get_" + splt[i] + "()";
                        }
                        return true;
                    }
                }
                return false;
            };
            return make_lambda(f);
        }
    }  // namespace cpp
}  // namespace binred
