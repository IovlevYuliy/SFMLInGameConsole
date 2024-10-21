///
///  IMGUIQuakeConsole.h - Single Header Quake Console Library
///  Created by VirtuosoChris on 8/19/20.
///
/// This file is a single header frontend gui widget corresponding to (and depending on) QuakeStyleConsole.h in this repository
///
/// This is  written against the Dear IMGUI library (https://github.com/ocornut/imgui)
///
/// For example programs see the demos folder in this repo.
///
/// The easiest way to use this file is to include it and then create an IMGUIQuakeConsole in an application that supports IMGUI rendering,
/// then call IMGUIQuakeConsole::Render during your application update loop.  CTRL-F IMGUIQuakeConsole class definition and look at the sample code in the demos filter to get started.
///
/// This file contains base classes, out of which the IMGUIQuakeConsole is composed, and you can experiment with as well.
///
/// The text area of the console is an IMGUIOstream, which inherits from std::ostream and you can do all the things that implies.
/// The IMGUIOstream's streambuf parses ANSI color codes so you can add color formatting that way.
///
/// IMGUIInputLine handles some IMGUI callbacks, and pushes user input into a stringstream on enter.  You can get the stream directly or get a line.
///
/// Both the Input Line and the Ostream have render methods that draw them in whatever surrounding IMGUI context the caller has,
/// and also helper methods to draw them in their own windows.
///
/// a MultiStream is an ostream that forwards input to multiple other ostreams.
/// The console widget is a 'multistream' so you can mirror console output to a file or cout or whatever other streams you need.

/*
 This software is available under 2 licenses -- choose whichever you prefer.
 Either MIT or public domain.  Full license text at the bottom of the file.
*/

//  Some code in this file Forked from Example Console code in IMGUI samples as reference
//  https://github.com/ocornut/imgui/blob/master/imgui_demo.cpp

#ifndef ConsoleWidget_h
#define ConsoleWidget_h

#include <unordered_set>
#include <algorithm>
#include <cstring>

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Font.hpp>

#include "QuakeStyleConsole.h"

namespace Virtuoso
{

inline constexpr std::string_view TEXT_COLOR_RESET = "\u001b[0m";
inline constexpr std::string_view TEXT_COLOR_BLACK = "\u001b[30m";
inline constexpr std::string_view TEXT_COLOR_RED = "\u001b[31m";
inline constexpr std::string_view TEXT_COLOR_GREEN = "\u001b[32m";
inline constexpr std::string_view TEXT_COLOR_YELLOW = "\u001b[33m";
inline constexpr std::string_view TEXT_COLOR_BLUE = "\u001b[34m";
inline constexpr std::string_view TEXT_COLOR_MAGENTA = "\u001b[35m";
inline constexpr std::string_view TEXT_COLOR_CYAN = "\u001b[36m";
inline constexpr std::string_view TEXT_COLOR_WHITE = "\u001b[37m";
inline constexpr std::string_view TEXT_COLOR_BLACK_BRIGHT = "\u001b[30;1m";
inline constexpr std::string_view TEXT_COLOR_RED_BRIGHT = "\u001b[31;1m";
inline constexpr std::string_view TEXT_COLOR_GREEN_BRIGHT = "\u001b[32;1m";
inline constexpr std::string_view TEXT_COLOR_YELLOW_BRIGHT = "\u001b[33;1m";
inline constexpr std::string_view TEXT_COLOR_BLUE_BRIGHT = "\u001b[34;1m";
inline constexpr std::string_view TEXT_COLOR_MAGENTA_BRIGHT = "\u001b[35;1m";
inline constexpr std::string_view TEXT_COLOR_CYAN_BRIGHT = "\u001b[36;1m";
inline constexpr std::string_view TEXT_COLOR_WHITE_BRIGHT = "\u001b[37;1m";

enum AnsiColorCode
{
    ANSI_RESET = 0,
    ANSI_BRIGHT_TEXT = 1,

