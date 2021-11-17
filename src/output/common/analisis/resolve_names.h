/*license*/
#pragma once

#include "sort_element.h"

namespace binred::analisis {

    struct Error {
        std::string errmsg;
        std::shared_ptr<Element> elm;
        std::shared_ptr<token_t> token;

        operator bool() {
            return errmsg.size() != 0;
        }
    };

    struct TypeResolver {
        static Error resolve_read(SortElement& sorted, Record& rec) {
            for (auto& e : sorted.read) {
                auto found = rec.cargos.find(e->name);
                if (found == rec.cargos.end()) {
                    return {
                        "cargo `" + e->name + "` not found; need exists cargo name for read cargo",
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
                auto resolve_transfer_and_cargo = [&](TransferData& data, std::shared_ptr<token_t>& token) -> Error {
                    auto cargo = rec.cargos.find(data.cargoname);
                    if (cargo != rec.cargos.end()) {
                        return {
                            "cargo `" + data.cargoname + "` not found; need exist cargo name",
                            e,
                            token,
                        };
                    }
                    if (cargo->second->base.cargo.lock() != found->second) {
                        return {
                            "cargo `" + cargo->second->name + "` must be derived cargo of `" + found->second->name + "`",
                            e,
                            token,
                        };
                    }
                    data.cargo = cargo->second;
                };
                for (auto& c : e->cmds) {
                    switch (c->kind) {
                        case CommandKind::transfer_direct: {
                            auto direct = castptr<TransferDirect>(c);
                            if (auto err = resolve_transfer_and_cargo(direct->data, direct->token); !err) {
                                return err;
                            }
                            break;
                        }
                        case CommandKind::transfer_if: {
                            auto tif = castptr<TransferIf>(c);
                            if (auto err = resolve_transfer_and_cargo(tif->data, tif->token); !err) {
                                return err;
                            }
                            break;
                        }
                        case CommandKind::transfer_switch: {
                            auto tsw = castptr<TransferSwitch>(c);
                            for (auto& t : tsw->to) {
                                if (auto err = resolve_transfer_and_cargo(t.second, tsw->token); !err) {
                                    return err;
                                }
                            }
                            if (tsw->defaults.cargoname.size()) {
                                if (auto err = resolve_transfer_and_cargo(tsw->defaults, tsw->token); !err) {
                                    return err;
                                }
                            }
                            break;
                        }
                    }
                }
            }
        }

        static Error resolve_cargo(SortElement& sorted, Record& rec) {
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