#include <SFML/Graphics.hpp>
#include <cassert>
#include <iostream>
#include <string>

#include "../include/SFMLInGameConsole.hpp"

struct MyStruct {
  int id = 0;
  std::string name = "";

  friend std::istream& operator>>(std::istream& is, MyStruct& obj) {
    is >> obj.id >> obj.name;
    return is;
  }

  friend std::ostream& operator<<(std::ostream& os, const MyStruct& obj) {
    os << "ID: " << obj.id << ", Name: " << obj.name;
    return os;
  }
};

int main() {
  sf::RenderWindow window(sf::VideoMode({1280u, 720u}), "SFML Game Console");
  window.setFramerateLimit(30);

  sf::Font font;
  const bool loaded = font.loadFromFile("FreeMono.ttf");
  assert(loaded && "Unable to load font FreeMono.ttf");

  sfe::SFMLInGameConsole console(font);
  console.show(true);
  console.SetTextLeftOffset(0.F);
  console.SetMaxInputLineSymbols(30);
  console.SetConsoleHeightPart(0.7);

  int varInt = 1;
  std::string varStr = "string";
  MyStruct varCustom{2, "custom struct"};

  console.bindCVar("varInt", varInt, "Int variable");
  console.bindCVar("varStr", varStr, "String variable");
  console.bindCVar("varCustom", varCustom, "Custom struct variable");

  console.bindCommand(
      "sum",
      [&console](int a, int b) {
        console << sfe::TEXT_COLOR_GREEN << a + b << sfe::TEXT_COLOR_RESET
                << std::endl;
      },
      "Print sum of given numbers");

  console.SetCommandKeywords("sum", { "10", "100", "200", "300" });

  while (window.isOpen()) {
    sf::Event event;
    while (window.pollEvent(event)) {
      if (console.visible()) {
        console.HandleUIEvent(event);
      }
      if (event.type == sf::Event::KeyPressed &&
          event.key.code == sf::Keyboard::F10) {
        console.show(!console.visible());
      }
      if (event.type == sf::Event::Closed) window.close();
    }

    window.clear(sf::Color(128u, 128u, 128u));

    console.Render(&window);
    window.display();
  }
}