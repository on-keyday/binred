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
                    case SyntaxType::or_: {
                        auto or_ = std::static_pointer_cast<OrSyntax>(stx);
                        if (!tkpsr::BinaryIO::write_num(target, or_->syntax.size())) {
                            return false;
                        }
                        for (auto& v : or_->syntax) {
                            if (!tkpsr::BinaryIO::write_num(target, v.size())) {
                                return false;
                            }
                            for (auto& c : v) {
                                if (!write_syntax(target, stx, stxtok)) {
                                    return false;
                                }
                            }
                        }
                        [[fallthrough]];
                    }
                    default: {
                        target.write(std::uint8_t(stx->flag));
                        auto found = stxtok.find(stx->token);
                        if (found == stxtok.end()) {
                            return false;
                        }
                        if (!tkpsr::BinaryIO::write_num(target, found->second)) {
                            return false;
                        }
                        return true;
                    }
                }
            }
        };
    }  // namespace syntax
}  // namespace PROJECT_NAME
