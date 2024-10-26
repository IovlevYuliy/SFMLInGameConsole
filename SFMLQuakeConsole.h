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

#include <algorithm>
#include <cstring>
#include <unordered_set>

#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>

#include "QuakeStyleConsole.h"
#include "RichText.hpp"

namespace sfe {

constexpr float kConsoleHeightPart  = 0.6F;
constexpr float kTextLeftOffset     = 0.01F;
constexpr size_t kCommandBufferSize = 100;

inline constexpr std::string_view TEXT_COLOR_RESET   = "\u001b[0m";
inline constexpr std::string_view TEXT_COLOR_BLACK   = "\u001b[30m";
inline constexpr std::string_view TEXT_COLOR_RED     = "\u001b[31m";
inline constexpr std::string_view TEXT_COLOR_GREEN   = "\u001b[32m";
inline constexpr std::string_view TEXT_COLOR_YELLOW  = "\u001b[33m";
inline constexpr std::string_view TEXT_COLOR_BLUE    = "\u001b[34m";
inline constexpr std::string_view TEXT_COLOR_MAGENTA = "\u001b[35m";
inline constexpr std::string_view TEXT_COLOR_CYAN    = "\u001b[36m";
inline constexpr std::string_view TEXT_COLOR_WHITE   = "\u001b[37m";

enum AnsiColorCode {
  ANSI_RESET = 0,

  ANSI_BLACK   = 30,
  ANSI_RED     = 31,
  ANSI_GREEN   = 32,
  ANSI_YELLOW  = 33,
  ANSI_BLUE    = 34,
  ANSI_MAGENTA = 35,
  ANSI_CYAN    = 36,
  ANSI_WHITE   = 37,
};

inline sf::Color GetAnsiTextColor(AnsiColorCode code) {
  switch (code) {
    case ANSI_RESET:
      return sf::Color::White;
    case ANSI_BLACK:
      return sf::Color::Black;
    case ANSI_RED:
      return sf::Color::Red;
    case ANSI_GREEN:
      return sf::Color::Green;
    case ANSI_YELLOW:
      return sf::Color::Yellow;
    case ANSI_BLUE:
      return sf::Color::Blue;
    case ANSI_MAGENTA:
      return sf::Color::Magenta;
    case ANSI_CYAN:
      return sf::Color::Cyan;
    case ANSI_WHITE:
      return sf::Color::White;
    default:
      return sf::Color::Black;
  }
}

/// Stream Buffer for the IMGUI Console Terminal.  Breaks text stream into
/// Lines, which are an array of formatted text sequences Formatting is
/// presently handled via ANSI Color Codes.  Some other input transformation can
/// be applied to the input before it hits this stream eg. to do syntax
/// highlighting, etc.
class ConsoleBuffer : public std::streambuf {
 public:
  ConsoleBuffer();

  struct TextSequence {
    sf::Color text_color = sf::Color::White;
    std::string text     = "";
  };

  struct Line {
    std::vector<TextSequence> sequences;

    inline TextSequence& CurSequence() {
      return sequences[sequences.size() - 1];
    }
    inline const TextSequence& CurSequence() const {
      return sequences[sequences.size() - 1];
    }

    inline bool IsEmpty() const {
      return std::all_of(sequences.begin(), sequences.end(),
                         [](const auto& seq) { return seq.text.empty(); });
    }
  };

  void clear();

  inline const std::vector<Line>& GetLines() const { return lines; }

 protected:
  int overflow(int c) override;  // -- streambuf overloads --

  /// change formatting state based on an integer code in the ansi-code input
  /// stream.  Called by the streambuf methods
  inline void ProcessANSICode(int code);

  inline Line& CurrentLine() { return lines[lines.size() - 1]; }
  inline std::string& CurrentWord() { return CurrentLine().CurSequence().text; }

  ///< ANSI color code we last saw for text
  sf::Color cur_text_color = sf::Color::White;

  // ANSI color code parser state variable
  bool parsing_ansi_code = false;
  // ANSI color code parser is listening for next digit
  bool listening_digits = false;
  // ANSI color code parser accumulator
  std::stringstream num_parse_stream;
  // All output lines
  std::vector<Line> lines;
};

/// streambuffer implementation for MultiStream
class MultiStreamBuffer : public std::streambuf {
 public:
  std::unordered_set<std::ostream*> streams;

  MultiStreamBuffer() {}

  int overflow(int in) override;

  std::streamsize xsputn(const char* s, std::streamsize n);
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
class SFMLInGameConsole : public MultiStream {
 public:
  SFMLInGameConsole(sf::Font font);

  sf::Font* Font();  ///< Returns pointer to the current font

