#include <SFML/Graphics.hpp>

#include "../include/SFMLInGameConsole.h"
#include "cassert"

int main() {
  sf::RenderWindow window(sf::VideoMode({1280u, 720u}), "sfe::RichText");
  window.setFramerateLimit(30);

  sf::Font font;
  const bool loaded = font.loadFromFile("FreeMono.ttf");
  assert(loaded && "Unable to load font FreeMono.ttf");

  sfe::SFMLInGameConsole console(font);
  console.show(true);

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