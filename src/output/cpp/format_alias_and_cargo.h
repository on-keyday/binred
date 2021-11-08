#pragma once

#include "../../parse/parse.h"
#include <extutil.h>

namespace binred {
    auto format_alias_and_cargo(Record& rec, std::string& current, Cargo& cargo) {
        return [&](std::string& ret, std::shared_ptr<Expr> p) {
            if (p->kind == ExprKind::ref) {
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
                        if (i != 1 || self != false) {
                            ret += ".";
                        }
                        ret += "get_" + splt[i] + "()";
                    }
                    return true;
                }
            }
            return false;
        };
    }
}  // namespace binred