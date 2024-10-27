#pragma once

//////////////////////////////////////////////////////////////////////////
// Headers
//////////////////////////////////////////////////////////////////////////
#include <vector>

#include <SFML/Graphics/Transformable.hpp>
#include <SFML/Graphics/Drawable.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Text.hpp>

#include <SFML/System/Vector2.hpp>

namespace sf
{
class Font;
class String;
template <class T> class Rect;
typedef Rect<float> FloatRect;
}

namespace sfe
{

class RichText : public sf::Drawable, public sf::Transformable
{
public:
    //////////////////////////////////////////////////////////////////////////
    // Nested class that represents a single line
    //////////////////////////////////////////////////////////////////////////
    class Line : public sf::Transformable, public sf::Drawable
    {
    public:
        //////////////////////////////////////////////////////////////////////
        // Set character size
        //////////////////////////////////////////////////////////////////////
        void setCharacterSize(unsigned int size);

        //////////////////////////////////////////////////////////////////////
        // Set font
        //////////////////////////////////////////////////////////////////////
        void setFont(const sf::Font &font);

        //////////////////////////////////////////////////////////////////////
        // Get texts
        //////////////////////////////////////////////////////////////////////
        const std::vector<sf::Text> &getTexts() const;

        //////////////////////////////////////////////////////////////////////
        // Append text
        //////////////////////////////////////////////////////////////////////
        void appendText(sf::Text text);

        //////////////////////////////////////////////////////////////////////
        // Get local bounds
        //////////////////////////////////////////////////////////////////////
        sf::FloatRect getLocalBounds() const;

        //////////////////////////////////////////////////////////////////////
        // Get global bounds
        //////////////////////////////////////////////////////////////////////
        sf::FloatRect getGlobalBounds() const;

    protected:
        //////////////////////////////////////////////////////////////////////
        // Draw
        //////////////////////////////////////////////////////////////////////
        void draw(sf::RenderTarget &target, const sf::RenderStates& states) const override;

    private:
        //////////////////////////////////////////////////////////////////////
        // Update geometry
        //////////////////////////////////////////////////////////////////////
        void updateGeometry() const;

        //////////////////////////////////////////////////////////////////////
        // Update geometry for a given text
        //////////////////////////////////////////////////////////////////////
        void updateTextAndGeometry(sf::Text &text) const;

        //////////////////////////////////////////////////////////////////////
        // Member data
        //////////////////////////////////////////////////////////////////////
        mutable std::vector<sf::Text> m_texts; ///< List of texts
        mutable sf::FloatRect m_bounds;        ///< Local bounds
    };

    //////////////////////////////////////////////////////////////////////////
    // Constructor
    //////////////////////////////////////////////////////////////////////////
    RichText();

    //////////////////////////////////////////////////////////////////////////
    // Constructor
    //////////////////////////////////////////////////////////////////////////
    RichText(const sf::Font &font);

    //////////////////////////////////////////////////////////////////////////
    // Operators
    //////////////////////////////////////////////////////////////////////////
    RichText & operator << (const sf::Color &color);
    RichText & operator << (sf::Text::Style style);
    RichText & operator << (const sf::String &string);

    //////////////////////////////////////////////////////////////////////////
    // Set character size
    //////////////////////////////////////////////////////////////////////////
    void setCharacterSize(unsigned int size);

    //////////////////////////////////////////////////////////////////////////
    // Set font
    //////////////////////////////////////////////////////////////////////////
    void setFont(const sf::Font &font);

    //////////////////////////////////////////////////////////////////////////
    // Clear
    //////////////////////////////////////////////////////////////////////////
    void clear();

    //////////////////////////////////////////////////////////////////////////
    // Get text list
    //////////////////////////////////////////////////////////////////////////
    const std::vector<Line> &getLines() const;

    //////////////////////////////////////////////////////////////////////////
    // Get character size
    //////////////////////////////////////////////////////////////////////////
    unsigned int getCharacterSize() const;

    //////////////////////////////////////////////////////////////////////////
    // Get font
    //////////////////////////////////////////////////////////////////////////
    const sf::Font *getFont() const;

    //////////////////////////////////////////////////////////////////////////
    // Get local bounds
    //////////////////////////////////////////////////////////////////////////
    sf::FloatRect getLocalBounds() const;

    //////////////////////////////////////////////////////////////////////////
    // Get global bounds
    //////////////////////////////////////////////////////////////////////////
    sf::FloatRect getGlobalBounds() const;

protected:
    //////////////////////////////////////////////////////////////////////////
    // Render
    //////////////////////////////////////////////////////////////////////////
    void draw(sf::RenderTarget &target, const sf::RenderStates& states) const override;

private:
    //////////////////////////////////////////////////////////////////////////
    // Delegate constructor
    //////////////////////////////////////////////////////////////////////////
    RichText(const sf::Font *font);

    //////////////////////////////////////////////////////////////////////////
    // Creates a sf::Text instance using the current styles
    //////////////////////////////////////////////////////////////////////////
    sf::Text createText(const sf::String &string) const;

    //////////////////////////////////////////////////////////////////////////
    // Update geometry
    //////////////////////////////////////////////////////////////////////////
    void updateGeometry() const;

    //////////////////////////////////////////////////////////////////////////
    // Member data
    //////////////////////////////////////////////////////////////////////////
    mutable std::vector<Line> m_lines; ///< List of lines
    const sf::Font *m_font;            ///< Font
    unsigned int m_characterSize;      ///< Character size
    mutable sf::FloatRect m_bounds;    ///< Local bounds
    sf::Color m_currentColor;          ///< Last used color
    sf::Text::Style m_currentStyle;    ///< Last style used
};

}