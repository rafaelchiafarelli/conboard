#include "clipped_panel.hpp"

#include <iostream>

ClippedPanel::ClippedPanel(std::unique_ptr<PanelDriver> inner, int xOffset, int yOffset,
                            int width, int height)
    : inner_(std::move(inner)), xOffset_(xOffset), yOffset_(yOffset),
      width_(width), height_(height)
{
}

bool ClippedPanel::init()
{
    if (!inner_->init())
        return false;

    if (xOffset_ < 0 || yOffset_ < 0 || width_ <= 0 || height_ <= 0 ||
        xOffset_ + width_ > inner_->width() || yOffset_ + height_ > inner_->height()) {
        std::cerr << "ClippedPanel: working box " << width_ << "x" << height_
                  << " at (" << xOffset_ << "," << yOffset_
                  << ") does not fit the panel's " << inner_->width() << "x"
                  << inner_->height() << " physical resolution" << std::endl;
        return false;
    }
    return true;
}

void ClippedPanel::flush(int x1, int y1, int x2, int y2, const uint16_t *pixels)
{
    inner_->flush(x1 + xOffset_, y1 + yOffset_, x2 + xOffset_, y2 + yOffset_, pixels);
}

void ClippedPanel::setBacklight(bool on)
{
    inner_->setBacklight(on);
}
