/*license*/

#include <utility>

namespace binred {
    template <typename F>
    struct make_lambda {
        F f;
        make_lambda(F in)
            : f(in) {}
        template <typename... Args>
        decltype(auto) operator()(Args&&... args) const& {
            return f(std::ref(*this), std::forward<Args>(args)...);
        }
    };

    template <typename F>
    make_lambda(F) -> make_lambda<F>;
}  // namespace binred