    ANSI_BLACK = 30,
    ANSI_RED = 31,
    ANSI_GREEN = 32,
    ANSI_YELLOW = 33,
    ANSI_BLUE = 34,
    ANSI_MAGENTA = 35,
    ANSI_CYAN = 36,
    ANSI_WHITE = 37,
};

/// Stream Buffer for the IMGUI Console Terminal.  Breaks text stream into Lines, which are an array of formatted text sequences
/// Formatting is presently handled via ANSI Color Codes.  Some other input transformation can be applied to the input before it hits this stream
/// eg. to do syntax highlighting, etc.
class ConsoleBuf : public std::streambuf
{
  public:

    struct FormattingParams
    {
        sf::Color textColor = ImVec4(1.0, 1.0, 1.0, 1.0);
    };

    struct TextSequence
    {
        FormattingParams style;
        std::string text = "";
    };

    struct Line
    {
        std::vector<TextSequence> sequences;

        inline TextSequence &curSequence() { return sequences[sequences.size() - 1]; }
        inline const TextSequence &curSequence() const { return sequences[sequences.size() - 1]; }
    };

    void clear();

    inline void applyDefaultStyle(){currentStyle = defaultStyle;}
    inline const Line &currentLine() const { return lines[lines.size() - 1]; }
    inline const std::string &curStr() const { return currentLine().curSequence().text; }

    ConsoleBuf();

    inline const std::vector<Line>& getLines() const { return lines; }

    FormattingParams defaultStyle; ///< can change default text color and background

  protected:
    /// change formatting state based on an integer code in the ansi-code input stream.  called by the streambuf methods
    void processANSICode(int code);

    // -- streambuf overloads --
    int overflow(int c);

    FormattingParams currentStyle; ///< // current formatting

    bool brightText = false;                       ///< saw ansi code for bright-mode text
    AnsiColorCode textCode = ANSI_RESET;           ///< ANSI color code we last saw for text

    std::vector<Line> lines; ///< All output lines

    bool parsingANSICode = false; ///< ANSI color code parser state variable
    bool listeningDigits = false; ///< ANSI color code parser state variable - listening for next digit

    std::stringstream numParse; ///< ANSI color code parser state variable - digit accumulator

    inline Line &currentLine() { return lines[lines.size() - 1]; }
    inline std::string &curStr() { return currentLine().curSequence().text; }
};

/// streambuffer implementation for MultiStream
class MultiStreamBuf : public std::streambuf
{
  public:
    std::unordered_set<std::ostream *> streams;

    MultiStreamBuf() {}

    int overflow(int in);

    std::streamsize xsputn(const char *s, std::streamsize n);
};

/// An ostream that is actually a container of ostream pointers, that pipes output to every ostream in the container
class MultiStream : public std::ostream
{
    MultiStreamBuf buf;

  public:
    MultiStream() : std::ostream(&buf) {}

    void addStream(std::ostream &str) { buf.streams.insert(&str); }
};

/// A user input line that supports callbacks and pushes user input to a stream on enter
struct IMGUIInputLine
{
  private:
    std::string InputBuf;       ///< buffer user is typing into currently
    std::stringstream stream; ///< stream that input lines accumulate into on enter presses

  public:
    IMGUIInputLine();

    std::istream &getStream(); ///< returns the istream that user input accumulates into

    std::string getInput(); ///< Pulls a single line from the input stream and returns it

    /// Renders the contorl in whatever the surrounding IMGUI context is.  Returns true if new user input is available.
    bool render();
};

/// GUI Ostream pane with ANSI Color Code Support
class IMGUIOstream : public std::ostream
{
  public:
    ConsoleBuf strb;        ///< custom streambuf

    bool autoScrollEnabled = true;
    bool shouldScrollToBottom = false;

    inline void Clear() { strb.clear(); } ///< clear the output pane

    inline IMGUIOstream() : std::ostream(&strb) {}

    /// renders the control in a new popup window.
    void renderInWindow(bool &p_open, const char *title = "");

    /// Renders the control in whatever the surrounding IMGUI context is.
    void render();

