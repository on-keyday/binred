/*license*/
#pragma once
#include "../../../parse/parser/parse.h"
#include "../../../calc/cast_ptr.h"
namespace binred {

    struct SortElement {
        template <class T>
        using Sorted = std::vector<std::shared_ptr<T>>;
        Sorted<Cargo> cargo;
        Sorted<NumberAlias> numberalias;
        Sorted<TypeAlias> typealias;
        Sorted<Read> read;
        Sorted<Write> write;
        std::set<std::weak_ptr<Param>> unresolved_param;
        SortElement(ParseResult& result) {
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

}  // namespace binred