/*
    binred - binary I/O code generator
    Copyright (c) 2021 on-keyday (https://github.com/on-keyday)
    Released under the MIT license
    https://opensource.org/licenses/mit-license.php
*/

#pragma once
#include <callback_invoker.h>

namespace binred {
    namespace syntax {
        template <class Ret, class... Args>
        struct Callback {
           private:
            struct Base {
                virtual Ret operator()(Args&&...) const = 0;
                virtual ~Base() {}
            };

            Base* fn = nullptr;

            DEFINE_ENABLE_IF_EXPR_VALID(return_Ret, static_cast<Ret>(std::declval<T>()(std::declval<Args>()...)));

            template <class F, bool ret_void = std::disjunction_v<std::is_same<Ret, void>, std::negation<return_Ret<F>>>>
            struct Impl : Base {
                F f;
                Impl(F&& in)
                    : f(std::forward<F>(in)) {}
                Ret operator()(Args&&... args) const override {
                    return static_cast<Ret>(f(std::forward<Args>(args)...));
                }
            };

            template <class F>
            struct Impl<F, true> : Base {
                F f;
                Impl(F&& in)
                    : f(std::forward<F>(in)) {}
                Ret operator()(Args&&... args) const override {
                    f(std::forward<Args>(args)...);
                    return Ret();
                }
            };

           public:
            constexpr Callback() {}

            constexpr operator bool() const {
                return fn != nullptr;
            }

            Callback(Callback&& in) noexcept {
                fn = in.f;
                in.f = nullptr;
            }

            Callback& operator=(Callback&& in) noexcept {
                delete fn;
                fn = in.fn;
                in.fn = nullptr;
                return *this;
            }

            template <class F>
            Callback(F&& f) {
                fn = new Impl<std::decay_t<F>>(std::forward<std::decay_t<F>>(f));
            }

            template <class... CArg>
            Ret operator()(CArg&&... args) const {
                return (*fn)(std::forward<CArg>(args)...);
            }

            ~Callback() {
                delete fn;
            }
        };

    }  // namespace syntax
}  // namespace binred