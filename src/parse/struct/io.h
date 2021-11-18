/*
    binred - binary I/O code generator
    Copyright (c) 2021 on-keyday (https://github.com/on-keyday)
    Released under the MIT license
    https://opensource.org/licenses/mit-license.php
*/

#pragma once
#include "element.h"
#include <vector>
#include "param.h"
#include "command.h"

namespace binred {

    struct IOElement : Element {
        using Element::Element;
        std::string name;
        std::vector<std::shared_ptr<Param>> args;
        std::vector<std::shared_ptr<Command>> cmds;
        std::weak_ptr<Cargo> cargo;
    };

    struct Read : IOElement {
        Read(std::shared_ptr<token_t>&& tok)
            : IOElement(std::move(tok), ElementType::read) {}
    };

    struct Write : IOElement {
        Write(std::shared_ptr<token_t>&& tok)
            : IOElement(std::move(tok), ElementType::write) {}
    };
}  // namespace binred
