/*licnese*/
#pragma once

#include "syntax_parser.h"
#include "syntax_matcher.h"

namespace binred {
    namespace syntax {
        struct SyntaxCompiler {
           private:
            SyntaxParserMaker pm;
            SyntaxMatching match;

           public:
            template <class F>
            void set_callback(F&& f) {
                match.cb = std::forward<F>(f);
            }

            auto& callback() {
                return match.cb;
            }

            template <class Reader>
            MergeErr make_parser(Reader& r) {
                auto err = pm.parse(r);
                if (!err) {
                    return err;
                }
                auto compile = pm.get_compiler();
                if (!compile()) {
                    return false;
                }
                match.p = std::move(compile);
                return true;
            }

            template <class Reader>
            MergeErr parse(Reader& r) {
                auto err = match.p.parse(r);
                if (!err) {
                    return err;
                }
                if (!match.parse_follow_syntax()) {
                    return false;
                }
                return true;
            }
        };
    }  // namespace syntax
}  // namespace binred
