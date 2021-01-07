#include <iostream>
#include <string>
#include <vector>
#include <sol/sol.hpp>
#include <yaml-cpp/yaml.h>

#include <map>
#include <vector>
#include <string>
#include <spdlog/spdlog.h>
#include <memory>
#include <cstdlib>
#include <any>

#include <iostream>
#include <unistd.h>
#include <wait.h>

#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <optional>
#include <nlohmann/json.hpp>
#include "actions.hpp"
#include <algorithm>
#include <tuple>
#include <filesystem>
#include <docopt/docopt.h>

namespace fs = std::filesystem;
using nlohmann::json;
using std::find_if;
using std::make_tuple;
using std::optional;
using std::string;
using std::to_string;
using std::tuple;
using std::vector;

struct User
{
    double id;
};

void to_json(json &j, const User &p)
{
    j = json{{"id", p.id}};
}

void from_json(const json &j, User &p)
{
    j.at("id").get_to(p.id);
}

struct Item
{
    Item() = default;
    Item(int _id, string _name, string _desc, int _count) : id(_id), name(_name), description(_desc), count(_count) {}
    Item(int _id, string _name, string _desc, int _count, int _price) : id(_id), name(_name), description(_desc), count(_count), price(_price) {}
    Item(int _id, string _name, string _desc) : id(_id), name(_name), description(_desc), count(-1) {}

    int id;
    string name;
    string description;
    int count;
    int price = std::rand() % 100;
};

class Shop
{
public:
    Shop(int items_count)
    {
        for (int item_id = 0; item_id < items_count; item_id++)
        {
            Item item;
            item.id = item_id;
            item.count = 2;
            item.description = string("Item Description") + to_string(item_id);
            item.name = string("Item ") + to_string(item_id);
            items.push_back(item);
        }
    };

    vector<Item> items;
};

void to_json(json &j, const Item &p)
{
    j = json{{"id", p.id}, {"name", p.name}, {"description", p.description}, { "count", p.count }, { "price", p.price }};
}

void from_json(const json &j, Item &p)
{
    j.at("id").get_to(p.id);
    j.at("name").get_to(p.name);
    j.at("description").get_to(p.description);
    j.at("count").get_to(p.count);
    j.at("price").get_to(p.price);
}

enum class PipeRW
{
    eREAD = 0,
    eWRITE = 1,
};

class Pipe
{
public:
    Pipe()
    {
        if (pipe(pipefd) < 0)
        {
            spdlog::critical("Could not create pipes: {} {}", errno, strerror(errno));
            exit(1);
        }

        if (auto fd = static_cast<int>(PipeRW::eREAD); fcntl(pipefd[fd], F_SETFL, fcntl(pipefd[fd], F_GETFL) | O_NONBLOCK) < 0)
        {
            spdlog::critical("Error setting Socket to be NonBlocking for socket {}", pipefd[fd]);
            exit(1);
        }

        read_buffer.resize(max_read_size);
    }

private:
    void ReadToBuffer()
    {
        auto fd = pipefd[static_cast<int>(PipeRW::eREAD)];

        if (auto result = read(fd, &read_buffer[write_index], max_read_size - write_index); result > 0)
        {
            write_index += result;
            ReadToBuffer();
        }
        else if (result < 0 && errno == EWOULDBLOCK)
        {
            return;
        }

        return;
    }

    const optional<string> ReadString()
    {
        if (write_index < 4)
            return {};

        int size = 0;
        size |= ((read_buffer[0] << (8 * 0)));
        size |= ((read_buffer[1] << (8 * 1)));
        size |= ((read_buffer[2] << (8 * 2)));
        size |= ((read_buffer[3] << (8 * 3)));

        if (write_index < size + 4)
            return {};

        std::string result(read_buffer.begin() + 4, read_buffer.begin() + 4 + size);

        read_buffer = vector(read_buffer.begin() + 4 + size, read_buffer.end());
        read_buffer.resize(max_read_size);
        write_index -= size;

        return result;
    }

public:
    optional<string> Read()
    {
        ReadToBuffer();
        return ReadString();
    }

