# SFML In-Game Console
A fork of the [Virtuoso Console](https://github.com/VirtuosoChris/VirtuosoConsole), this project extends its functionality by implementing an SFML-based console. Unlike the original repository, which uses IMGUI for rendering, this version leverages SFML as the rendering engine.

# Overview
The `SFMLInGameConsole` is a robust in-game console widget designed for SFML applications, offering Quake-style console functionality with color formatting, command history, and text autocompletion.

## Features
* **SFML Rendering**: Integrates seamlessly with SFML projects for consistent graphics rendering.
* **Modular Codebase**: Consists of multiple headers and source files, simplifying customization and maintenance.
* **ANSI Color Support**: Enables colorful and styled console output through ANSI codes.
* **Stream Mirroring**: Output can be mirrored to various streams, including files and `std::cout`.
* **Customizable UI**: Options for font scaling, background color, position, and console size.
* **Command Autocompletion**: Supports custom command keywords for intuitive text entry.

## Built-in commands
The following commands built in to every instance of the `SFMLInGameConsole` class

* `echo` – Prints the value of a variable.
* `help` – With no arguments, prints a general help string on how to use the console.
With an argument string naming a variable or a command, prints the help string for that variable or command.
* `listVars` – Lists all variables bound to the console, including dynamically created ones.
* `listCmd` – Lists all commands bound to the console.
* `listHelp` – Lists all the commands and variables that have an available help string for the "help" command.
* `runFile` – Runs commands in a text file named by the argument. Example: `runFile game.ini`.
* `set` – Assigns a value to a variable. Uses istream `operator >>` for parsing.
* `var` – Declares a variable dynamically.

# Getting Started

### Requirements

* **SFML Library**: Ensure SFML is installed and properly configured in your development environment.

### Installation
1. Clone this repository.
2. Include the headers and sources in your SFML project.
3. Link SFML libraries in your build configuration.

### Usage
Include the headers in your project, instantiate `SFMLInGameConsole`, and invoke `Render()` within your main rendering loop.
For more detailed information about the console’s underlying structure, please refer to the documentation in the [original repository](https://github.com/VirtuosoChris/VirtuosoConsole)

```cpp
#include "SFMLInGameConsole.h"

// Create console with custom font
sf::Font font;
font.loadFromFile("path/to/font.ttf");
sfe::SFMLInGameConsole console(font);

// Main render loop
while (window.isOpen()) {
    sf::Event event;
    while (window.pollEvent(event)) {
        console.HandleUIEvent(event);
    }

    window.clear();
    console.Render(&window);
    window.display();
}
```

#### Customization

* **Set Background Color**: `console.SetBackgroundColor(sf::Color::Red);`
* **Adjust Font Scale**: `console.SetFontScale(1.2f);`
* **Set Console Position**: `console.SetPosition(sf::Vector2f(10.f, 10.f));`
* **Configure Command Autocomplete**: `console.SetCommandKeywords("help", {"list", "info", "keyword"});`

Check the demos folder for more examples.

# License

This project is dual-licensed under either the **MIT License** or **Public Domain**; choose the one that best suits your needs.

# Credits
* **Forked from**: [Virtuoso Console](https://github.com/VirtuosoChris/VirtuosoConsole)
* **Extended by**: Yuliy Iovlev (October 26, 2024)