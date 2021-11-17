/*
    binred - binary reader code generator
    Copyright (c) 2021 on-keyday (https://github.com/on-keyday)
    Released under the MIT license
    https://opensource.org/licenses/mit-license.php
*/

#pragma once
#include "element.h"
#include <map>
#include <vector>
namespace binred {
    struct Function {
        std::string name;
        std::vector<std::string> args;
        std::string expand;
    };

    struct LangPattern {
        std::string lang;
        std::string pattern;
        std::map<std::string, Function> func;
    };

    struct Complex : Element {
        Complex(std::shared_ptr<token_t>&& tok)
            : Element(std::move(tok), ElementType::complex) {}
        std::map<std::string, Function> func;
        std::map<std::string, LangPattern> pattern;
    };
}  // namespace binred
