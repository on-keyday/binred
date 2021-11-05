/*license*/
#pragma once
#include "macro.h"
#include "cargo.h"
#include "alias.h"

namespace binred {
    struct Record {
        MacroExpander mep;
        std::map<std::string, std::shared_ptr<Cargo>> cargos;
        std::map<std::string, std::shared_ptr<Alias>> aliases;
        bool expand(TokenReader& r) {
            return mep.expand(r);
        }

        bool add_cargo(const std::string& name, std::shared_ptr<Cargo>& c) {
            return cargos.insert({name, c}).second;
        }

        bool add_alias(const std::string& name, std::shared_ptr<Alias>& c) {
            return aliases.insert({name, c}).second;
        }
    };
}  // namespace binred