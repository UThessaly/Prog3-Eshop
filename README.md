# Basic Piped Shop

## Building

To build the Eshop, a couple of programs will be needed:

- Conan
- CMake
- C++17

Docker can also be used to build the image

### Installing Conan

Click here for the full documentation on how to install Conan. 

But these are the commands that you need to execute in order to install Conan

```bash
# Python and Pip
sudo apt install python3 python3-pip

# Downloading Conan
pip3 install conan

# Add conan to Sources
source ~/.profile
```

### Installing CMake

```bash
# Installing the required packages
sudo apt install cmake make g++ gcc build-essential
```

### Building the program

```bash
cd <project-path>

mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug # Or Release, depending on Debug/Release
make
```

### Building using Docker

```bash
cd <project-path>

docker build -t image_example:latest .

# To Run
docker run -it image_example:latest
```

## How the program works

### Pipes

The program uses Pipes in order for the Bi-Directional Communication of Parent <-> Child

**Pipes are in NonBlocking Mode**

#### JSON 

The Pipes use JSON in order to send and receive messages

### Lua Scripts

Lua Scrips are uses in order to Simulate a Customer, and by default there are three scripts that are premade

- A Customer that buys everything
- A Customer that buys only when an item is low in stock
- And a Customer that is limited by how much money they have

#### Sol3

Sol3 is used in order to connect C++ and Lua. Sol3 adds the ability to add custom User types, as well as require other scripts (libraries)

Example:

```cpp
#include <sol/sol.h>
#include <string>
#include <iostream>

using std::string;

int main() {
    struct Item {
        int id;
        int count;
        string name;

        void PrintItem() {
            std::cout << id << count << name << '\n';
        }
    };

    sol::state lua;

    auto item_type = lua.new_usertype<Item>("Item"); // Creates Item.new()
    item_type["id"] = &Item::id;
    // similar for count and name
    item_type["print"] = &Item::PrintItem;

    lua.script(R"(
        local item = Item.new()

        item:print()
    )");
}
```

The above script would print the item's ID, Count and Name

### DocOpt

Docopt is a command line interface description module. It makes it super easy to parse command line arguments

### SpdLog

SpdLog is a super fast and super easy to use Logger