/*license*/
#pragma once
#include "../../../parse/parser/parse.h"
#include "../../../calc/cast_ptr.h"
namespace binred {
    namespace cpp {
        struct SortElement {
            ParseResult cargo;
            ParseResult numberalias;
            ParseResult typealias;
            ParseResult read;
            ParseResult write;
            SortElement(Record& rec, ParseResult& result) {
                for (auto& e : result) {
                    switch (e->type) {
                        case ElementType::cargo:
                            cargo.push_back(std::static_pointer_cast<Cargo>(e));
                            break;
                        case ElementType::numalias:
                            numberalias.push_back(std::static_pointer_cast<NumberAlias>(e));
                            break;
                        case ElementType::tyalias:
                            typealias.push_back(std::static_pointer_cast<TypeAlias>(e));
                            break;
                        case ElementType::read:
                            read.push_back(std::static_pointer_cast<Read>(e));
                            break;
                        case ElementType::write:
                            write.push_back(std::static_pointer_cast<Write>(e));
                            break;
                        default:
                            break;
                    }
                }
            }
        };
    }  // namespace cpp
}  // namespace binred