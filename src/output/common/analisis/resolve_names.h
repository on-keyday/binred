/*license*/
#pragma once

#include "sort_element.h"

namespace binred {

    struct Error {
        std::string errmsg;
        std::shared_ptr<Element> elm;
        operator bool() {
            return errmsg.size() != 0;
        }
    };
    struct TypeResolver {
        Error resolve_cargo(SortElement& sorted, Record& rec) {
            for (auto& c : sorted.cargo) {
                if (c->base.basename.size()) {
                    auto found = rec.cargos.find(c->base.basename);
                    if (found == rec.cargos.end()) {
                        return {
                            "cargo `" + c->base.basename + "` not found; need exists class name for base class",
                            c,
                        };
                    }
                    c->base.cargo = found->second;
                    found->second->derived[c->name] = c->base.cargo;
                }
                for (auto& p : c->params) {
                    if (p->type == ParamType::custom) {
                        auto custom = castptr<Custom>(p);
                        auto found = rec.cargos.find(custom->cargoname);
                        if (found != rec.cargos.end()) {
                            continue;
                        }
                    }
                }
            }
        }
    };

}  // namespace binred