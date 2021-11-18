/*
    commonlib - common utility library
    Copyright (c) 2021 on-keyday (https://github.com/on-keyday)
    Released under the MIT license
    https://opensource.org/licenses/mit-license.php
*/

#pragma once

#include "project_name.h"

#include "optmap.h"

namespace PROJECT_NAME {
    template <class Cmd, class Char = char, class String = std::string, template <class...> class Vec = std::vector, template <class...> class Map = std::map>
    struct SubCommand_base {
        using option_t = OptMap<Char, String, Vec, Map>;
        using optset_t = typename option_t::Opt;
        using optres_t = typename option_t::OptResMap;

       protected:
        String cmdname;
        option_t opt;
        Map<String, Cmd> subcmd;
        Cmd* parent = nullptr;

       public:
        SubCommand_base() {}
        SubCommand_base(const String& name)
            : cmdname(name) {}

        struct SubCmdResult {
            friend Cmd;
            friend SubCommand_base;

           private:
            Vec<std::pair<String, optres_t>> result;
            Cmd* current = nullptr;

           public:
            const Cmd* get_current() const {
                return current;
            }

            size_t get_layersize() const {
                return result.size();
            }
            optres_t* get_layer(const String& name) {
                for (auto& v : result) {
                    if (v.first == name) {
                        return &v.second;
                    }
                }
                return nullptr;
            }

            optres_t* get_layer(size_t index) {
                if (index >= result.size()) {
                    return nullptr;
                }
                return &v[index].second;
            }
        };

        String get_currentcmdname(Char sep = ':') const {
            String ret;
            if (parent) {
                ret = parent->get_currentcmdname();
            }
            if (sep != 0) {
                ret += sep;
            }
            ret += ' ';
            return ret;
        }

        const String& get_cmdname() const {
            return cmdname;
        }

        const Cmd* get_parent() const {
            return parent;
        }

        option_t& get_option() {
            return opt;
        }

        OptErr set_option(std::initializer_list<optset_t> list) {
            return opt.set_option(list);
        }

        Cmd* set_subcommand(const String& name, std::initializer_list<optset_t> list = {}) {
            if (subcmd.count(name)) {
                return nullptr;
            }
            Cmd ret(name);
            if (!ret.opt.set_option(list)) {
                return nullptr;
            }
            ret.parent = static_cast<Cmd*>(this);
            return &(subcmd[name] = std::move(ret));
        }

        template <class C, class Ignore = bool (*)(const String&, bool)>
        OptErr parse_opt(int argc, C** argv, SubCmdResult& optres, OptOption op = OptOption::default_mode, Ignore&& cb = Ignore()) {
            int index = 1, col = 0;
            return parse_opt(index, col, argc, argv, optres, op, cb);
        }

        template <class C, class Ignore = bool (*)(const String&, bool)>
        OptErr parse_opt(int& index, int& col, int argc, C** argv, SubCmdResult& optres, OptOption op = OptOption::default_mode, Ignore&& cb = Ignore()) {
            auto flag = op;
            if (subcmd.size()) {
                flag &= ~OptOption::parse_all_arg;
            }
            optres.result.push_back(std::pair{cmdname, optres_t()});
            optres.current = static_cast<Cmd*>(this);
            while (true) {
                if (auto e = opt.parse_opt(index, col, argc, argv, optres.result.back().second, flag, cb)) {
                    return true;
                }
                else if (e == OptError::option_suspended && subcmd.size()) {
                    if (auto found = subcmd.find(argv[index]); found != subcmd.end()) {
                        index++;
                        return found->second.parse_opt(index, col, argc, argv, optres, op, cb);
                    }
                    if (any(op & OptOption::parse_all_arg)) {
                        flag = op;
                        continue;
                    }
                    return e;
                }
                else {
                    return e;
                }
            }
        }
    };

