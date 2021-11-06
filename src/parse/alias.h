/*license*/
#pragma once

#include "element.h"
#include <map>
#include <string>
#include <memory>
namespace binred {

    struct Alias : Element {
        Alias()
            : Element(ElementType::alias) {}
        std::string name;
        std::map<std::string, std::shared_ptr<Value>> alias;
        std::string baseclass;
    };
}  // namespace binred
