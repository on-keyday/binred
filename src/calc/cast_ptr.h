/*
    binred - binary I/O code generator
    Copyright (c) 2021 on-keyday (https://github.com/on-keyday)
    Released under the MIT license
    https://opensource.org/licenses/mit-license.php
*/

#pragma once
#include <utility>
namespace binred {
    template <class T, class F>
    T* castptr(F& f) {
        if (!f) {
            return nullptr;
        }
        return static_cast<T*>(std::addressof(*f));
    }

    template <class T, class F>
    T* castptr(F* f) {
        if (!f) {
            return nullptr;
        }
        return static_cast<T*>(f);
    }
}  // namespace binred
