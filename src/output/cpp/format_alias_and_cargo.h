#pragma once

#include "../../parse/parse.h"
#include <extutil.h>

namespace binred {
    auto format_alias_and_cargo(Record& rec) {
        return [&](std::string& ret, std::shared_ptr<Expr> p) {
            if (p->kind == ExprKind::ref) {
                auto splt = commonlib2::split(p->v, ".");
                if (splt.size() == 1) {
                    return false;
                }
                else if (splt.size() == 2) {
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
            }
            return false;
        };
    }
}  // namespace binred