    void Write(const string &str)
    {
        auto fd = pipefd[static_cast<int>(PipeRW::eWRITE)];

        vector<uint8_t> buffer(str.size() + 4);

        auto size = str.size();
        buffer[0] = ((size >> (8 * 0)) & 0xFF);
        buffer[1] = ((size >> (8 * 1)) & 0xFF);
        buffer[2] = ((size >> (8 * 2)) & 0xFF);
        buffer[3] = ((size >> (8 * 3)) & 0xFF);

        for (int i = 0; i < size; i++)
        {
            buffer[i + 4] = str[i];
        }

        write(fd, &buffer[0], buffer.size());
    }

    int pipefd[2];

private:
    int max_read_size = 65535;
    vector<uint8_t> read_buffer;
    int write_index = 0;
};

struct Child
{
    pid_t id;
    Pipe parentWrite;
    Pipe childWrite;
    fs::path script;
    fs::path luaIncludeDir;
};

void SetupLuaBinds(sol::state &lua)
{
    auto item_type = lua.new_usertype<Item>("Item", sol::constructors<Item(), Item(int, string, string, int), Item(int, string, string), Item(int, string, string, int, int)>());
    item_type["id"] = &Item::id;
    item_type["name"] = &Item::name;
    item_type["description"] = &Item::description;
    item_type["count"] = &Item::count;
    item_type["price"] = &Item::price;

    auto pipe_type = lua.new_usertype<Pipe>("Pipe", sol::constructors<Pipe()>());
    pipe_type["write"] = &Pipe::Write;
    pipe_type["read"] = [](Pipe &pipe) {
        const auto &result = pipe.Read();
        return result.has_value() ? result.value() : "";
    };

    auto child_type = lua.new_usertype<Child>("Child", sol::no_construction());
    child_type["read"] = [](Child &child) {
        const auto &result = child.parentWrite.Read();
        return result.has_value() ? result.value() : "";
    };
    child_type["write"] = [](Child &child, const std::string &str) {
        child.childWrite.Write(str);
    };
    child_type["id"] = &Child::id;
}

[[noreturn]] void Children(Child child)
{
    sol::state lua;
    lua.open_libraries();
    lua.open_libraries(sol::lib::base, sol::lib::package);
    const std::string package_path = lua["package"]["path"];
    lua["package"]["path"] = package_path + (!package_path.empty() ? ";" : "") + child.luaIncludeDir.string() + std::string("/?.lua;");

    SetupLuaBinds(lua);

    lua["get_child"] = [&]() { return child; };

    lua["log"] = [&](const std::string& str) {
        spdlog::info("{}: {}", child.id, str);
    };

    spdlog::info("{}: Running Script {}", child.id, child.script.string());
    lua.script_file(child.script.string());

    exit(0);
}

void Fork(Child &child)
{
    if (auto id = fork(); id > 0)
    {
        close(child.childWrite.pipefd[static_cast<int>(PipeRW::eWRITE)]);
        close(child.parentWrite.pipefd[static_cast<int>(PipeRW::eREAD)]);
        child.id = id;
        return;
    }
    else if (id == 0)
    {
        close(child.parentWrite.pipefd[static_cast<int>(PipeRW::eWRITE)]);
        close(child.childWrite.pipefd[static_cast<int>(PipeRW::eREAD)]);
        child.id = getpid();

        Children(child);
        exit(0);
    }
    else
    {
        spdlog::critical("Could not fork(): {} {}", errno, strerror(errno));
        exit(1);
    }
}

