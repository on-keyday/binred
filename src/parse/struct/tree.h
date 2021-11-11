/*license*/
#pragma once
#include <vector>
#include <string>
#include <memory>
#include "element.h"
namespace binred {
    using namespace commonlib2;
    struct TreeDepth {
        std::vector<std::string> expected;
        TreeDepth(std::initializer_list<std::string> list)
            : expected(list.begin(), list.end()) {}
    };

    struct Tree {
        std::vector<TreeDepth> depth;
        size_t index = 0;

        Tree(std::initializer_list<TreeDepth> list)
            : depth(list.begin(), list.end()) {}
        bool expect(std::shared_ptr<token>& r, std::string& expected) {
            if (!r) return false;
            for (auto& s : depth[index % depth.size()].expected) {
                if (r->is_(TokenKind::symbols) && r->has_(s)) {
                    expected = s;
                    return true;
                }
            }
            return false;
        }
        void increment() {
            index++;
        }
        void decrement() {
            index--;
        }

        void set_index(size_t idx) {
            index = idx;
        }

        size_t get_index() const {
            return index;
        }

        bool is_end() {
            return index != 0 && (index % depth.size() == 0);
        }
    };
}  // namespace binred