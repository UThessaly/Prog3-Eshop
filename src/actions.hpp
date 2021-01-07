#pragma once

#include <vector>
#include <string>
#include <map>
#include <iostream>
#include <functional>

struct HandlerData
{
    HandlerData(std::vector<std::uint8_t> buffer);

    std::vector<std::uint8_t> data;
};

/**
     * @brief Run functions depending on the Key ID
     * 
     * @tparam Key The ID Type
     * @tparam Data The Data Type to pass to the functions
     */
template <typename Key = std::int32_t, typename Data = HandlerData>
class Handlers
{
private:
    typedef std::function<void(const Data &data)> ActionHandler;

public:
    Handlers() = default;

    /**
         * @brief Runs all Action Handlers for that particual ID, with the given data
         * 
         * @param id The ID of the Action Handler
         * @param data The Data of the Action Handler
         */
    void Run(const Key &id, const Data &data) const
    {
        if (handlers.count(id) == 0)
            return Run(defaultHandlers, data);

        auto &handlers = this->handlers.at(id);
        return Run(handlers, data);
    };

    /**
         * @brief Adds a Handler for that particual key
         * 
         * @param id The ID of the Action Handler
         * @param handler The Handler to Run
         */
    void Add(const Key &id, ActionHandler handler)
    {
        if (handlers.count(id) == 0)
        {
            handlers[id] = {};
        }

        handlers.at(id).push_back(handler);
    };

    // /**
    //  * @brief Adds a Handler for that particual key
    //  *
    //  * @param id The ID of the Action Handler
    //  * @param handler The Handler to Run
    //  */
    // void Add(const Key id, ActionHandler handler)
    // {
    //     if (handlers.count(id) == 0)
    //     {
    //         handlers[id] = {};
    //     }

    //     handlers.at(id).push_back(handler);
    // };

    /**
         * @brief Adds a Handler as a Default Handler
         * 
         * @param handler The Default Handler
         */
    void Add(ActionHandler handler)
    {
        defaultHandlers.push_back(handler);
    }

private:
    /**
         * @brief Runs a set of Handlers
         * 
         * @param handlers A vector of Handlers to Run
         * @param data The Data to pass into those Handlers
         */
    void Run(const std::vector<ActionHandler> &handlers, const Data &data) const
    {
        for (auto &handler : handlers)
        {
            handler(data);
        }
    }

    std::vector<ActionHandler> defaultHandlers;

    std::map<Key, std::vector<ActionHandler>> handlers;
};