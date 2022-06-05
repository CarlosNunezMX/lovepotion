#include "objects/font/fontc.h"

#include "common/bidirectionalmap.h"
#include "utf8/utf8.h"

using namespace love::common;

love::Type Font::type("Font", &Object::type);

int Font::fontCount = 0;

Font::Font(Rasterizer* rasterizer, const SamplerState& state) :
    rasterizers({ rasterizer }),
    samplerState(state)
{
    fontCount++;
}

Font::~Font()
{
    --fontCount;
}

float Font::GetLineHeight() const
{
    return this->lineHeight;
}

void Font::SetLineHeight(float lineHeight)
{
    this->lineHeight = lineHeight;
}

const love::SamplerState& Font::GetSamplerState()
{
    return this->samplerState;
}

int Font::GetWidth(const std::string& string)
{
    if (string.size() == 0)
        return 0;

    int maxWidth = 0;

    try
    {
        utf8::iterator<std::string::const_iterator> i(string.begin(), string.begin(), string.end());
        utf8::iterator<std::string::const_iterator> end(string.end(), string.begin(), string.end());

        while (i != end)
        {
            uint32_t prevGlyph = 0;
            int width          = 0;

            for (; i != end && *i != '\n'; ++i)
            {
                uint32_t current = *i;

                if (current == '\r')
                    continue;

                width += this->GetWidth(prevGlyph, current);
                prevGlyph = current;
            }

            maxWidth = std::max(maxWidth, width);

            if (i != end)
                ++i;
        }
    }
    catch (utf8::exception& e)
    {
        throw love::Exception("UTF-8 decoding error: %s", e.what());
    }

    return maxWidth;
}

bool Font::HasGlyphs(const std::string& text) const
{
    if (text.size() == 0)
        return false;

    try
    {
        utf8::iterator<std::string::const_iterator> i(text.begin(), text.begin(), text.end());
        utf8::iterator<std::string::const_iterator> end(text.end(), text.begin(), text.end());

        while (i != end)
        {
            uint32_t codepoint = *i++;

            if (!this->HasGlyph(codepoint))
                return false;
        }
    }
    catch (utf8::exception& e)
    {
        throw love::Exception("UTF-8 decoding error: %s", e.what());
    }

    return true;
}

// clang-format off
constexpr auto alignModes = BidirectionalMap<>::Create(
    "left",    Font::AlignMode::ALIGN_LEFT,
    "right",   Font::AlignMode::ALIGN_RIGHT,
    "center",  Font::AlignMode::ALIGN_CENTER,
    "justify", Font::AlignMode::ALIGN_JUSTIFY
);

// clang-format on

bool Font::GetConstant(const char* in, AlignMode& out)
{
    return alignModes.Find(in, out);
}

bool Font::GetConstant(AlignMode in, const char*& out)
{
    return alignModes.ReverseFind(in, out);
}

std::vector<const char*> Font::GetConstants(AlignMode)
{
    return alignModes.GetNames();
}
