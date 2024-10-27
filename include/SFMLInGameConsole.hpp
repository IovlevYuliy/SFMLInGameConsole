///
///  SFMLInGameConsole.h - Single Header Quake Console Library
///  Created by Yuliy Iovlev on 10/26/24.
///
/// This file is a single header frontend gui widget corresponding to (and
/// depending on) QuakeStyleConsole.h in this repository
///
///
/// For example programs see the demos folder in this repo.
///
/// The easiest way to use this file is to include it and then create an
/// SFMLInGameConsole in an application that supports IMGUI rendering, then call
/// SFMLInGameConsole::Render during your application update loop.  CTRL-F
/// SFMLInGameConsole class definition and look at the sample code in the demos
/// filter to get started.
///
/// This file contains base classes, out of which the SFMLInGameConsole is
/// composed, and you can experiment with as well.
///
/// The text area of the console is an IMGUIOstream, which inherits from
/// std::ostream and you can do all the things that implies. The IMGUIOstream's
/// streambuf parses ANSI color codes so you can add color formatting that way.
///
/// IMGUIInputLine handles some IMGUI callbacks, and pushes user input into a
/// stringstream on enter.  You can get the stream directly or get a line.
///
/// Both the Input Line and the Ostream have render methods that draw them in
/// whatever surrounding IMGUI context the caller has, and also helper methods
/// to draw them in their own windows.
///
/// a MultiStream is an ostream that forwards input to multiple other ostreams.
/// The console widget is a 'multistream' so you can mirror console output to a
/// file or cout or whatever other streams you need.

/*
 This software is available under 2 licenses -- choose whichever you prefer.
 Either MIT or public domain.
*/

#pragma once

#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Window/Event.hpp>
#include <algorithm>
#include <cstring>
#include <unordered_set>

#include "ConsoleBuffer.hpp"
#include "QuakeStyleConsole.h"
#include "RichText.hpp"

namespace sf {
class RenderTarget;
}

namespace sfe {

inline constexpr std::string_view TEXT_COLOR_RESET = "\u001b[0m";
inline constexpr std::string_view TEXT_COLOR_BLACK = "\u001b[30m";
inline constexpr std::string_view TEXT_COLOR_RED = "\u001b[31m";
inline constexpr std::string_view TEXT_COLOR_GREEN = "\u001b[32m";
inline constexpr std::string_view TEXT_COLOR_YELLOW = "\u001b[33m";
inline constexpr std::string_view TEXT_COLOR_BLUE = "\u001b[34m";
inline constexpr std::string_view TEXT_COLOR_MAGENTA = "\u001b[35m";
inline constexpr std::string_view TEXT_COLOR_CYAN = "\u001b[36m";
inline constexpr std::string_view TEXT_COLOR_WHITE = "\u001b[37m";

/// streambuffer implementation for MultiStream
class MultiStreamBuffer : public std::streambuf {
 public:
  std::unordered_set<std::ostream*> streams;

  MultiStreamBuffer() {}

  int overflow(int in) override {
    char c = in;  ///\todo check for eof, etc?
    for (std::ostream* str : streams) {
      (*str) << c;
    }
    return 1;
  }

  std::streamsize xsputn(const char* s, std::streamsize n) override {
    std::streamsize ssz = 0;

    for (std::ostream* str : streams) {
      ssz = str->rdbuf()->sputn(s, n);
    }

    return ssz;
  }
};

/// An ostream that is actually a container of ostream pointers, that pipes
/// output to every ostream in the container
class MultiStream : public std::ostream {
  MultiStreamBuffer buf;

 public:
  MultiStream() : std::ostream(&buf) {}

  void AddStream(std::ostream& str) { buf.streams.insert(&str); }
};

/// Quake style console : IMGUI Widget
/// The widget IS-A MultiStream, so you can call .addStream() to add additional
/// streams to mirror the output - like a file or cout A MultiStream IS-A
/// ostream, so you can write to it with << and pass it to ostream functions You
/// can also get its streambuf and pass it to another ostream, such as cout so
/// that those ostreams write to the console.  eg.  cout.rdbuf(console.rdbuf())
class SFMLInGameConsole : protected Virtuoso::QuakeStyleConsole,
                          public MultiStream {
 public:
  static constexpr sf::Color kDefaultBackgroundColor{0u, 0u, 0u, 140u};

  SFMLInGameConsole(sf::Font font);

  // Setters
  void SetBackgroundColor(sf::Color color);
  void SetFontScale(float scale);
  void SetPosition(sf::Vector2f pos);
  void SetMaxInputLineSymbols(size_t count);
  void SetTextLeftOffset(float offset_part);
  void SetConsoleHeightPart(float height_part);

  sf::Font* Font();  ///< Returns pointer to the current font

  void clear();  ///< clear the output pane

  void show(bool v = true);

  bool visible() const;

  void HandleUIEvent(const sf::Event& e);

  // Renders the console in the provided target based on its sizes
  void Render(sf::RenderTarget* window);

 private:
  void ScrollCallback(const sf::Event& e);
  void HistoryCallback(const sf::Event& e);
  void TextCompletionCallback();

  void UpdateDrawnText();

  ConsoleBuffer console_buffer_;  ///< custom streambuf
  std::ostream console_stream_;

  std::string buffer_text_;
  int scroll_offset_y_ = 0;
  float all_lines_height_ = 0.F;
  size_t max_input_line_symbols_ = 100u;
  float text_left_offset_part_ = 0.005F;
  float console_height_part_ = 0.6F;

  sf::Vector2f position_;
  // SFML rendering variables
  sf::Color background_color_ = kDefaultBackgroundColor;
  sf::Font font_;
  sf::RectangleShape background_rect_;
  sfe::RichText output_text_;
  sfe::RichText input_line_;

  bool shown_ = false;
  // Index into the console history buffer, for when we
  // press up/down arrow to scroll previous commands
  int history_pos_ = -1;
  size_t cursor_pos_ = 0;
  float font_scale_ = 0.6F;  ///< text scale for the console widget window
};

}  // namespace sfe