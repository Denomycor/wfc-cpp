#pragma once

#include <cassert>
#include <climits>
#include <type_traits>
#include <utility>
#include <vector>
#include <functional>

/*
 * Implements observer pattern. Callables are registered with IDs. Negative IDs are reserved for auto-generation
 */ 
template<typename... Args>
class Signal {
private:
    using callable_t = void(const Args&...);
    using subscriber_t = std::pair<int, std::function<callable_t>>;

    std::vector<subscriber_t> m_subscribers; 
    int counter = INT_MIN;

    subscriber_t* get_subscriber(int key) {
        for(auto& subs : m_subscribers){
            if(subs.first == key) return &subs;
        }
        return nullptr;
    }

    template<typename T, typename = std::enable_if<std::is_invocable_v<T, const Args&...>>>
    std::function<callable_t> make_one_shot(int key, T&& callable) {
        return [=](const Args&... args){
            callable(args...);
            disconnect(key);
        };
    }

public:

    template<typename T, typename = std::enable_if<std::is_invocable_v<T, const Args&...>>>
    void connect(int key, T&& callable, bool once = false) {
        assert(key >= 0 && "Manual signal ids must be >= 0");
        auto p_sub = get_subscriber(key);
        if(p_sub == nullptr)
            m_subscribers.emplace_back(key, once ? make_one_shot(key, std::forward<T>(callable)) : std::forward<T>(callable));
        else
            p_sub->second = once ? make_one_shot(key, std::forward<T>(callable)) : std::forward<T>(callable);
    }

    template<typename T, typename = std::enable_if<std::is_invocable_v<T, const Args&...>>>
    int connect(T&& callable, bool once = false) {
        int id = counter;
        m_subscribers.emplace_back(counter++, once ? make_one_shot(id, std::forward<T>(callable)) : std::forward<T>(callable));
        return id;
    }

    bool is_connected(int key) {
        return get_subscriber(key) != nullptr;
    }

    void disconnect(int key) {
        for(std::size_t i = 0; i < m_subscribers.size(); i++){
            if(key == m_subscribers[i].first){
                m_subscribers.erase(m_subscribers.begin() + i);
                return;
            }
        }
    }

    void emit(const Args&... args) {
        for (const auto&[_,f] : m_subscribers) {
            f(args...);
        }
    }

    void clear(){
        m_subscribers.clear();
    }

    int get_counter() {
        return counter;
    }

    void reset_counter() {
        counter = INT_MIN;
    }

};

