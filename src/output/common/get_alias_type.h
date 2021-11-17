/*
    binred - binary reader code generator
    Copyright (c) 2021 on-keyday (https://github.com/on-keyday)
    Released under the MIT license
    https://opensource.org/licenses/mit-license.php
*/

#pragma once

#include <cstdint>
#include <string>

namespace binred {
    template <class F>
    std::string get_alias_type(std::int64_t maxvalue, std::int64_t minvalue, F&& cb) {
        if (minvalue >= 0) {
            if (maxvalue <= 0xff) {
                return cb(1, true);
            }
            else if (maxvalue <= 0xffff) {
                return cb(2, true);
            }
            else if (maxvalue <= 0xffffffff) {
                return cb(4, true);
            }
            else {
                return cb(8, true);
            }
        }
        else {
            if (maxvalue <= 127 && minvalue >= -128) {
                return cb(1, false);
            }
            else if (maxvalue <= 32767 && minvalue >= -32767) {
                return cb(2, false);
            }
            else if (maxvalue <= 2147483647 && minvalue >= -2147483648) {
                return cb(4, false);
            }
            else {
                return cb(8, false);
            }
        }
    }
}  // namespace binred
