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
    template <class Char = char, class String = std::string, template <class...> class Vec = std::vector, template <class...> class Map = std::map>
    struct SubCommand {
        using option_t = OptMap<Char, String, Vec, Map>;
        using optset_t = typename option_t::Opt;
        using optres_t = typename option_t::OptResMap;

       private:
        String cmdname;
        option_t opt;
        Map<String, SubCommand> subcmd;

       public:
        struct SubCmdResult {
            friend SubCommand;

           private:
            Vec<std::pair<String, optres_t>> result;

           public:
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

        option_t& get_option() {
            return opt;
        }

        OptErr set_option(std::initializer_list<optset_t> list) {
            return opt.set_option(list);
        }

        SubCommand* set_subcommand(const String& name, std::initializer_list<optset_t> list = {}) {
            if (subcmd.count(name)) {
                return nullptr;
            }
            SubCommand ret;
            if (!ret.opt.set_option(list)) {
                return nullptr;
            }
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
            if (auto e = opt.parse_opt(index, col, argc, argv, optres.result.back().second, flag, cb)) {
                return true;
            }
            else if (e == OptError::option_suspended && subcmd.size()) {
                if (auto found = subcmd.find(argv[index]); found != subcmd.end()) {
                    index++;
                    return found->second.parse_opt(index, col, argc, argv, optres, op, cb);
                }
                return e;
            }
            else {
                return e;
            }
        }
    };
}  // namespace PROJECT_NAME