    template <class Char = char, class String = std::string, template <class...> class Vec = std::vector, template <class...> class Map = std::map>
    struct SubCommand : SubCommand_base<SubCommand<Char, String, Vec, Map>, Char, String, Vec, Map> {
        using base_t = SubCommand_base<SubCommand, Char, String, Vec, Map>;
        using result_t = typename base_t::SubCmdResult;
        SubCommand() {}
        SubCommand(const String& name)
            : SubCommand_base<SubCommand, Char, String, Vec, Map>(name) {}
    };

    template <class... Args>
    struct FuncHolder {
       private:
        struct Base {
            virtual int operator()(Args&&... args) const = 0;
            virtual ~Base() {}
        };
        Base* base = nullptr;

        DEFINE_ENABLE_IF_EXPR_VALID(return_int, (int)std::declval<T>()(std::declval<Args>()...));

        template <class F, bool is_int = return_int<F>::value>
        struct Impl : Base {
            F f;
            Impl(F&& in)
                : f(std::forward<F>(in)) {}
            int operator()(Args&&... args) const {
                return (int)f(std::forward<Args>(args)...);
            }
            virtual ~Impl() {}
        };

        template <class F>
        struct Impl<F, false> : Base {
            F f;
            Impl(F&& in)
                : f(std::forward<F>(in)) {}
            int operator()(Args&&... args) const {
                f(std::forward<Args>(args)...);
                return 0;
            }
            virtual ~Impl() {}
        };

       public:
        constexpr FuncHolder() {}

        operator bool() const {
            return base != nullptr;
        }

        int operator()(Args&&... args) const {
            return (*base)(std::forward<Args>(args)...);
        }

        FuncHolder& operator=(FuncHolder&& in) noexcept {
            delete base;
            base = in.base;
            in.base = nullptr;
            return *this;
        }

        template <class F>
        FuncHolder(F&& in) {
            base = new Impl<F>(std::forward<F>(in));
        }

        FuncHolder(const FuncHolder&) = delete;

        FuncHolder(FuncHolder&& in) noexcept {
            delete base;
            base = in.base;
            in.base = nullptr;
        }

        ~FuncHolder() {
            delete base;
        }
    };

    template <class Char = char, class String = std::string, template <class...> class Vec = std::vector, template <class...> class Map = std::map>
    struct SubCmdDispatch : SubCommand_base<SubCmdDispatch<Char, String, Vec, Map>, Char, String, Vec, Map> {
        using base_t = SubCommand_base<SubCmdDispatch<Char, String, Vec, Map>, Char, String, Vec, Map>;
        using result_t = typename base_t::SubCmdResult;
        using optset_t = typename base_t::optset_t;

       private:
        using holder_t = FuncHolder<result_t&>;
        holder_t func;

       public:
        SubCmdDispatch() {}
        SubCmdDispatch(const String& name)
            : SubCommand_base<SubCmdDispatch<Char, String, Vec, Map>, Char, String, Vec, Map>(name) {}

        template <class F>
        SubCmdDispatch(const String& name, F&& f)
            : func(std::decay_t<F>(f)), SubCommand_base<SubCmdDispatch<Char, String, Vec, Map>, Char, String, Vec, Map>(name) {}

        template <class F>
        void set_callback(F&& f) {
            func = std::forward<F>(f);
        }

        template <class F = holder_t>
        SubCmdDispatch* set_subcommand(const String& name, std::initializer_list<optset_t> list, F&& in = holder_t()) {
            auto ret = base_t::set_subcommand(name, list);
            if (ret) {
                ret->func = std::forward<F>(in);
            }
            return ret;
        }

        template <class C, class Ignore = bool (*)(const String&, bool)>
        std::pair<OptErr, int> run(int argc, C** argv, OptOption op = OptOption::default_mode, Ignore&& cb = Ignore()) {
            result_t result;
            op |= OptOption::parse_all_arg;
            if (auto e = this->parse_opt(argc, argv, result, op, cb); !e) {
                return {e, (int)e.e};
            }
            auto ptr = result.get_current();
            while (ptr) {
                if (ptr->func) {
                    return {true, ptr->func(result)};
                }
                ptr = ptr->get_parent();
            }
            return {false, -1};
        }
    };
}  // namespace PROJECT_NAME