  void clear() {
    console_buffer_.clear();
    scroll_offset_y_ = 0.F;
  }  ///< clear the output pane

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

  Virtuoso::QuakeStyleConsole
      console_;  ///< implementation of the quake style console

  ConsoleBuffer console_buffer_;  ///< custom streambuf
  std::ostream console_stream_;

  std::string buffer_text_;
  int scroll_offset_y_    = 0;
  float all_lines_height_ = 0.F;

  sf::Font font_;
  sf::RectangleShape background_rect_;
  sfe::RichText output_text_;
  sfe::RichText input_line_;

  bool shown_ = false;
  // Index into the console history buffer, for when we
  // press up/down arrow to scroll previous commands
  int history_pos_   = -1;
  size_t cursor_pos_ = 0;
  float font_scale_  = 0.6F;  ///< text scale for the console widget window
};

// -------------------------------------------
// --- SFMLInGameConsole Implementation --- //
// -------------------------------------------

inline SFMLInGameConsole::SFMLInGameConsole(sf::Font font)
    : console_(kCommandBufferSize),
      console_stream_(&console_buffer_),
      font_(std::move(font)) {
  AddStream(console_stream_);

  console_.bindMemberCommand("clear", *this, &SFMLInGameConsole::clear,
                             "Clear the console");
  console_.bindCVar("consoleTextScale", font_scale_);

  console_.style = Virtuoso::QuakeStyleConsole::ConsoleStylingColor();
}

inline sf::Font* SFMLInGameConsole::Font() { return &font_; }

inline void SFMLInGameConsole::show(bool v) { shown_ = v; }

inline bool SFMLInGameConsole::visible() const { return shown_; }

inline void SFMLInGameConsole::UpdateDrawnText() {
  input_line_.clear();
  input_line_.setFont(font_);
  input_line_.setScale({font_scale_, font_scale_});
  input_line_ << "> " << buffer_text_.substr(0, cursor_pos_) << "_"
              << buffer_text_.substr(cursor_pos_);

  const float input_line_height = input_line_.getGlobalBounds().height;
  const float console_height    = background_rect_.getSize().y;
  const float left_offset = background_rect_.getSize().x * kTextLeftOffset;
  const float free_height = console_height - input_line_height;

  // Set the position of input line at console's bottom.
  input_line_.setPosition({left_offset, console_height - input_line_height});

  const auto& lines = console_buffer_.GetLines();
  int last_line_idx = static_cast<int>(lines.size());
  if (lines.back().IsEmpty()) {
    --last_line_idx;
  }

  std::optional<int> first_visible_idx, last_visible_idx;
  float hidden_bottom_part = 0.F;

  sfe::RichText temp_text(font_);
  temp_text.setScale({font_scale_, font_scale_});
  for (int i = last_line_idx - 1; i >= 0; --i) {
    for (const auto& seq : lines[i].sequences) {
      temp_text << seq.text;
    }
    float text_height = temp_text.getLocalBounds().height * font_scale_;
    if (scroll_offset_y_ > 0 && !last_visible_idx.has_value() &&
        text_height > scroll_offset_y_) {
      last_visible_idx   = i;
      hidden_bottom_part = text_height;
      first_visible_idx.reset();
    }
    if (!first_visible_idx.has_value() &&
        text_height > free_height + hidden_bottom_part) {
      first_visible_idx = i + 1;
    }
    if (i) {
      temp_text << "\n";
    }
  }

  output_text_.clear();
  output_text_.setFont(font_);
  output_text_.setScale({font_scale_, font_scale_});
  const int begin = first_visible_idx.value_or(0);
  const int end   = last_visible_idx.value_or(last_line_idx);
  for (int i = begin; i < end; ++i) {
    for (const auto& seq : lines[i].sequences) {
      if (!seq.text.empty()) {
        output_text_ << seq.text_color << seq.text;
      }
    }
    if (i + 1 < end) {
      output_text_ << "\n";
    }
  }

  all_lines_height_ = temp_text.getGlobalBounds().height;

  if (all_lines_height_ > free_height) {
    output_text_.setPosition(
        {left_offset, free_height - output_text_.getGlobalBounds().height});
  } else {
    output_text_.setPosition({left_offset, 0.F});
  }
}

inline void SFMLInGameConsole::HandleUIEvent(const sf::Event& e) {
  if (e.type == sf::Event::KeyPressed) {
    switch (e.key.code) {
      case sf::Keyboard::Backspace:
        if (cursor_pos_ > 0) {
          buffer_text_.erase(cursor_pos_ - 1, 1);
          --cursor_pos_;
        }
        return;
      case sf::Keyboard::Delete:
        if (cursor_pos_ < buffer_text_.size()) {
          buffer_text_.erase(cursor_pos_, 1);
        }
        return;
      case sf::Keyboard::Enter:
        console_.commandExecute(buffer_text_, (*this));
        buffer_text_.clear();
        history_pos_     = -1;
        cursor_pos_      = 0;
        scroll_offset_y_ = 0;
        return;
      case sf::Keyboard::Tab:
        TextCompletionCallback();
        return;
      case sf::Keyboard::Up:
        if (e.key.shift) {
          ScrollCallback(e);
        } else {
          HistoryCallback(e);
        }
        return;
      case sf::Keyboard::Down:
        if (e.key.shift) {
          ScrollCallback(e);
        } else {
          HistoryCallback(e);
        }
        return;
      case sf::Keyboard::Left:
        cursor_pos_ = cursor_pos_ > 0 ? cursor_pos_ - 1 : 0;
        return;
      case sf::Keyboard::Right:
        cursor_pos_ = std::min(cursor_pos_ + 1, buffer_text_.size());
        return;
      default:
        return;
    }
  } else if (e.type == sf::Event::TextEntered) {
    // handle ASCII characters only, skipping backspace and delete
    if (e.text.unicode > 31 &&
        e.text.unicode <
            127)  // TODO: && m_commandBuffer.size() < maxBufferLength
    {
      buffer_text_.insert(cursor_pos_, 1, static_cast<char>(e.text.unicode));
      ++cursor_pos_;
    }
  }
}

inline void SFMLInGameConsole::Render(sf::RenderTarget* window) {
  if (!shown_) {
    return;
  }

  background_rect_.setPosition({0.F, 100.F});
  background_rect_.setFillColor(sf::Color(0u, 0u, 0u, 140u));
  background_rect_.setSize(sf::Vector2f(
      window->getSize().x, window->getSize().y * kConsoleHeightPart));

  UpdateDrawnText();

  input_line_.setPosition(input_line_.getPosition() + sf::Vector2f(0.F, 100.F));
  output_text_.setPosition(output_text_.getPosition() +
                           sf::Vector2f(0.F, 100.F));

  window->draw(background_rect_);
  window->draw(output_text_);
  window->draw(input_line_);
}

inline void SFMLInGameConsole::ScrollCallback(const sf::Event& e) {
  const float input_line_height = input_line_.getGlobalBounds().height;
  const float overflow_height   = std::max(
        all_lines_height_ - background_rect_.getSize().y + input_line_height,
        0.F);
  const float step =
      font_.getLineSpacing(input_line_.getCharacterSize() * font_scale_);
  if (e.key.code == sf::Keyboard::Up) {
    scroll_offset_y_ = std::min(scroll_offset_y_ + step, overflow_height);
  } else if (e.key.code == sf::Keyboard::Down) {
    scroll_offset_y_ = std::max(scroll_offset_y_ - step, 0.F);
  }
}

inline void SFMLInGameConsole::HistoryCallback(const sf::Event& e) {
  const int prev_history_pos = history_pos_;
  if (e.key.code == sf::Keyboard::Up) {
    if (history_pos_ == -1) {
      history_pos_ = static_cast<int>(console_.historyBuffer().size()) - 1;
    } else if (history_pos_ > 0) {
      history_pos_--;
    }
  } else if (e.key.code == sf::Keyboard::Down) {
    if (history_pos_ != -1) {
      if (++history_pos_ >= static_cast<int>(console_.historyBuffer().size())) {
        history_pos_ = -1;
      }
    }
  }

  if (prev_history_pos != history_pos_) {
    buffer_text_ =
        history_pos_ >= 0 ? console_.historyBuffer()[history_pos_] : "";
    cursor_pos_ = buffer_text_.size();
  }
}

inline void SFMLInGameConsole::TextCompletionCallback() {
  // Locate beginning of current word
  size_t word_start_pos = cursor_pos_;
  while (word_start_pos > 0) {
    const char c = buffer_text_[word_start_pos - 1];
    if (c == ' ' || c == '\t' || c == ',' || c == ';')
      break;
    word_start_pos--;
  }

  const std::string cur_word =
      buffer_text_.substr(word_start_pos, cursor_pos_ - word_start_pos);

  // No autocompletion for an empty line
  if (cur_word.empty()) {
    return;
  }

  // Build a list of candidates
  std::vector<std::string> candidates;

  // autocomplete commands...
  for (auto it = console_.getCommandTable().begin();
       it != console_.getCommandTable().end(); it++) {
    if (it->first.starts_with(cur_word)) {
      candidates.emplace_back(it->first);
    }
  }

  // ... and autcomplete variables
  for (auto it = console_.getCVarReadTable().begin();
       it != console_.getCVarReadTable().end(); it++) {
    if (it->first.starts_with(cur_word)) {
      candidates.emplace_back(it->first);
    }
  }

  // No matches at all.
  if (candidates.empty()) {
    return;
  }

  if (candidates.size() == 1)  // Single match.
  {
    buffer_text_.resize(word_start_pos + candidates[0].size());
    buffer_text_.replace(word_start_pos, candidates[0].size(), candidates[0]);
    cursor_pos_ = buffer_text_.size();
  } else {
    // Multiple matches. Complete as much as we can..
    // So inputing "C"+Tab will complete to "CL" then display "CLEAR" and
    // "CLASSIFY" as matches.
    int match_len = cur_word.size();
    for (;;) {
      int c                       = 0;
      bool all_candidates_matches = true;
      for (size_t i = 0; i < candidates.size() && all_candidates_matches; i++)
        if (i == 0) {
          c = toupper(candidates[i][match_len]);
        } else if (c == 0 || c != toupper(candidates[i][match_len])) {
          all_candidates_matches = false;
        }
      if (!all_candidates_matches) {
        break;
      }
      match_len++;
    }

    if (match_len > 0) {
      const std::string replacement = candidates[0].substr(0, match_len);
      buffer_text_.resize(word_start_pos + replacement.size());
      buffer_text_.replace(word_start_pos, replacement.size(), replacement);
      cursor_pos_ = buffer_text_.size();
    }

    // List matches
    (*this) << "Possible matches:\n";
    for (size_t i = 0; i < candidates.size(); i++) {
      (*this) << "- " << candidates[i] << '\n';
    }
  }
}

// --------------------------------------
// --- MultiStreamBuf implementation ---
// --------------------------------------

inline int MultiStreamBuffer::overflow(int in) {
  char c = in;  ///\todo check for eof, etc?
  for (std::ostream* str : streams) {
    (*str) << c;
  }
  return 1;
}

inline std::streamsize MultiStreamBuffer::xsputn(const char* s,
                                                 std::streamsize n) {
  std::streamsize ssz = 0;

  for (std::ostream* str : streams) {
    ssz = str->rdbuf()->sputn(s, n);
  }

  return ssz;
}

// --------------------------------------
// ---- ConsoleBuffer implementation -------
// --------------------------------------

inline void ConsoleBuffer::clear() {
  lines.clear();
  lines.emplace_back();
  CurrentLine().sequences.emplace_back(sf::Color::White, "");
}

inline void ConsoleBuffer::ProcessANSICode(int code) {
  switch (code) {
    case ANSI_RESET:
    case ANSI_BLACK:
    case ANSI_RED:
    case ANSI_GREEN:
    case ANSI_YELLOW:
    case ANSI_BLUE:
    case ANSI_MAGENTA:
    case ANSI_CYAN:
    case ANSI_WHITE:
      cur_text_color = GetAnsiTextColor(static_cast<AnsiColorCode>(code));
      break;
    default:
      std::cerr << "unknown ansi code " << code << " in output\n";
      return;
  }
}

inline int ConsoleBuffer::overflow(int c) {
  if (c != EOF) {
    if (parsing_ansi_code) {
      bool error = false;

      if (std::isdigit(static_cast<char>(c)) && listening_digits) {
        num_parse_stream << static_cast<char>(c);
      } else {
        switch (c) {
          case 'm':  // end of ansi code; apply color formatting to new
          {
            parsing_ansi_code = false;
            int x;
            if (num_parse_stream >> x) {
              ProcessANSICode(x);
            }
            num_parse_stream.clear();
            CurrentLine().sequences.emplace_back(cur_text_color, "");
            break;
          }
          case '[': {
            listening_digits = true;
            num_parse_stream.clear();
            break;
          }
          case ';': {
            int x;
            num_parse_stream >> x;
            num_parse_stream.clear();
            ProcessANSICode(x);
            break;
          }
          default: {
            error = true;
            break;
          }
        }

        if (error) {
          num_parse_stream.clear();
          listening_digits  = false;
          parsing_ansi_code = false;

          std::cerr << "Parsing ANSI code failed. Unknown symbol "
                    << static_cast<char>(c);
        }
      }
    } else {
      switch (c) {
        case '\u001b': {
          parsing_ansi_code = true;
          num_parse_stream.clear();
          break;
        }
        case '\n': {
          // currentline add \n
          lines.emplace_back();
          CurrentLine().sequences.emplace_back(cur_text_color, "");
          break;
        }
        default: {
          CurrentWord() += static_cast<char>(c);
        }
      }
    }
  }
  return c;
}

inline ConsoleBuffer::ConsoleBuffer() {
  lines.emplace_back();
  // start a new run of chars with default formatting
  CurrentLine().sequences.emplace_back(sf::Color::White, "");
}

}  // namespace sfe