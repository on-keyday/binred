/*license*/
#pragma once

#include "sort_element.h"

namespace binred::analisis {

    struct Error {
        std::string errmsg;
        std::shared_ptr<Element> elm;

        operator bool() {
            return errmsg.size() != 0;
        }
    };

    struct TypeResolver {
        Error resolve_read(SortElement& sorted, Record& rec) {
            for (auto& e : sorted.read) {
                auto found = rec.cargos.find(e->name);
                if (found == rec.cargos.end()) {
                    return {
                        "cargo `" + e->name + "` not found; need exists class name for read class",
                        e,
                    };
                }
                if (!found->second->read.expired()) {
                    return {
                        "cargo `" + e->name + "` already has read statment; need one read for one cargo",
                        e,
                    };
                }
                found->second->read = e;
                e->cargo = found->second;
                for (auto& c : e->cmds) {
                    if (c->kind == CommandKind::transfer_direct) {
                        auto s = castptr<TransferDirect>(c);
                    }
                }
            }
        }

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
                            sorted.unresolved_param.emplace(p);
                            continue;
                        }
                        sorted.unresolved_param.erase(p);
                        custom->cargo = found->second;
                    }
                }
            }
            return {};
        }
    };

}  // namespace binred::analisis