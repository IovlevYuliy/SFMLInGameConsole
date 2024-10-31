#include "SFMLInGameConsole.hpp"

#include <SFML/Graphics/RenderTarget.hpp>
#include <cmath>

namespace sfe {

namespace {

// Helper function to map ANSI color codes to SFML colors.
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

// Extracts and returns the first word from a given string.
inline std::string GetFirstWord(const std::string& str) {
  std::istringstream istream(str);
  std::string firstWord;
  istream >> firstWord;
  return firstWord;
}

}  // namespace

// Constructor for initializing the console with a font.
SFMLInGameConsole::SFMLInGameConsole(sf::Font font, size_t command_history_size)
    : Virtuoso::QuakeStyleConsole(command_history_size),
      console_stream_(&console_buffer_),
      font_(std::move(font)) {
  AddStream(console_stream_);

  bindMemberCommand("clear", *this, &SFMLInGameConsole::clear,
                    "Clear the console");
  // Overrides default styling for different message types.
  style = {{"\u001b[31m[error]: ", std::string(TEXT_COLOR_RESET)},
           {"\u001b[33m[warning]: ", std::string(TEXT_COLOR_RESET)},
           {"\u001b[37m> ", std::string(TEXT_COLOR_RESET)}};
}

void SFMLInGameConsole::SetBackgroundColor(sf::Color color) {
  background_color_ = std::move(color);
}

void SFMLInGameConsole::SetFontScale(float scale) { font_scale_ = scale; }

void SFMLInGameConsole::SetMaxInputLineSymbols(size_t count) {
  max_input_line_symbols_ = count;
}

void SFMLInGameConsole::SetPosition(sf::Vector2f pos) {
  position_ = std::move(pos);
}

void SFMLInGameConsole::SetTextLeftOffset(float offset_part) {
  text_left_offset_part_ = std::clamp(offset_part, 0.F, 1.F);
}

void SFMLInGameConsole::SetConsoleHeightPart(float height_part) {
  console_height_part_ = height_part;
}

void SFMLInGameConsole::SetCommandKeywords(const std::string& cmd_name,
                                           std::vector<std::string> keywords) {
  cmd_keywords_.insert({cmd_name, std::move(keywords)});
}

void SFMLInGameConsole::clear() {
  console_buffer_.clear();
  scroll_lines_offset_ = 0;
}

sf::Font* SFMLInGameConsole::Font() { return &font_; }

void SFMLInGameConsole::show(bool v) { shown_ = v; }

bool SFMLInGameConsole::visible() const { return shown_; }

/// Updates the rendered text based on the console's internal state and buffer.
void SFMLInGameConsole::UpdateDrawnText() {
  // Clear and set up input line with the cursor at the appropriate position.
  input_line_.clear();
  output_text_.clear();

  input_line_.setFont(font_);
  input_line_.setScale({font_scale_, font_scale_});
  input_line_ << "> " << buffer_text_.substr(0, cursor_pos_) << "_"
              << buffer_text_.substr(cursor_pos_);

  const float console_height = background_rect_.getSize().y;
  const float left_offset =
      background_rect_.getSize().x * text_left_offset_part_;
  const float line_height =
      font_scale_ * font_.getLineSpacing(input_line_.getCharacterSize());
  const int visible_lines_count = std::floor(console_height / line_height) - 1;

  // Position input line at the bottom of the console.
  input_line_.setPosition({left_offset, console_height - line_height});

  // Get range of currently visible lines.
  const int end = console_buffer_.size() - scroll_lines_offset_;
  const int begin = std::max(end - visible_lines_count, 0);
  const auto& lines = console_buffer_.GetLines();
  for (int i = begin; i < end; ++i) {
    for (const auto& seq : lines[i].sequences) {
      if (!seq.text.empty()) {
        output_text_ << GetAnsiTextColor(seq.color_code) << seq.text;
      }
    }
    if (i + 1 < end) {
      output_text_ << "\n";
    }
  }

  output_text_.setFont(font_);
  output_text_.setPosition({left_offset, 0.F});
  output_text_.setScale({font_scale_, font_scale_});

  // Apply current console position.
  output_text_.move(position_);
  input_line_.move(position_);
}

// Processes user input events for interacting with the console.
void SFMLInGameConsole::HandleUIEvent(const sf::Event& e) {
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
        commandExecute(buffer_text_, (*this));
        buffer_text_.clear();
        history_pos_ = -1;
        cursor_pos_ = 0;
        scroll_lines_offset_ = 0;
        return;
      case sf::Keyboard::Tab:
        TextAutocompleteCallback();
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
    // ASCII characters only; skips backspace and delete
    if (e.text.unicode > 31 && e.text.unicode < 127 &&
        buffer_text_.size() < max_input_line_symbols_) {
      buffer_text_.insert(cursor_pos_, 1, static_cast<char>(e.text.unicode));
      ++cursor_pos_;
    }
  }
}

