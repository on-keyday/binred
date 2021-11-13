/*license*/
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