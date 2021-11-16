/*license*/
#pragma once

#include "element.h"
#include "param.h"
#include <map>
#include <string>
#include <memory>
namespace binred {

    struct NumberAlias : Element {
        NumberAlias(std::shared_ptr<token_t>&& tok)
            : Element(std::move(tok), ElementType::numalias) {}
        std::string name;
        std::map<std::string, std::shared_ptr<Value>> alias;
        std::string baseclass;
    };

    struct TypeAlias : Element {
        TypeAlias(std::shared_ptr<token_t>&& tok)
            : Element(std::move(tok), ElementType::tyalias) {}
        std::shared_ptr<Param> type;
    };
}  // namespace binred
