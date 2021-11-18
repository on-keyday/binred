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
            ret.parent = this;
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
            optres.current = this;
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

    template <class Function, class Char = char, class String = std::string, template <class...> class Vec = std::vector, template <class...> class Map = std::map>
    struct SubCmdDispach : SubCommand_base<SubCmdDispach<Char, String, Vec, Map>, Char, String, Vec, Map> {
        using base_t = SubCommand_base<SubCommand, Char, String, Vec, Map>;
        using result_t = typename base_t::SubCmdResult;
        Function func;
        template <class F>
        SubCmdDispach(F&& f)
            : func(std::decay_t<F>(f)) {}

        template <class C, class Ignore = bool (*)(const String&, bool)>
        std::pair<OptErr, int> run(int argc, C** argv, OptOption op = OptOption::default_mode, Ignore&& cb = Ignore()) {
            result_t result;
            op |= OptOption::parse_all_arg;
            if (auto e = this->parse_opt(); !e) {
                return {
                    e,
                }
            }
        }
    };
}  // namespace PROJECT_NAME
