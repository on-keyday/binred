/*
    commonlib - common utility library
    Copyright (c) 2021 on-keyday (https://github.com/on-keyday)
    Released under the MIT license
    https://opensource.org/licenses/mit-license.php
*/

#pragma once

#include "syntax_parser.h"
#include "../tokenparser/token_bin.h"

namespace PROJECT_NAME {
    namespace syntax {
        struct SyntaxIO {
            static bool write_syntax(Serializer<std::string>& target, std::shared_ptr<Syntax>& stx, std::map<std::shared_ptr<token_t>, size_t>& stxtok) {
                if (!tkpsr::BinaryIO::write_num(target, size_t(stx->type))) {
                    return false;
                }
                switch (stx->type) {
                    default:
                }
            }
        };
    }  // namespace syntax
}  // namespace PROJECT_NAME
