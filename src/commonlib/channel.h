/*
    commonlib - common utility library
    Copyright (c) 2021 on-keyday (https://github.com/on-keyday)
    Released under the MIT license
    https://opensource.org/licenses/mit-license.php
*/

#pragma once
#include <enumext.h>
#include <atomic>
#include <memory>
#include <tuple>
#include <deque>
#include <map>

namespace PROJECT_NAME {

    enum class ChanError {
        none,
        closed,
        limited,
        empty,
    };

    using ChanErr = commonlib2::EnumWrap<ChanError, ChanError::none, ChanError::closed>;

    enum class ChanDisposeFlag {
        remove_front,
        remove_back,
        remove_new,
    };

    template <class T, template <class...> class Queue>
    struct Channel;

    template <class T, template <class...> class Que = std::deque>
    struct SendChan {
        using base_chan = Channel<T, Que>;

       private:
        std::shared_ptr<base_chan> chan;

       public:
        SendChan(std::shared_ptr<base_chan>& chan)
            : chan(chan) {}

        ChanErr operator<<(T&& value) {
            return chan->store(std::move(value));
        }

        bool close() {
            return chan->close();
        }

        bool closed() const {
            return chan->closed();
        }
    };

    template <class T, template <class...> class Que = std::deque>
    struct RecvChan {
        using base_chan = Channel<T, Que>;

       private:
        std::shared_ptr<base_chan> chan;
        bool block = false;

       public:
        RecvChan(std::shared_ptr<base_chan>& chan)
            : chan(chan) {}

        ChanErr operator>>(T& value) {
            return block ? chan->block_load(value) : chan->load(value);
        }

        bool close() {
            return chan->close();
        }

        bool closed() const {
            return chan->closed();
        }

        void set_block(bool block) {
            this->block = block;
        }
    };

    template <class T, template <class...> class Queue>
    struct ChanBuf {
        Queue<T> impl;
        T& front() {
            return impl.front();
        }
        void pop_front() {
            impl.pop_front();
        }

        void pop_back() {
            impl.pop_back();
        }

        void push_back(T&& t) {
            impl.push_back(std::move(t));
        }

        size_t size() const {
            return impl.size();
        }
    };

    template <class T, template <class...> class Queue>
    struct Channel {
        using queue_type = ChanBuf<T, Queue>;
        using value_type = T;

       private:
        ChanDisposeFlag dflag = ChanDisposeFlag::remove_new;
        size_t quelimit = ~0;
        queue_type que;
        std::atomic_flag lock_;
        std::atomic_flag closed_;
        std::atomic_flag block_;

       public:
        Channel(size_t quelimit = ~0, ChanDisposeFlag dflag = ChanDisposeFlag::remove_new) {
            lock_.clear();
            closed_.clear();
            block_.clear();
            this->quelimit = quelimit;
            this->dflag = dflag;
        }

       private:
        bool dispose() {
            if (que.size() == quelimit) {
                switch (dflag) {
                    case ChanDisposeFlag::remove_new:
                        return false;
                    case ChanDisposeFlag::remove_front:
                        que.pop_front();
                        return true;
                    case ChanDisposeFlag::remove_back:
                        que.pop_back();
                        return true;
                    default:
                        return false;
                }
            }
            return true;
        }

        bool lock() {
            while (lock_.test_and_set()) {
                lock_.wait(true);
            }
            if (closed_.test()) {
                unlock();
                return false;
            }
            return true;
        }

        void unlock() {
            lock_.clear();
            lock_.notify_all();
        }

        void unblock() {
            block_.clear();
            block_.notify_all();
        }

       public:
        ChanErr store(T&& t) {
            if (!lock()) {
                return ChanError::closed;
            }
            if (!dispose()) {
                unlock();
                return ChanError::limited;
            }
            que.push_back(std::move(t));
            unblock();
            unlock();
            return true;
        }

       private:
        bool load_impl(T& t) {
            t = std::move(que.front());
            que.pop_front();
            unlock();
            return true;
        }

       public:
        ChanErr block_load(T& t) {
            while (true) {
                if (!lock()) {
                    return ChanError::closed;
                }
                if (que.size() == 0) {
                    block_.test_and_set();
                    unlock();
                    block_.wait(true);
                    continue;
                }
                break;
            }
            return load_impl(t);
        }

        ChanErr load(T& t) {
            if (!lock()) {
                return ChanError::closed;
            }
            if (que.size() == 0) {
                unlock();
                return ChanError::empty;
            }
            return load_impl(t);
        }

        bool close() {
            lock();
            bool res = closed_.test_and_set();
            unblock();
            unlock();
            return res;
        }

        bool closed() const {
            return closed_.test();
        }
    };

    template <class T, template <class...> class Que = std::deque>
    std::tuple<SendChan<T, Que>, RecvChan<T, Que>> make_chan(size_t limit = ~0, ChanDisposeFlag dflag = ChanDisposeFlag::remove_new) {
        auto base = std::make_shared<Channel<T, Que>>(limit, dflag);
        return {base, base};
    }

    template <class T, template <class...> class Que = std::deque, template <class...> class Map = std::map>
    struct ForkChan {
       private:
        Map<size_t, SendChan<T, Que>> listeners;
        std::atomic_flag lock_;
        size_t id = 0;

       private:
        bool lock() {
            if (lock_.test_and_set()) {
                lock_.wait(true);
            }
            return true;
        }
        void unlock() {
            lock_.clear();
            lock_.notify_all();
        }

       public:
        ChanError subscribe(size_t& id, SendChan<T, Que> que) {
            if (que->closed()) {
                return ChanError::closed;
            }
            if (!lock()) {
                return ChanError::closed;
            }
            listeners.emplace(this->id, que);
            this->id++;
            unlock();
            return true;
        }

        bool remove(size_t id) {
            if (!lock()) {
                return ChanError::closed;
            }
            auto result = (bool)listeners.erase(id);
            unlock();
            return result;
        }

        ChanError operator<<(T&& t) {
            return store(std::forward<T>(t));
        }

        ChanError store(T&& value) {
            if (!lock()) {
                return ChanError::closed;
            }
            std::erase_if(listeners, [](auto& h) {
                if (h->second.closed()) {
                    return true;
                }
                return false;
            });
            T copy(std::move(value));
            for (auto& p : listeners) {
                auto tmp = copy;
                p->second << std::move(tmp);
            }
            unlock();
            return true;
        }
    };
}  // namespace PROJECT_NAME
