#pragma once
#include "DependentSystem.hpp"
#include <memory>
#include <unordered_map>
#include <iostream>

struct SystemNode {
    std::shared_ptr<ISystem> system;
    size_t index = 0;
    std::vector<size_t> dependencies{};
    std::vector<size_t> dependents{};
    size_t dependenciesNotReady = 0;
    bool ran = false;

    // override ->
    std::shared_ptr<ISystem> operator ->() {
        return system;
    }
};

class SystemGraph {
public:
    SystemGraph() = default;
    SystemGraph(const SystemGraph&) = delete;
    SystemGraph(SystemGraph&&) = delete;
    SystemGraph& operator=(const SystemGraph&) = delete;
    SystemGraph& operator=(SystemGraph&&) = delete;
    ~SystemGraph() = default;

    // Use the returned pointer if the system needs initialization
    template <typename T>
    std::shared_ptr<T> addSystem()
    {
        auto system = std::make_shared<T>();
        nodes.push_back({system, nodes.size()});
        auto node = nodes.back().index;
        if (systemMap.find(typeid(T)) != systemMap.end()) {
            auto name = nodes[node]->name();
            throw std::runtime_error("System (" + std::string(name) + ") already added to graph.");
        }
        systemMap[typeid(T)] = node;

        auto writeTypes = nodes[node]->writes();
        for (auto& type : writeTypes) {
            this->writes[type].push_back(node);
            for (auto read : reads[type]) {
                auto readNode = &nodes[read];
                readNode->dependencies.push_back(node);
                nodes[node].dependents.push_back(read);
            }
        }

        auto readTypes = nodes[node]->reads();
        for (auto& type : readTypes) {
            this->reads[type].push_back(node);
            for (auto write : this->writes[type]) {
                auto writeNode = &nodes[write];
                writeNode->dependents.push_back(node);
                nodes[node].dependencies.push_back(write);
            }
        }
        return system;
    }

    void init(entt::registry& registry) {
        for (auto& node : nodes) {
            node->init(registry);
        }
    }

    void update(entt::registry& registry, float delta) {
        std::vector<size_t> readyNodes;
        for (auto& node : nodes) {
            node.dependenciesNotReady = node.dependencies.size();
            node.ran = false;
            if (node.dependenciesNotReady == 0) {
                readyNodes.push_back(node.index);
            }
        }

        while (!readyNodes.empty()) {
            auto node = readyNodes.back();
            readyNodes.pop_back();
            nodes[node]->update(registry, delta);
            nodes[node].ran = true;
            for (auto dependent : nodes[node].dependents) {
                auto dependentNode = &nodes[dependent];
                dependentNode->dependenciesNotReady--;
                if (dependentNode->dependenciesNotReady == 0) {
                    readyNodes.push_back(dependentNode->index);
                }
                if (dependentNode->dependenciesNotReady < 0) {
                    throw std::runtime_error("SystemGraph::update() - dependency count is less than 0. System name: " +
                        std::string(dependentNode->system->name()));
                }
            }
        }

        for (auto& node : nodes) {
            if (!node.ran) {
                std::cerr << "SystemGraph::update() - system did not run. System name: " << node.system->name() << std::endl;
                std::cerr << "SystemGraph::update() - system dependencies: "; 
                for (auto& dependency : node.dependencies) {
                    std::cerr << nodes[dependency].system->name() << " ";
                }
                std::cerr << std::endl;
                std::cerr << "A cycle likely exists in the system graph." << std::endl;
            }
        }
    }

    void debugPrint() {
        for (auto& node : nodes) {
            auto name = node.system->name();
            std::cout << name << " depends on: (";
            for (int i = 0; i < node.dependencies.size(); i++) {
                auto dependency = node.dependencies[i];
                auto dependencyName = nodes[dependency].system->name();
                std::cout << dependencyName << (i + 1 < node.dependencies.size() ? ", " : "");
            }
            std::cout << ") and is depended on by: (";
            for (int i = 0; i < node.dependents.size(); i++) {
                auto dependent = node.dependents[i];
                auto dependentName = nodes[dependent].system->name();
                std::cout << dependentName << (i + 1 < node.dependents.size() ? ", " : "");
            }
            std::cout << ")" << std::endl;
        }
    }

private:
    std::unordered_map<std::type_index, size_t> systemMap;
    std::unordered_map<std::type_index, std::vector<size_t>> writes;
    std::unordered_map<std::type_index, std::vector<size_t>> reads;
    std::vector<SystemNode> nodes;
};