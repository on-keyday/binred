/*license*/
#pragma once
#include "element.h"
#include <vector>
#include <memory>
#include <string>
#include "param.h"
namespace binred {

    struct BaseInfo {
        std::string selfname;
        std::string basename;
    };

    struct Cargo : Element {
        Cargo(std::shared_ptr<token_t>&& tok)
            : Element(std::move(tok), ElementType::cargo) {}
        std::string name;
        std::vector<std::shared_ptr<Param>> params;
        BaseInfo base;
        bool expanded = false;
    };

}  // namespace binred