void Parent(Shop &shop, vector<Child> children)
{
    for (auto &child : children)
        Fork(child);

    Handlers<string, tuple<Child *, json>> handlers;

    handlers.Add("products:get", [&](tuple<Child *, json> _) {
        auto &[child, data] = _;

        auto response_id = data["id"].get<int>();

        json response{
            {"response", response_id},
        };

        int item_count = 0;

        for (auto &item : shop.items)
        {
            if (item.count == 0)
                continue;
            response["data"]["items"].push_back(item);
            item_count++;
        }

        response["data"]["item_count"] = item_count;

        child->parentWrite.Write(response.dump());
    });

    handlers.Add("products:buy", [&](tuple<Child *, json> _) {
        auto &[child, data] = _;

        auto response_id = data["id"].get<int>();

        json response{
            {"response", response_id},
        };

        json item_id = data["data"]["product_id"].get<int>();

        auto item_itr = find_if(shop.items.begin(), shop.items.end(), [&](Item &item) {
            return item.id == item_id;
        });

        if (item_itr == shop.items.end())
        {
            response["error"] = {{"message", "Product ID Not Found"}};
            child->parentWrite.Write(response.dump());
            return;
        }

        auto &item = *item_itr;

        if (item.count == 0)
        {
            response["error"] = {{"message", "Out of Stock"}};
            child->parentWrite.Write(response.dump());
            return;
        }

        item.count--;

        response["data"]["bought"] = true;
        response["data"]["item"] = item;

        spdlog::info("Shop: User {} Bought Item {}. {} left. Price is {}", child->id, item.name, item.count);

        child->parentWrite.Write(response.dump());
    });

    while (std::count_if(shop.items.begin(), shop.items.end(), [&](const Item &item) {
        return item.count > 0;
    }))
    {
        for (auto &child : children)
        {
            auto str = child.childWrite.Read();

            if (!str.has_value() || str.value() == "")
                continue;

            json request = json::parse(str.value());

            if (request["action"].get<string>() == "")
                continue;

            handlers.Run(request["action"].get<string>(), make_tuple(&child, request));
        }
    };
}

constexpr static char USAGE[] = R"(
EShop Usage
    Usage:
      Luafy [options]
      Luafy (-h | --help)
      Luafy --version

    Options:
      -h --help               Show this screen.
      --version               Show version.
      -i --include=<dir>      The Directory to Include in the Lua Scripts [default: ./LuaIncludes].
      --items=<amount>        How many items to generate [default: 100].
      --users=<amount>        How many users to create (forks) [default: 50].
      --scripts=<dir>         The Directory of the User Lua Scripts to Run
                              One of these scripts will be chosen at random
                              to be executed. This will allow the Users to have
                              different characteristics such as:
                              - A user who buys anything at random
                              - A user who buys only when it's the last item
                              - etc
                              [default: ./Users]
)";

int main(int argc, char **argv)
{
    auto args = docopt::docopt(USAGE, {argv + 1, argv + argc}, true, "EShop 1.0");

    const auto scripts = args["--scripts"].isString() ? args["--scripts"].asString() : std::string("./Users");
    const auto includes = args["--include"].isString() ? args["--include"].asString() : std::string("./LuaIncludes");
    const auto forks = args["--users"].isLong() ? static_cast<uint>(args["--users"].asLong()) : static_cast<uint>(std::atoi(args["--users"].asString().data()));
    const auto items = args["--items"].isLong() ? static_cast<uint>(args["--items"].asLong()) : static_cast<uint>(std::atoi(args["--items"].asString().data()));

    const auto scriptsForDir = [](fs::path path) {
        std::vector<fs::path> scripts;

        for (auto &entry : fs::directory_iterator(path))
        {
            if (entry.is_regular_file() && entry.path().extension().string() == ".lua")
            {
                scripts.push_back(entry);
            }
        }

        return scripts;
    };

    auto allScripts = scriptsForDir(scripts);

    for (auto s : allScripts)
    {
        spdlog::info("Script: {}", s.string());
    }

    Shop shop(items);

    vector<Child> children;

    for (int i = 0; i < forks; i++)
    {
        Child child;

        child.luaIncludeDir = includes;
        child.script = allScripts[std::rand() % allScripts.size()];

        children.push_back(child);
    }

    Parent(shop, children);
}
