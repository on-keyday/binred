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
            static bool write_syntax(Serializer<std::string>& target, const std::shared_ptr<Syntax>& stx, std::map<std::shared_ptr<token_t>, size_t>& stxtok) {
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

            static bool read_syntax(Deserializer<std::string>& target, std::shared_ptr<Syntax>& stx, std::vector<std::shared_ptr<token_t>>& stxtok) {
                size_t tmpnum = 0;
                if (!tkpsr::BinaryIO::read_num(target, tmpnum)) {
                    return false;
                }
                auto type = SyntaxType(tmpnum);
                switch (type) {
                    case SyntaxType::or_: {
                        auto or_ = std::make_shared<OrSyntax>(SyntaxType::or_);
                        if (!tkpsr::BinaryIO::read_num(target, tmpnum)) {
                            return false;
                        }
                        auto count = tmpnum;
                        for (size_t i = 0; i < count; i++) {
                            if (!tkpsr::BinaryIO::read_num(target, tmpnum)) {
                                return false;
                            }
                            or_->syntax.push_back(std::vector<std::shared_ptr<Syntax>>());
                            auto& back = or_->syntax.back();
                            for (size_t k = 0; k < tmpnum; k++) {
                                if (!read_syntax(target, stx, stxtok)) {
                                    return false;
                                }
                                back.push_back(std::move(stx));
                                stx = nullptr;
                            }
                        }
                        stx = std::move(or_);
                        [[fallthrough]];
                    }
                    default: {
                        if (!stx) {
                            stx = std::make_shared<Syntax>(type);
                        }
                        target.read_as<std::uint8_t>(stx->flag);
                        if (!tkpsr::BinaryIO::read_num(target, tmpnum)) {
                            return false;
                        }
                        if (stxtok.size() <= tmpnum) {
                            return false;
                        }
                        stx->token = stxtok[tmpnum];
                        return true;
                    }
                }
            }

            static bool read_() {
            }
        };
    }  // namespace syntax
}  // namespace PROJECT_NAME
