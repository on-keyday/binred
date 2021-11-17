/*license*/
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

    struct Cargo : Element {
        Cargo(std::shared_ptr<token_t>&& tok)
            : Element(std::move(tok), ElementType::cargo) {}
        std::string name;
        std::vector<std::shared_ptr<Param>> params;
        BaseInfo base;
        bool expanded = false;

        std::map<std::string, std::weak_ptr<Cargo>> derived;
    };

}  // namespace binred
