/*
    binred - binary I/O code generator
    Copyright (c) 2021 on-keyday (https://github.com/on-keyday)
    Released under the MIT license
    https://opensource.org/licenses/mit-license.php
*/

#pragma once
#include "element.h"
#include <vector>
#include <memory>
#include <string>
#include "param.h"
namespace binred {

    struct Cargo;

    struct BaseInfo {
        std::string selfname;
        std::string basename;
        std::weak_ptr<Cargo> cargo;
    };

    struct Read;
    struct Write;

    struct Cargo : Element {
        Cargo(std::shared_ptr<token_t>&& tok)
            : Element(std::move(tok), ElementType::cargo) {}
        std::string name;
        std::vector<std::shared_ptr<Param>> params;
        BaseInfo base;
        bool expanded = false;

        std::map<std::string, std::weak_ptr<Cargo>> derived;

        std::weak_ptr<Read> read;
        std::weak_ptr<Write> write;
    };

}  // namespace binred
