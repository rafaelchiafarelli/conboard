#include "panel_null.hpp"

#include <algorithm>
#include <fstream>

NullPanel::NullPanel(int width, int height, std::string dumpPath)
    : w_(width), h_(height), dumpPath_(std::move(dumpPath)),
      frame_(dumpPath_.empty() ? 0 : static_cast<size_t>(w_) * h_, 0)
{
}

bool NullPanel::init()
{
    return true;
}

void NullPanel::flush(int x1, int y1, int x2, int y2, const uint16_t *pixels)
{
    if (dumpPath_.empty())
        return;  // discard -- caller just wants a working flush path, no output

    const int rowLen = x2 - x1 + 1;
    for (int y = y1; y <= y2; ++y) {
        const uint16_t *srcRow = pixels + static_cast<size_t>(y - y1) * rowLen;
        uint16_t *dstRow = frame_.data() + static_cast<size_t>(y) * w_ + x1;
        std::copy(srcRow, srcRow + rowLen, dstRow);
    }
    dumpFrame();
}

void NullPanel::dumpFrame() const
{
    std::ofstream f(dumpPath_, std::ios::binary | std::ios::trunc);
    if (!f)
        return;
    f << "P6\n" << w_ << " " << h_ << "\n255\n";
    for (uint16_t px : frame_) {
        // RGB565 -> RGB888, replicating the high bits into the low ones so
        // full black/white still map to 0x00/0xFF rather than 0xF8-style crush.
        uint8_t r5 = (px >> 11) & 0x1F, g6 = (px >> 5) & 0x3F, b5 = px & 0x1F;
        uint8_t r = static_cast<uint8_t>((r5 << 3) | (r5 >> 2));
        uint8_t g = static_cast<uint8_t>((g6 << 2) | (g6 >> 4));
        uint8_t b = static_cast<uint8_t>((b5 << 3) | (b5 >> 2));
        f.put(static_cast<char>(r)).put(static_cast<char>(g)).put(static_cast<char>(b));
    }
}
