/*
    commonlib - common utility library
    Copyright (c) 2021 on-keyday (https://github.com/on-keyday)
    Released under the MIT license
    https://opensource.org/licenses/mit-license.php
*/

#pragma once
#include "project_name.h"
#include "callback_invoker.h"
#include <stdexcept>

namespace PROJECT_NAME {
    template <class Ret, class... Args>
    struct Callback {
       private:
        struct Base {
            virtual Ret operator()(Args&&...) = 0;
            virtual Ret operator()(Args&&...) const = 0;
            virtual bool is_sametype(const std::type_info&) const = 0;
            virtual void* get_rawptr() = 0;
            virtual bool const_callable() const = 0;
            virtual ~Base() {}
        };

#define CB_COMMON_METHOD()                                        \
    F f;                                                          \
    Impl(F&& in)                                                  \
        : f(std::forward<F>(in)) {}                               \
    bool is_sametype(const std::type_info& info) const override { \
        return typeid(F) == info;                                 \
    }                                                             \
                                                                  \
    void* get_rawptr() override {                                 \
        return reinterpret_cast<void*>(std::addressof(f));        \
    }                                                             \
    bool const_callable() const override {                        \
        return has_const_call<F>::value;                          \
    }

        Base* fn = nullptr;

        DEFINE_ENABLE_IF_EXPR_VALID(return_Ret, static_cast<Ret>(std::declval<T>()(std::declval<Args>()...)));
        DEFINE_ENABLE_IF_EXPR_VALID(has_const_call, static_cast<Ret>(std::declval<const T>()(std::declval<Args>()...)));

        template <bool is_ref = std::is_reference_v<Ret>>
        struct throw_exception_if_Ret_is_ref {
            static Ret get_Ret() {
                return Ret();
            }
        };

        template <>
        struct throw_exception_if_Ret_is_ref<true> {
            static Ret get_Ret() {
                    throw std::logic_error("Ret is reference type,"
                                           " but callback returned what is not castable to Ret"));
            }
        };

        template <class F, bool flag = has_const_call<F>::value>
        struct invoke_if_const_exists {
            static decltype(auto) invoke(const F& f, Args&&... args) {
                return f(std::forward<Args>(args)...);
            }
        };

        template <class F>
        struct invoke_if_const_exists<F, false> {
            static decltype(auto) invoke(const F& f, Args&&... args) {
                return throw_exception_if_Ret_is_ref<>::get_Ret();
            }
        };

        template <class F, bool ret_void = std::disjunction_v<std::is_same<Ret, void>, std::negation<return_Ret<F>>>>
        struct Impl : Base {
            CB_COMMON_METHOD()

            Ret operator()(Args&&... args) override {
                return static_cast<Ret>(f(std::forward<Args>(args)...));
            }

            Ret operator()(Args&&... args) const override {
                return static_cast<Ret>(invoke_if_const_exists<F>::invoke(f, std::forward<Args>(args)...));
            }
        };

        template <class F>
        struct Impl<F, true> : Base {
            CB_COMMON_METHOD()

            Ret operator()(Args&&... args) override {
                f(std::forward<Args>(args)...);
                return throw_exception_if_Ret_is_ref<>::get_Ret();
            }

            Ret operator()(Args&&... args) const override {
                invoke_if_const_exists<F>::invoke(f, std::forward<Args>(args)...);
                return throw_exception_if_Ret_is_ref<>::get_Ret();
            }
        };

#define CB_MEMBER(STNAME, ...)                                \
    template <class F, class Res, class... MArg>              \
    struct STNAME {                                           \
        using Member = Res (F::*)(MArg...) __VA_ARGS__;       \
        __VA_ARGS__ F* ptr = nullptr;                         \
        Member member = nullptr;                              \
                                                              \
        STNAME(__VA_ARGS__ F* p, Member memb)                 \
            : ptr(p), member(memb) {}                         \
                                                              \
        Res operator()(Args&&... args) __VA_ARGS__ {          \
            return ptr->*member(std::forward<Args>(args)...); \
        }                                                     \
    };
        CB_MEMBER(MemberFunc)
        CB_MEMBER(ConstMemberFunc, const)
#undef CB_MEMBER
#undef CB_COMMON_METHOD
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

       private:
        template <class F>
        void make_fn(F&& f) {
            fn = new Impl<std::decay_t<F>>(std::forward<std::decay_t<F>>(f));
        }

       public:
        template <class F>
        Callback(F&& f) {
            make_fn(std::forward<F>(f));
        }

        template <class F, class Res, class... MArg>
        Callback(F* ptr, Res (F::*member)(MArg...)) {
            make_fn(MemberFunc(ptr, member));
        }

        template <class F, class Res, class... MArg>
        Callback(const F* ptr, Res (F::*member)(MArg...) const) {
            make_fn(ConstMemberFunc(ptr, member));
        }

        template <class... CArg>
        Ret operator()(CArg&&... args) {
            return (*fn)(std::forward<CArg>(args)...);
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
        template <class Base, class DerivedOne, class... Derived>
        Base* get_rawfunc() {
            return find_derived<Base, DerivedOne, Derived...>();
        }

        bool is_const_callable() const {
            return fn ? fn->const_callable() : false;
        }

        ~Callback() {
            delete fn;
        }
    };

}  // namespace PROJECT_NAME