// Renders the console background, output text, and input line.
void SFMLInGameConsole::Render(sf::RenderTarget* window) {
  if (!shown_) {
    return;
  }

  background_rect_.setPosition(position_);
  background_rect_.setFillColor(background_color_);
  background_rect_.setSize(sf::Vector2f(
      window->getSize().x, window->getSize().y * console_height_part_));

  UpdateDrawnText();

  window->draw(background_rect_);
  window->draw(output_text_);
  window->draw(input_line_);
}

// Adjusts scroll position based on key events.
void SFMLInGameConsole::ScrollCallback(const sf::Event& e) {
  const float line_height =
      font_scale_ * font_.getLineSpacing(input_line_.getCharacterSize());
  const int visible_lines_count =
      std::floor(background_rect_.getSize().y / line_height) - 1;
  const int overflow_lines =
      std::max(console_buffer_.size() - visible_lines_count, 0);
  if (e.key.code == sf::Keyboard::Up) {
    scroll_lines_offset_ = std::min(scroll_lines_offset_ + 1, overflow_lines);
  } else if (e.key.code == sf::Keyboard::Down) {
    scroll_lines_offset_ = std::max(scroll_lines_offset_ - 1, 0);
  }
}

// Handles navigation of command history for past commands.
void SFMLInGameConsole::HistoryCallback(const sf::Event& e) {
  const int prev_history_pos = history_pos_;
  if (e.key.code == sf::Keyboard::Up) {
    if (history_pos_ == -1) {
      history_pos_ = static_cast<int>(historyBuffer().size()) - 1;
    } else if (history_pos_ > 0) {
      history_pos_--;
    }
  } else if (e.key.code == sf::Keyboard::Down) {
    if (history_pos_ != -1) {
      if (++history_pos_ >= static_cast<int>(historyBuffer().size())) {
        history_pos_ = -1;
      }
    }
  }

  if (prev_history_pos != history_pos_) {
    buffer_text_ = history_pos_ >= 0 ? historyBuffer()[history_pos_] : "";
    cursor_pos_ = buffer_text_.size();
  }
}

// Retrieves autocomplete suggestions based on the current input.
std::vector<std::string> SFMLInGameConsole::GetCandidatesForAutocomplete(
    const std::string& cur_word, bool is_first_word) const {
  // Build a list of candidates.
  std::vector<std::string> candidates;

  if (is_first_word) {
    // Autocomplete for command names.
    for (auto it = getCommandTable().begin(); it != getCommandTable().end();
         it++) {
      if (it->first.starts_with(cur_word)) {
        candidates.emplace_back(it->first);
      }
    }
  }

  if (!is_first_word) {
    // Autocomplete variables.
    for (auto it = getCVarReadTable().begin(); it != getCVarReadTable().end();
         it++) {
      if (it->first.starts_with(cur_word)) {
        candidates.emplace_back(it->first);
      }
    }
    // Autocomplete keywords for a particular command.
    const std::string cmd_name = GetFirstWord(buffer_text_);
    if (cmd_keywords_.contains(cmd_name)) {
      for (auto it = cmd_keywords_.at(cmd_name).begin();
           it != cmd_keywords_.at(cmd_name).end(); it++) {
        if (it->starts_with(cur_word)) {
          candidates.emplace_back(*it);
        }
      }
    }
  }

  std::sort(candidates.begin(), candidates.end());
  return candidates;
}

// Handles autocompletion logic based on input and available suggestions.
void SFMLInGameConsole::TextAutocompleteCallback() {
  // Locate beginning of current word.
  size_t word_start_pos = cursor_pos_;
  while (word_start_pos > 0) {
    const char c = buffer_text_[word_start_pos - 1];
    if (std::isspace(static_cast<unsigned char>(c))) {
      break;
    }
    word_start_pos--;
  }

  const bool is_first_word = std::all_of(
      buffer_text_.begin(), buffer_text_.begin() + word_start_pos - 1,
      [](const char c) { return std::isspace(static_cast<unsigned char>(c)); });

  const std::string cur_word =
      buffer_text_.substr(word_start_pos, cursor_pos_ - word_start_pos);

  const std::vector<std::string> candidates =
      GetCandidatesForAutocomplete(cur_word, is_first_word);

  // No matches at all.
  if (candidates.empty()) {
    return;
  }

  if (candidates.size() == 1) {  // Single match completion.
    buffer_text_.resize(word_start_pos + candidates[0].size());
    buffer_text_.replace(word_start_pos, candidates[0].size(), candidates[0]);
    cursor_pos_ = buffer_text_.size();
  } else {
    // Multiple matches - partial completion.
    // So inputing "C"+Tab will complete to "CL" then display "CLEAR" and
    // "CLASSIFY" as matches.
    int match_len = cur_word.size();
    for (;;) {
      int c = 0;
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

    int max_length = 0;
    for (const auto& word : candidates) {
      max_length = std::max(max_length, static_cast<int>(word.length()));
    }
    constexpr static size_t kMatchesInLine = 5;
    for (size_t i = 0; i < candidates.size(); i++) {
      if (i && !(i % kMatchesInLine)) {
        (*this) << std::endl;
      }
      (*this) << std::setw(max_length + 2) << std::left << candidates[i];
    }
    (*this) << std::endl;
  }
}

}  // namespace sfe