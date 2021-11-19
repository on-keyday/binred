/*
    binred - binary I/O code generator
    Copyright (c) 2021 on-keyday (https://github.com/on-keyday)
    Released under the MIT license
    https://opensource.org/licenses/mit-license.php
*/

#pragma once

#include "../parser.h"

#include <vector>

namespace binred {
    struct Syntax {
        std::shared_ptr<token_t> token;
        std::vector<std::shared_ptr<Syntax>> next;
    };

    struct SyntaxC {
    };
}  // namespace binred