/*license*/
#pragma once
#include "element.h"
#include <vector>
#include "param.h"
#include "command.h"

namespace binred {

    struct Read : Element {
        Read()
            : Element(ElementType::read) {}
        std::string name;
        std::vector<std::string> args;
        std::vector<std::shared_ptr<Command>> cmds;
    };
}  // namespace binred
