/*
    binred - binary reader code generator
    Copyright (c) 2021 on-keyday (https://github.com/on-keyday)
    Released under the MIT license
    https://opensource.org/licenses/mit-license.php
*/

#pragma once
#include "macro.h"
#include "cargo.h"
#include "alias.h"
#include "complex.h"
#include "tree.h"

namespace binred {
    struct Record {
        std::string libname;
        MacroExpander mep;
        std::map<std::string, std::shared_ptr<Cargo>> cargos;
        std::map<std::string, std::shared_ptr<NumberAlias>> aliases;
        std::map<std::string, std::shared_ptr<TypeAlias>> types;
        std::map<std::string, std::shared_ptr<Complex>> complexes;
        Tree tree = {
            {"==", ">", "<", ">=", "<="},
            {"+", "-", "&", "|"},
            {"*", "/", "%"},
        };
        bool expand(TokenReader& r) {
            return mep.expand(r);
        }

        bool add_cargo(const std::string& name, std::shared_ptr<Cargo>& c) {
            return cargos.insert({name, c}).second;
        }

        bool add_alias(const std::string& name, std::shared_ptr<NumberAlias>& c) {
            return aliases.insert({name, c}).second;
        }

        bool add_types(const std::string& name, std::shared_ptr<TypeAlias>& c) {
            return types.insert({name, c}).second;
        }

        Tree& get_tree() {
            return tree;
        }
    };
}  // namespace binred
