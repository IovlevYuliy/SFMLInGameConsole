#pragma once

#include <algorithm>
#include <cstring>
#include <iostream>
#include <sstream>

namespace sfe {

// Enumeration of ANSI color codes for console output.
// These codes are used to format text with specific colors in the terminal.
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

// Console buffer for the UI widget. This buffer splits a stream of text
// into lines (`Line`), each of which contains one or more formatted sequences
// (`TextSequence`). Formatting is currently achieved via ANSI color codes.
// Additional input transformations (e.g., syntax highlighting) can be applied
// to the input before passing it to this stream.
class ConsoleBuffer : public std::streambuf {
 public:
  ConsoleBuffer();

  // Represents a sequence of text with an associated ANSI color code.
  struct TextSequence {
    // Color code for this text sequence.
    AnsiColorCode color_code = AnsiColorCode::ANSI_WHITE;
    // Text content of the sequence.
    std::string text = "";
  };

  // Represents a line of text, potentially containing multiple `TextSequence`
  // objects, each with separate color formatting.
  struct Line {
    std::vector<TextSequence> sequences;

    inline TextSequence& CurSequence() {
      return sequences[sequences.size() - 1];
    }
    inline const TextSequence& CurSequence() const {
      return sequences[sequences.size() - 1];
    }

    // Checks if the line contains only empty text sequences.
    inline bool IsEmpty() const {
      return std::all_of(sequences.begin(), sequences.end(),
                         [](const auto& seq) { return seq.text.empty(); });
    }
  };

  // Clears the entire console buffer, resetting it to an initial empty state.
  void clear();

  // Returns all lines in the buffer.
  inline const std::vector<Line>& GetLines() const { return lines; }

 protected:
  // Streambuf override method for handling overflow.
  // This method is responsible for processing each character added to the
  // stream, handling both regular text and ANSI color codes.
  int overflow(int c) override;

  // Processes an ANSI code from the input stream, updating the current
  // formatting state.
  /// @param code The ANSI code to process.
  inline void ProcessANSICode(int code);

  // Returns the current line being processed.
  inline Line& CurrentLine() { return lines[lines.size() - 1]; }
  // Returns the current word (text) sequence in the current line.
  inline std::string& CurrentWord() { return CurrentLine().CurSequence().text; }

  // Last used ANSI color code.
  AnsiColorCode cur_color_code = AnsiColorCode::ANSI_WHITE;

  // Tracks if currently parsing an ANSI color code.
  bool parsing_ansi_code = false;
  // Tracks if waiting for a digit in an ANSI sequence.
  bool listening_digits = false;
  // Accumulates digits for ANSI code parsing.
  std::stringstream num_parse_stream;
  // All lines in the console buffer.
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

// Processes a given ANSI code, updating the current color code based on the
// input.
/// @param code The ANSI color code to interpret and apply.
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

// Handles characters pushed to the stream, managing text and ANSI sequences.
/// @param c The character to process.
/// @return The character processed, or `EOF` if at the end of the file.
inline int ConsoleBuffer::overflow(int c) {
  if (c != EOF) {
    if (parsing_ansi_code) {
      bool error = false;

      if (std::isdigit(static_cast<char>(c)) && listening_digits) {
        num_parse_stream << static_cast<char>(c);
      } else {
        switch (c) {
          case 'm': {  // End of ANSI code; apply color formatting to new sequence.
            parsing_ansi_code = false;
            int x;
            if (num_parse_stream >> x) {
              ProcessANSICode(x);
            }
            num_parse_stream.clear();
            CurrentLine().sequences.emplace_back(cur_color_code, "");
            break;
          }
          case '[':  // Start of ANSI sequence.
            listening_digits = true;
            num_parse_stream.clear();
            break;
          case ';': { // Multiple ANSI codes; process current and prepare for next.
            int x;
            num_parse_stream >> x;
            num_parse_stream.clear();
            ProcessANSICode(x);
            break;
          }
          default: // Invalid character in ANSI code.
            error = true;
            break;
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
        case '\u001b': // Start of an ANSI escape sequence.
          parsing_ansi_code = true;
          num_parse_stream.clear();
          break;
        case '\n': // End of line; add a new line.
          lines.emplace_back();
          CurrentLine().sequences.emplace_back(cur_color_code, "");
          break;
        default: // Regular character, added to the current text sequence.
          CurrentWord() += static_cast<char>(c);
      }
    }
  }
  return c;
}

/// Initializes a ConsoleBuffer with an empty line and default color formatting.
inline ConsoleBuffer::ConsoleBuffer() {
  lines.emplace_back();
  // Start a new run of characters with default formatting.
  CurrentLine().sequences.emplace_back(AnsiColorCode::ANSI_WHITE, "");
}

}  // namespace sfe