/*
    binred - binary reader code generator
    Copyright (c) 2021 on-keyday (https://github.com/on-keyday)
    Released under the MIT license
    https://opensource.org/licenses/mit-license.php
*/

#pragma once
#include <string>
#include <vector>
namespace binred {
    namespace cpp {
        struct CppOutContext {
            std::string buffer;
            std::vector<std::string> enum_v;

            void write(const std::string& w) {
                buffer += w;
            }

            void set_error_enum(const std::string& v) {
                enum_v.push_back(v);
            }

            std::string length_of_byte(const std::string& var) {
                return "std::size(" + var + ")";
            }

            const char* buffer_type() {
                return "std::string";
            }

            bool allow_fixed() {
                return true;
            }

            std::string error_enum() {
                return "FrameError";
            }
        };
    }  // namespace cpp
}  // namespace binred
