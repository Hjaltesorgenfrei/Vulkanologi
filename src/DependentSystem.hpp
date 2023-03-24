#pragma once
#include <typeinfo>
#include <typeindex>
#include <unordered_set>
#include <string>
#include <entt/entt.hpp>

struct System {
    virtual const char* name() = 0;
    virtual constexpr std::unordered_set<std::type_index> reads() = 0;
    virtual constexpr std::unordered_set<std::type_index> writes() = 0;
    virtual void update(entt::registry& registry, float delta) const = 0;
};

template<size_t N>
struct StringLiteral {
    constexpr StringLiteral(const char (&str)[N]) {
        std::copy_n(str, N, value);
    }
    
    char value[N];
};

template <typename... ReadArgs>
struct Reads {
    template <typename... WriteArgs>
    struct Writes { 
        template <StringLiteral Name>
        struct Named : System {
    protected:
            virtual void run(float delta, ReadArgs..., WriteArgs&...) const = 0;
            
    public:
            static constexpr auto contents = Name.value;

            virtual const char* name() override {
                return contents;
            }

            virtual constexpr std::unordered_set<std::type_index> reads() {
                static auto set = std::unordered_set<std::type_index>{ typeid(ReadArgs)... };
                return set;
            }

            virtual constexpr std::unordered_set<std::type_index> writes() {
                static auto set = std::unordered_set<std::type_index>{ typeid(WriteArgs)... };
                return set;
            }
        
            virtual void update(entt::registry& registry, float delta) const override {
                auto view = registry.view<ReadArgs..., WriteArgs...>();
                for (auto entity : view) {
                    run(delta, view.get<ReadArgs>(entity)..., view.get<WriteArgs>(entity)...);
                }
            }
        };
    };
};