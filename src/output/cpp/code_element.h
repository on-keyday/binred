/*
    binred - binary I/O code generator
    Copyright (c) 2021 on-keyday (https://github.com/on-keyday)
    Released under the MIT license
    https://opensource.org/licenses/mit-license.php
*/

#pragma once

#include <string>

namespace binred {
    namespace cpp {
        std::string& write_return(std::string& buf, const std::string& value) {
            buf += "return ";
            buf += value;
            buf += ";";
            return buf;
        }

        std::string& write_if(std::string& buf, const std::string& cond) {
            buf += "if(";
            buf += cond;
            buf += ")";
            return buf;
        }

        std::string write_not(const std::string& cond) {
            return "!(" + cond + ")";
        }

        std::string write_noteq(const std::string& left, const std::string& right) {
            return "(" + left + ") != (" + right + ")";
        }

        std::string& write_beginblock(std::string& buf) {
            buf += "{\n";
            return buf;
        }

        std::string& write_endblock(std::string& buf) {
            buf += "\n}\n";
            return buf;
        }

        std::string& write_args(std::string& buf) {
            return buf;
        }

        template <class String, class... Args>
        std::string& write_args(std::string& buf, String&& str, Args&&... args) {
            buf += str;
            if (sizeof...(Args)) {
                buf += ",";
            }
            return write_args(buf, std::forward<Args>(args)...);
        }

        template <class... Args>
        std::string& write_call(std::string& buf, const std::string& funcname, Args&&... args) {
            buf += funcname;
            buf += "(";
            write_args(buf, std::forward<Args>(args)...);
            buf += ")";
            return buf;
        }
    }  // namespace cpp

}  // namespace binred
