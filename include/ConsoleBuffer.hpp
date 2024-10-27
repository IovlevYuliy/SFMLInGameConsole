#pragma once

#include <algorithm>
#include <cstring>
#include <iostream>
#include <sstream>

namespace sfe {

enum AnsiColorCode {
  ANSI_RESET = 0,

  ANSI_BLACK = 30,
  ANSI_RED = 31,
  ANSI_GREEN = 32,
  ANSI_YELLOW = 33,
  ANSI_BLUE = 34,
  ANSI_MAGENTA = 35,
  ANSI_CYAN = 36,
  ANSI_WHITE = 37,
};

/// Stream Buffer for the IMGUI Console Terminal.  Breaks text stream into
/// Lines, which are an array of formatted text sequences Formatting is
/// presently handled via ANSI Color Codes.  Some other input transformation can
/// be applied to the input before it hits this stream eg. to do syntax
/// highlighting, etc.
class ConsoleBuffer : public std::streambuf {
 public:
  ConsoleBuffer();

  struct TextSequence {
    AnsiColorCode color_code = AnsiColorCode::ANSI_WHITE;
    std::string text = "";
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
  AnsiColorCode cur_color_code = AnsiColorCode::ANSI_WHITE;

  // ANSI color code parser state variable
  bool parsing_ansi_code = false;
  // ANSI color code parser is listening for next digit
  bool listening_digits = false;
  // ANSI color code parser accumulator
  std::stringstream num_parse_stream;
  // All output lines
  std::vector<Line> lines;
};

// --------------------------------------
// ---- ConsoleBuffer implementation -------
// --------------------------------------

inline void ConsoleBuffer::clear() {
  lines.clear();
  lines.emplace_back();
  CurrentLine().sequences.emplace_back(AnsiColorCode::ANSI_WHITE, "");
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
      cur_color_code = static_cast<AnsiColorCode>(code);
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
            CurrentLine().sequences.emplace_back(cur_color_code, "");
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
          listening_digits = false;
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
          CurrentLine().sequences.emplace_back(cur_color_code, "");
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
  CurrentLine().sequences.emplace_back(AnsiColorCode::ANSI_WHITE, "");
}

}  // namespace sfe