    inline void applyDefaultStyle(){strb.applyDefaultStyle();}
    inline ConsoleBuf::FormattingParams& defaultStyle(){return strb.defaultStyle;}
};

/// Quake style console : IMGUI Widget
/// The widget IS-A MultiStream, so you can call .addStream() to add additional streams to mirror the output - like a file or cout
/// A MultiStream IS-A ostream, so you can write to it with << and pass it to ostream functions
/// You can also get its streambuf and pass it to another ostream, such as cout so that those ostreams write to the console.  eg.  cout.rdbuf(console.rdbuf())
class IMGUIQuakeConsole : public MultiStream
{
  public:
    Virtuoso::QuakeStyleConsole con; ///< implementation of the quake style console
    IMGUIOstream os;                 ///< IMGUI ostream pane
    IMGUIInputLine is;               ///< IMGUI input line
    std::size_t prevLineCount = 0;   ///< previous line count for os; used to autoscroll when os gets a new line.

    sf::Font font;

    int HistoryPos = -1; ///< index into the console history buffer, for when we press up/down arrow to scroll previous commands

    float fontScale = 1.2f; ///< text scale for the console widget window

    void ClearLog(); ///< Clear the ostream

    void render(const char *title, bool& p_open); ///< Renders an IMGUI window implementation of the console

    IMGUIQuakeConsole();

  private:
    void historyCallback();
    void textCompletionCallback();
};

// -------------------------------------------
// ------------ANSI COLOR HELPERS-------------
// -------------------------------------------

sf::Color getAnsiTextColor(AnsiColorCode code);
sf::Color getAnsiTextColorBright(AnsiColorCode code);

// -------------------------------------------
// --------------Portable String Helpers------
// -------------------------------------------

inline static int Strnicmp(const char *s1, const char *s2, int n)
{
    int d = 0;
    while (n > 0 && (d = toupper(*s2) - toupper(*s1)) == 0 && *s1)
    {
        s1++;
        s2++;
        n--;
    }
    return d;
}

inline static void Strtrim(char *s)
{
    char *str_end = s + strlen(s);
    while (str_end > s && str_end[-1] == ' ')
        str_end--;
    *str_end = 0;
}

// -------------------------------------------
// ----- IMGUIOstream Implementation ------ //
// -------------------------------------------

inline void IMGUIOstream::renderInWindow(bool &p_open, const char *title)
{
    ImGui::SetNextWindowSize(ImVec2(520, 600), ImGuiCond_FirstUseEver);

    if (!ImGui::Begin(title, &p_open))
    {
        ImGui::End();
        return;
    }

    render();

    ImGui::End();
}

inline void IMGUIOstream::render()
{
    for (const ConsoleBuf::Line& line : strb.getLines())
    {
        for (const ConsoleBuf::TextSequence &seq : line.sequences)
        {
            ImGui::TextColored(seq.style.textColor, seq.text.c_str());
            ImGui::SameLine();
        }

        ImGui::NewLine();
    }

    if ((autoScrollEnabled && shouldScrollToBottom) || (autoScrollEnabled && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()))
        ImGui::SetScrollHereY(1.0f);
    shouldScrollToBottom = false;
}

// -------------------------------------------
// --- IMGUIQuakeConsole Implementation --- //
// -------------------------------------------

inline IMGUIQuakeConsole::IMGUIQuakeConsole()
{
    addStream(os);

    con.bindMemberCommand("consoleClear", *this, &IMGUIQuakeConsole::ClearLog, "Clear the console");
    con.bindCVar("consoleTextScale", fontScale);

    con.style = QuakeStyleConsole::ConsoleStylingColor();
}

inline void IMGUIQuakeConsole::ClearLog() { os.Clear(); }

inline void IMGUIQuakeConsole::render(const char *title, bool& p_open)
{
    if (!p_open) return;

    if (font)
    {
        ImGui::PushFont(font);
    }

    ImGui::SetNextWindowSize(ImVec2(520, 600), ImGuiCond_FirstUseEver);

    if (!ImGui::Begin(title, &p_open))
    {
        ImGui::End();
        return;
    }

    ImGui::SetWindowFontScale(fontScale);

    // As a specific feature guaranteed by the library, after calling Begin() the last Item represent the title bar.
    // So e.g. IsItemHovered() will return true when hovering the title bar.
    // Here we create a context menu only available from the title bar.
    if (ImGui::BeginPopupContextItem())
    {
        if (ImGui::MenuItem("Close Console"))
            p_open = false;
        ImGui::EndPopup();
    }

    ImGui::TextWrapped("Enter 'help' for help, press TAB to use text completion.");

    // TODO: display items starting from the bottom

    if (ImGui::SmallButton("Clear"))
    {
        ClearLog();
    }
    ImGui::SameLine();

    bool copy_to_clipboard = ImGui::SmallButton("Copy");

    ImGui::Separator();

    // Options, Filter
    if (ImGui::Button("Options"))
        ImGui::OpenPopup("Options");
    ImGui::SameLine();
    os.filter.Draw("Filter (\"incl,-excl\") (\"error\")", 180);
    ImGui::Separator();

    // Reserve enough left-over height for 1 separator + 1 input text
    const float footer_height_to_reserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
    ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footer_height_to_reserve), false, ImGuiWindowFlags_HorizontalScrollbar);
    if (ImGui::BeginPopupContextWindow())
    {
        if (ImGui::Selectable("Clear"))
            ClearLog();
        ImGui::EndPopup();
    }

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1)); // Tighten spacing
    if (copy_to_clipboard)
        ImGui::LogToClipboard();

    os.render();

    if (prevLineCount < os.strb.getLines().size())
    {
        os.shouldScrollToBottom = true;
    }
    prevLineCount = os.strb.getLines().size();

    if (copy_to_clipboard)
        ImGui::LogFinish();

    ImGui::PopStyleVar();
    ImGui::EndChild();
    ImGui::Separator();

    if (is.render())
    {
        HistoryPos = -1;

        con.commandExecute(is.getStream(), (*this));

        // On command input, we scroll to bottom even if AutoScroll==false
        os.shouldScrollToBottom = true;
    }

    if (font)
    {
        ImGui::PopFont();
    }

    ImGui::End();
}

