#pragma once
#include <typeinfo>
#include <typeindex>
#include <unordered_set>
#include <string>
#include <entt/entt.hpp>

struct ISystem
{
    virtual const std::string_view name() = 0;
    virtual constexpr std::unordered_set<std::type_index> reads() = 0;
    virtual constexpr std::unordered_set<std::type_index> writes() = 0;
    virtual void update(entt::registry &registry, float delta) const = 0;
    virtual void init(entt::registry &registry) {}
};

template <typename... T>
struct Reads : entt::type_list<T...>
{
};

template <typename... T>
struct Writes : entt::type_list<T...>
{
};

template <typename... T>
struct Others : entt::type_list<T...>
{
};

template <typename Self, typename Reads, typename Writes, typename Others>
struct System;

template <typename Self, typename... Read, typename... Write, typename... Other>
struct System<Self, Reads<Read...>, Writes<Write...>, Others<Other...>> : virtual ISystem
{
    virtual const std::string_view name() override {
        static std::string name = typeid(Self).name();
        return std::string_view(name.begin() + 6, name.end());
    }

    virtual constexpr std::unordered_set<std::type_index> reads() {
        static auto set = std::unordered_set<std::type_index>{ typeid(Read)... };
        return set;
    }

    virtual constexpr std::unordered_set<std::type_index> writes() {
        static auto set = std::unordered_set<std::type_index>{ typeid(Write)... };
        return set;
    }

    void update(entt::registry &registry, float delta) const override
    {
        registry.view<Read..., Write..., Other...>().each([this, &registry, delta](entt::entity ent, Read const &...reads, Write&...writes, Other&... others)
                                                { static_cast<Self const *>(this)->update(registry, delta, ent, reads..., writes..., others...); });
    }
};

template <typename Self, typename... Write>
using WSystem = System<Self, Reads<>, Write..., Others<>>;

template <typename Self, typename... Read>
using RSystem = System<Self, Read..., Writes<>, Others<>>;

template <typename Self, typename... Other>
using OSystem = System<Self, Reads<>, Writes<>, Other...>;
