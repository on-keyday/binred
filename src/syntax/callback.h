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
                virtual bool is_sametype(const std::type_info&) const = 0;
                virtual void* get_rawptr() = 0;
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

                bool is_sametype(const std::type_info& info) const override {
                    return typeid(F) == info;
                }

                void* get_rawptr() override {
                    return reinterpret_cast<void*>(std::addressof(f));
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

                bool is_sametype(const std::type_info& info) const override {
                    return typeid(F) == info;
                }

                void* get_rawptr() override {
                    return reinterpret_cast<void*>(std::addressof(f));
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

            template <class T>
            T* get_rawfunc() {
                if (!fn) {
                    return nullptr;
                }
                if (!fn->is_sametype(typeid(T))) {
                    return nullptr;
                }
                return reinterpret_cast<T*>(fn->get_rawptr());
            }

           private:
            template <class Base, class T, class... Other>
            Base* find_derived() {
                auto ret = get_rawfunc<T>();
                if (ret) {
                    return static_cast<Base*>(ret);
                }
                return find_derived<Base, Other...>();
            }

            template <class Base>
            Base* find_derived() {
                return nullptr;
            }

           public:
            template <class Base, class... Derived>
            Base* get_rawfunc() {
                return find_derived<Base, Derived...>();
            }

            ~Callback() {
                delete fn;
            }
        };

    }  // namespace syntax
}  // namespace binred