inline void IMGUIQuakeConsole::historyCallback()
{
    // // Example of HISTORY
    // const int prev_history_pos = HistoryPos;
    // if (data->EventKey == ImGuiKey_UpArrow)
    // {
    //     if (HistoryPos == -1)
    //         HistoryPos = con.historyBuffer().size() - 1;
    //     else if (HistoryPos > 0)
    //         HistoryPos--;
    // }
    // else if (data->EventKey == ImGuiKey_DownArrow)
    // {
    //     if (HistoryPos != -1)
    //         if (++HistoryPos >= con.historyBuffer().size())
    //             HistoryPos = -1;
    // }

    // // A better implementation would preserve the data on the current input line along with cursor position.
    // if (prev_history_pos != HistoryPos)
    // {
    //     const char *history_str = (HistoryPos >= 0) ? con.historyBuffer()[HistoryPos].c_str() : "";
    //     data->DeleteChars(0, data->BufTextLen);
    //     data->InsertChars(0, history_str);
    // }
}

inline void IMGUIQuakeConsole::textCompletionCallback()
{
    // // Locate beginning of current word
    // const char *word_end = data->Buf + data->CursorPos;
    // const char *word_start = word_end;
    // while (word_start > data->Buf)
    // {
    //     const char c = word_start[-1];
    //     if (c == ' ' || c == '\t' || c == ',' || c == ';')
    //         break;
    //     word_start--;
    // }

    // // Build a list of candidates
    // std::vector<std::string> candidates;

    // // autocomplete commands...
    // for (auto it = con.getCommandTable().begin(); it != con.getCommandTable().end(); it++)
    // {
    //     if (Strnicmp(it->first.c_str(), word_start, (int)(word_end - word_start)) == 0)
    //     {
    //         candidates.emplace_back(it->first);
    //     }
    // }

    // // ... and autcomplete variables
    // for (auto it = con.getCVarReadTable().begin(); it != con.getCVarReadTable().end(); it++)
    // {
    //     if (Strnicmp(it->first.c_str(), word_start, (int)(word_end - word_start)) == 0)
    //     {
    //         candidates.emplace_back(it->first);
    //     }
    // }

    // if (candidates.empty())
    // {
    //     // No match
    //     //AddLog("No match for %.*s, , word_start);
    //     (*this) << "No match for ";
    //     (*this) << (int)(word_end - word_start);
    //     (*this) << ' ' << word_start;
    //     (*this) << "!\n";
    // }
    // else if (candidates.size() == 1)
    // {
    //     // Single match. Delete the beginning of the word and replace it entirely so we've got nice casing.
    //     data->DeleteChars((int)(word_start - data->Buf), (int)(word_end - word_start));
    //     data->InsertChars(data->CursorPos, candidates[0].c_str());
    //     data->InsertChars(data->CursorPos, " ");
    // }
    // else
    // {
    //     // Multiple matches. Complete as much as we can..
    //     // So inputing "C"+Tab will complete to "CL" then display "CLEAR" and "CLASSIFY" as matches.
    //     int match_len = (int)(word_end - word_start);
    //     for (;;)
    //     {
    //         int c = 0;
    //         bool all_candidates_matches = true;
    //         for (size_t i = 0; i < candidates.size() && all_candidates_matches; i++)
    //             if (i == 0)
    //                 c = toupper(candidates[i][match_len]);
    //             else if (c == 0 || c != toupper(candidates[i][match_len]))
    //                 all_candidates_matches = false;
    //         if (!all_candidates_matches)
    //             break;
    //         match_len++;
    //     }

    //     if (match_len > 0)
    //     {
    //         data->DeleteChars((int)(word_start - data->Buf), (int)(word_end - word_start));
    //         data->InsertChars(data->CursorPos, candidates[0].c_str(), candidates[0].c_str() + match_len);
    //     }

    //     // List matches
    //     (*this) << "Possible matches:\n";
    //     for (size_t i = 0; i < candidates.size(); i++)
    //         (*this) << "- " << candidates[i] << '\n';
    //}
}

