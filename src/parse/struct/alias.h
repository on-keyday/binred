/*license*/
#pragma once

#include "element.h"
#include "param.h"
#include <map>
#include <string>
#include <memory>
namespace binred {

    struct NumberAlias : Element {
        NumberAlias()
            : Element(ElementType::numalias) {}
        std::string name;
        std::map<std::string, std::shared_ptr<Value>> alias;
        std::string baseclass;
    };

    struct TypeAlias : Element {
        TypeAlias()
            : Element(ElementType::tyalias) {}
        std::shared_ptr<Param> type;
    };
}  // namespace binred
