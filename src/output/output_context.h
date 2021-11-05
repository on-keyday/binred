/*license*/
#pragma once
#include <string>
#include <vector>
namespace binred {
    struct OutContext {
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

        const char* error_enum() {
            return "FrameError";
        }
    };
}  // namespace binred