inline sf::Color getAnsiTextColor(AnsiColorCode code)
{
    switch (code)
    {
    case ANSI_RESET:
        return sf::Color::White;
    case ANSI_BLACK:
        return sf::Color::Black;
    case ANSI_RED:
        return sf::Color(190, 0, 0);
    case ANSI_GREEN:
        return sf::Color(0, 190, 0);
    case ANSI_YELLOW:
        return sf::Color(190, 190, 0);
    case ANSI_BLUE:
        return sf::Color(0, 0, 190);
    case ANSI_MAGENTA:
        return sf::Color(190, 0, 190);
    case ANSI_CYAN:
        return sf::Color(0, 190, 190);
    case ANSI_WHITE:
        return sf::Color(190, 190, 190);
    default:
        return sf::Color::Black;
    }
}

inline sf::Color getAnsiTextColorBright(AnsiColorCode code)
{
    switch (code)
    {
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

// --------------------------------------
// --- MultiStreamBuf implementation ---
// --------------------------------------

inline int MultiStreamBuf::overflow(int in)
{
    char c = in; ///\todo check for eof, etc?
    for (std::ostream *str : streams)
    {
        (*str) << c;
    }
    return 1;
}

inline std::streamsize MultiStreamBuf::xsputn(const char *s, std::streamsize n)
{
    std::streamsize ssz = 0;

    for (std::ostream *str : streams)
    {
        ssz = str->rdbuf()->sputn(s, n);
    }

    return ssz;
}

// --------------------------------------
// ---- ConsoleBuf implementation -------
// --------------------------------------

inline void ConsoleBuf::clear()
{
    // swap forces reallocation, unlike clear
    std::vector<Line> x;
    lines.swap(x);
    //lines.clear();

    lines.push_back(Line());
    currentLine().sequences.push_back(TextSequence()); // start a new run of chars with default formatting
}

inline void ConsoleBuf::processANSICode(int code)
{
    switch (code)
    {
    case ANSI_RESET:
        currentStyle = defaultStyle;
        break;
    case ANSI_BRIGHT_TEXT:
        brightText = true;
        if (textCode)
        {
            currentStyle.textColor = getAnsiTextColorBright(textCode);
        }
        break;
    case ANSI_BLACK:
    case ANSI_RED:
    case ANSI_GREEN:
    case ANSI_YELLOW:
    case ANSI_BLUE:
    case ANSI_MAGENTA:
    case ANSI_CYAN:
    case ANSI_WHITE:
        textCode = (AnsiColorCode)code;

        if (brightText)
        {
            currentStyle.textColor = getAnsiTextColorBright((AnsiColorCode)code);
        }
        else
        {
            currentStyle.textColor = getAnsiTextColor((AnsiColorCode)code);
        }
        break;
    default:
        std::cerr << "unknown ansi code " << code << " in output\n";
        return;
    }
}

inline int ConsoleBuf::overflow(int c)
{
    if (c != EOF)
    {
        if (parsingANSICode)
        {
            bool error = false;

            if (std::isdigit((char)c) && listeningDigits)
            {
                numParse << (char)c;
            }
            else
            {
                switch (c)
                {
                case 'm': // end of ansi code; apply color formatting to new sequence
                {
                    parsingANSICode = false;

                    int x;
                    if (numParse >> x)
                    {
                        processANSICode(x);
                    }

                    numParse.clear();

                    brightText = false;

                    currentLine().sequences.push_back({currentStyle, "" });

                    break;
                }
                case '[':
                {
                    listeningDigits = true;
                    numParse.clear();
                    break;
                }
                case ';':
                {
                    int x;
                    numParse >> x;

                    numParse.clear();

                    processANSICode(x);

                    break;
                }
                default:
                {
                    error = true;
                    break;
                }
                }

                if (error)
                {
                    numParse.clear();
                    listeningDigits = false;
                    parsingANSICode = false;

                    std::cerr << c;
                    //curStr() += (char)c;
                }
            }
        }
        else
        {
            switch (c)
            {
            case '\u001b':
            {
                parsingANSICode = true;
                numParse.clear();
                break;
            }
            case '\n':
            {
                //currentline add \n
                lines.push_back(Line());
                currentLine().sequences.push_back(TextSequence({currentStyle, "",}));
                break;
            }
            default:
            {
                //std::cerr <<c;
                curStr() += (char)c;
            }
            }
        }
    }
    return c;
}

inline ConsoleBuf::ConsoleBuf()
{
    lines.push_back(Line());
    currentLine().sequences.push_back(TextSequence()); // start a new run of chars with default formatting
}

// --------------------------------------
// ---  IMGUIInputLine Implementation ---
// --------------------------------------

inline IMGUIInputLine::IMGUIInputLine()
{
}

inline std::istream &IMGUIInputLine::getStream()
{
    return stream;
}

inline std::string IMGUIInputLine::getInput()
{
    std::string rval;
    std::getline(stream, rval);
    return rval;
}

inline bool IMGUIInputLine::render()
{
    bool rval = false;

    // Command-line
    bool reclaim_focus = false;

    if (ImGui::InputText("Input", &InputBuf, input_text_flags, &TextEditCallbackStub, (void*)(&textCallbacks)))
    {
        reclaim_focus = true;

        char* s = InputBuf.data();
        Strtrim(s);

        if (InputBuf.length() && strlen(s))
        {
            rval = true;

            auto pos1 = stream.tellp(); // save pos1
            stream << s;                // write
            stream << std::endl;
            stream.seekg(pos1);

            strcpy(s, "");
        }
    }

    // Auto-focus on window apparition
    ImGui::SetItemDefaultFocus();
    if (reclaim_focus)
        ImGui::SetKeyboardFocusHere(-1); // Auto focus previous widget

    return rval;
}

} // namespace Virtuoso
#endif /* ConsoleWidget_h */

/* ------------------------------------------------------------------------------
ALTERNATIVE A - MIT License
Copyright (c) 2020 Virtuoso Engine
Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
------------------------------------------------------------------------------
ALTERNATIVE B - Public Domain (www.unlicense.org)
This is free and unencumbered software released into the public domain.
Anyone is free to copy, modify, publish, use, compile, sell, or distribute this
software, either in source code form or as a compiled binary, for any purpose,
commercial or non-commercial, and by any means.
In jurisdictions that recognize copyright laws, the author or authors of this
software dedicate any and all copyright interest in the software to the public
domain. We make this dedication for the benefit of the public at large and to
the detriment of our heirs and successors. We intend this dedication to be an
overt act of relinquishment in perpetuity of all present and future rights to
this software under copyright law.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
