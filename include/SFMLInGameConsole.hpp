///
/// SFMLInGameConsole.hpp
/// Created by Yuliy Iovlev on 10/26/24.
///
/// This file provides a GUI widget based on SFML for in-game console
/// functionality, dependent on QuakeStyleConsole.h within this repository.
///
/// Usage:
/// Include the headers and sources in your project and instantiate
/// SFMLInGameConsole within an SFML application. Call SFMLInGameConsole::Render
/// during the application's update loop to display the console.
/// For examples, see the demos folder.
///
/// Components:
/// - ConsoleBuffer: Manages the console's text area, supports ANSI color codes.
/// - MultiStream: A derived ostream that duplicates output across multiple
/// streams.
///
/// License:
/// Available under MIT or public domain license; choose whichever you prefer.
///

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

// ANSI color codes for text formatting within the console.
inline constexpr std::string_view TEXT_COLOR_RESET = "\u001b[0m";
inline constexpr std::string_view TEXT_COLOR_BLACK = "\u001b[30m";
inline constexpr std::string_view TEXT_COLOR_RED = "\u001b[31m";
inline constexpr std::string_view TEXT_COLOR_GREEN = "\u001b[32m";
inline constexpr std::string_view TEXT_COLOR_YELLOW = "\u001b[33m";
inline constexpr std::string_view TEXT_COLOR_BLUE = "\u001b[34m";
inline constexpr std::string_view TEXT_COLOR_MAGENTA = "\u001b[35m";
inline constexpr std::string_view TEXT_COLOR_CYAN = "\u001b[36m";
inline constexpr std::string_view TEXT_COLOR_WHITE = "\u001b[37m";

// MultiStreamBuffer class: streambuffer implementation that forwards input to
// multiple ostreams.
class MultiStreamBuffer : public std::streambuf {
 public:
  // Set of output streams to forward data to.
  std::unordered_set<std::ostream*> streams;

  MultiStreamBuffer() {}

  // Overrides overflow to write a single character to each ostream in streams.
  int overflow(int in) override {
    char c = in;  ///\todo check for eof, etc?
    for (std::ostream* str : streams) {
      (*str) << c;
    }
    return 1;
  }

  // Override xsputn to write a string of characters to each ostream in streams.
  std::streamsize xsputn(const char* s, std::streamsize n) override {
    std::streamsize ssz = 0;

    for (std::ostream* str : streams) {
      ssz = str->rdbuf()->sputn(s, n);
    }

    return ssz;
  }
};

// MultiStream class: ostream that duplicates output to multiple other streams.
class MultiStream : public std::ostream {
  // Underlying buffer for managing output duplication.
  MultiStreamBuffer buf;

 public:
  MultiStream() : std::ostream(&buf) {}

  // Adds a stream to the buffer, allowing output to be mirrored to it.
  void AddStream(std::ostream& str) { buf.streams.insert(&str); }
};

/// SFMLInGameConsole class: main class representing an in-game console widget
/// using SFML. Extends QuakeStyleConsole and MultiStream for SFML-based
/// rendering and stream duplication.
class SFMLInGameConsole : public Virtuoso::QuakeStyleConsole,
                          public MultiStream {
 public:
  // Default console background color.
  static constexpr sf::Color kDefaultBackgroundColor{0u, 0u, 0u, 140u};
  // Default size of the command buffer for storing command history.
  static constexpr size_t kCommandHistoryBufferSize = 100u;

  // Constructor that initializes the console with a given font.
  SFMLInGameConsole(sf::Font font,
                    size_t command_history_size = kCommandHistoryBufferSize);

  // Setters for configuring console appearance and behavior.

  // Sets the console background color.
  void SetBackgroundColor(sf::Color color);
  // Sets the font scaling factor.
  void SetFontScale(float scale);
  // Sets the position of the console relative to the render target.
  void SetPosition(sf::Vector2f pos);
  // Sets the maximum symbols in the input line.
  void SetMaxInputLineSymbols(size_t count);
  // Sets the offset for text from the left margin.
  void SetTextLeftOffset(float offset_part);
  // Sets the console's height as a fraction of the render target height.
  void SetConsoleHeightPart(float height_part);

  /// Registers command keywords for autocomplete functionality.
  void SetCommandKeywords(const std::string& cmd_name,
                          std::vector<std::string> keywords);

  // Returns pointer to the current font.
  sf::Font* Font();
  // Clears the output pane.
  void clear();
  // Sets the console visibility.
  void show(bool v = true);
  // Returns whether the console is currently visible.
  bool visible() const;
  // Handles user input events, updating console behavior based on interactions.
  void HandleUIEvent(const sf::Event& e);

  // Renders the console to a specified SFML RenderTarget (window or other
  // renderable).
  void Render(sf::RenderTarget* window);

 private:
  // Event handlers and internal functionality for specific input and rendering
  // behaviors.
  void ScrollCallback(const sf::Event& e);  // Handles scroll events.
  void HistoryCallback(
      const sf::Event& e);          // Handles up/down history navigation.
  void TextAutocompleteCallback();  // Handles text autocomplete actions.

  /// Gets possible autocomplete suggestions for a word, based on commands and
  /// keywords.
  std::vector<std::string> GetCandidatesForAutocomplete(
      const std::string& cur_word, bool is_first_word) const;

  // Updates text rendering for the current output.
  void UpdateDrawnText();

  typedef std::unordered_map<std::string, std::vector<std::string>>
      CommandKeywordsMapping;

  // Map of commands and their associated keywords.
  CommandKeywordsMapping cmd_keywords_;

  // Custom buffer for the console's output pane.
  ConsoleBuffer console_buffer_;
  // Stream for handling console output.
  std::ostream console_stream_;

  // Text buffer for console input.
  std::string buffer_text_;
  // Vertical scroll offset for console content.
  int scroll_offset_y_ = 0;
  // Total height of all displayed lines.
  float all_lines_height_ = 0.F;
  // Max characters for a single line of input.
  size_t max_input_line_symbols_ = 100u;
  // Left offset for console text rendering.
  float text_left_offset_part_ = 0.005F;
  // Console height as a percentage of screen height.
  float console_height_part_ = 0.6F;

  // Position of the console on the screen.
  sf::Vector2f position_;
  // Background color for the console.
  sf::Color background_color_ = kDefaultBackgroundColor;
  // Font used for console text.
  sf::Font font_;
  // Rectangle shape for console background.
  sf::RectangleShape background_rect_;
  // Rich text object for output text formatting.
  sfe::RichText output_text_;
  // Rich text object for input line formatting.
  sfe::RichText input_line_;

  // Boolean indicating if the console is shown.
  bool shown_ = false;
  // Current position in command history for navigation.
  int history_pos_ = -1;
  // Cursor position in the input line.
  size_t cursor_pos_ = 0;
  // Scaling factor for console text.
  float font_scale_ = 0.6F;
};

}  // namespace sfe