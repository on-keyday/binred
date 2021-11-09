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
        Cargo()
            : Element(ElementType::cargo) {}
        std::string name;
        std::vector<std::shared_ptr<Param>> params;
        BaseInfo base;
    };

}  // namespace binred
