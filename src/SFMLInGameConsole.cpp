#include "SFMLInGameConsole.hpp"

#include <SFML/Graphics/RenderTarget.hpp>

namespace sfe {

SFMLInGameConsole::SFMLInGameConsole(sf::Font font)
    : console_(kCommandBufferSize),
      console_stream_(&console_buffer_),
      font_(std::move(font)) {
  AddStream(console_stream_);

  console_.bindMemberCommand("clear", *this, &SFMLInGameConsole::clear,
                             "Clear the console");
  console_.bindCVar("consoleTextScale", font_scale_);

  console_.style = Virtuoso::QuakeStyleConsole::ConsoleStylingColor();
}

sf::Font* SFMLInGameConsole::Font() { return &font_; }

void SFMLInGameConsole::show(bool v) { shown_ = v; }

bool SFMLInGameConsole::visible() const { return shown_; }

void SFMLInGameConsole::UpdateDrawnText() {
  input_line_.clear();
  input_line_.setFont(font_);
  input_line_.setScale({font_scale_, font_scale_});
  input_line_ << "> " << buffer_text_.substr(0, cursor_pos_) << "_"
              << buffer_text_.substr(cursor_pos_);

  const float input_line_height = input_line_.getGlobalBounds().height;
  const float console_height = background_rect_.getSize().y;
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
      last_visible_idx = i;
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
  const int end = last_visible_idx.value_or(last_line_idx);
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
        console_.commandExecute(buffer_text_, (*this));
        buffer_text_.clear();
        history_pos_ = -1;
        cursor_pos_ = 0;
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

void SFMLInGameConsole::Render(sf::RenderTarget* window) {
  if (!shown_) {
    return;
  }

  background_rect_.setFillColor(sf::Color(0u, 0u, 0u, 140u));
  background_rect_.setSize(sf::Vector2f(
      window->getSize().x, window->getSize().y * kConsoleHeightPart));

  UpdateDrawnText();

  window->draw(background_rect_);
  window->draw(output_text_);
  window->draw(input_line_);
}

void SFMLInGameConsole::ScrollCallback(const sf::Event& e) {
  const float input_line_height = input_line_.getGlobalBounds().height;
  const float overflow_height = std::max(
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

void SFMLInGameConsole::HistoryCallback(const sf::Event& e) {
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

void SFMLInGameConsole::TextCompletionCallback() {
  // Locate beginning of current word
  size_t word_start_pos = cursor_pos_;
  while (word_start_pos > 0) {
    const char c = buffer_text_[word_start_pos - 1];
    if (c == ' ' || c == '\t' || c == ',' || c == ';') break;
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

    // List matches
    (*this) << "Possible matches:\n";
    for (size_t i = 0; i < candidates.size(); i++) {
      (*this) << "- " << candidates[i] << '\n';
    }
  }
}

}  // namespace sfe