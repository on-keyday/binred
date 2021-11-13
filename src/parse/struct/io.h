/*license*/
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
    };

    struct Read : IOElement {
        Read()
            : IOElement(ElementType::read) {}
    };

    struct Write : IOElement {
        Write()
            : IOElement(ElementType::write) {}
    };
}  // namespace binred
