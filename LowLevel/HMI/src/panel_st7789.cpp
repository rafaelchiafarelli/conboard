#include "panel_st7789.hpp"

#include <gpiod.h>
#include <linux/spi/spidev.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <thread>
#include <vector>

namespace {
// Matches the known-working reference driver's chunk size (spidev's own
// buffered-transfer default cap on many kernel configs); a single ioctl
// covering a whole flush region can silently truncate past this.
constexpr size_t kSpiChunkBytes = 4096;
}  // namespace

namespace {
// ST7789 command set + init sequence. This is NOT a generic/datasheet-derived
// sequence -- it is copied byte-for-byte (command, order, and data) from the
// known-working reference driver for this exact board+panel
// (bart-lobotomy/ST7789/ST7789.py on the dev board), including the specific
// power/gamma trim values that particular panel needs. Do not "clean up" or
// "complete" this sequence against a generic ST7789 datasheet without
// re-verifying on real hardware -- cheap ST7789 clones vary enough in their
// power/gamma trim that a theoretically-more-correct sequence can render
// worse (or nothing) on a specific panel than what's proven to work on it.
constexpr uint8_t CMD_SLPOUT     = 0x11;
constexpr uint8_t CMD_MADCTL     = 0x36;
constexpr uint8_t CMD_COLMOD     = 0x3A;
constexpr uint8_t CMD_PORCTRL    = 0xB2;
constexpr uint8_t CMD_GCTRL      = 0xB7;
constexpr uint8_t CMD_VCOMS      = 0xBB;
constexpr uint8_t CMD_LCMCTRL    = 0xC0;
constexpr uint8_t CMD_VDVVRHEN   = 0xC2;
constexpr uint8_t CMD_VRHS       = 0xC3;
constexpr uint8_t CMD_VDVSET     = 0xC4;
constexpr uint8_t CMD_FRCTR2     = 0xC6;
constexpr uint8_t CMD_PWCTRL1    = 0xD0;
constexpr uint8_t CMD_INVON      = 0x21;
constexpr uint8_t CMD_PVGAMCTRL  = 0xE0;
constexpr uint8_t CMD_NVGAMCTRL  = 0xE1;
constexpr uint8_t CMD_DISPON     = 0x29;
constexpr uint8_t CMD_CASET      = 0x2A;
constexpr uint8_t CMD_RASET      = 0x2B;
constexpr uint8_t CMD_RAMWR      = 0x2C;

constexpr uint8_t COLMOD_16BIT[] = {0x05};
constexpr uint8_t PVGAMCTRL[] = {0x00, 0x19, 0x1E, 0x0A, 0x09, 0x15, 0x3D, 0x44,
                                  0x51, 0x12, 0x03, 0x00, 0x3F, 0x3F};
constexpr uint8_t NVGAMCTRL[] = {0x00, 0x18, 0x1E, 0x0A, 0x09, 0x25, 0x3F, 0x43,
                                  0x52, 0x33, 0x03, 0x00, 0x3F, 0x3F};

// MADCTL byte for each rotation, the standard ST7789 convention (MY/MX/MV
// bits -- MV exchanges row/column addressing, which is what makes 90/270
// "free" hardware rotation rather than a software framebuffer rotation).
// Unlike the rest of this file, NOT yet confirmed against this exact panel.
uint8_t madctlForRotation(int rotation)
{
    switch (rotation) {
        case 90:  return 0x60;
        case 180: return 0xC0;
        case 270: return 0xA0;
        default:  return 0x00;
    }
}
}  // namespace

St7789Panel::St7789Panel(Config cfg) : cfg_(std::move(cfg)) {}

int St7789Panel::width() const
{
    return (cfg_.rotation == 90 || cfg_.rotation == 270) ? cfg_.height : cfg_.width;
}

int St7789Panel::height() const
{
    return (cfg_.rotation == 90 || cfg_.rotation == 270) ? cfg_.width : cfg_.height;
}

St7789Panel::~St7789Panel()
{
    if (spiFd_ >= 0)
        close(spiFd_);
    if (resetLine_) gpiod_line_release(resetLine_);
    if (dcLine_) gpiod_line_release(dcLine_);
    if (backlightLine_) gpiod_line_release(backlightLine_);
    if (chip_) gpiod_chip_close(chip_);
}

bool St7789Panel::init()
{
    spiFd_ = open(cfg_.spiDevice.c_str(), O_RDWR);
    if (spiFd_ < 0) {
        std::cerr << "St7789Panel: could not open " << cfg_.spiDevice << std::endl;
        return false;
    }
    uint8_t mode = cfg_.spiMode;
    uint8_t bits = 8;
    if (ioctl(spiFd_, SPI_IOC_WR_MODE, &mode) < 0 ||
        ioctl(spiFd_, SPI_IOC_WR_BITS_PER_WORD, &bits) < 0 ||
        ioctl(spiFd_, SPI_IOC_WR_MAX_SPEED_HZ, &cfg_.spiSpeedHz) < 0) {
        std::cerr << "St7789Panel: spidev ioctl setup failed on " << cfg_.spiDevice << std::endl;
        return false;
    }

    chip_ = gpiod_chip_open_by_name(cfg_.gpioChip.c_str());
    if (!chip_) {
        std::cerr << "St7789Panel: could not open gpio chip " << cfg_.gpioChip << std::endl;
        return false;
    }
    resetLine_ = gpiod_chip_get_line(chip_, cfg_.resetLine);
    dcLine_    = gpiod_chip_get_line(chip_, cfg_.dcLine);
    if (!resetLine_ || !dcLine_) {
        std::cerr << "St7789Panel: could not get RESET/DC gpio lines" << std::endl;
        return false;
    }
    if (gpiod_line_request_output(resetLine_, "conHMI-st7789-reset", 1) < 0 ||
        gpiod_line_request_output(dcLine_, "conHMI-st7789-dc", 0) < 0) {
        std::cerr << "St7789Panel: could not request RESET/DC gpio lines as output" << std::endl;
        return false;
    }
    if (cfg_.backlightLine >= 0) {
        backlightLine_ = gpiod_chip_get_line(chip_, static_cast<unsigned>(cfg_.backlightLine));
        if (backlightLine_)
            gpiod_line_request_output(backlightLine_, "conHMI-st7789-bl", 1);
    }

    reset();

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    writeCommand(CMD_SLPOUT);
    std::this_thread::sleep_for(std::chrono::milliseconds(150));

    writeCommand(CMD_MADCTL);
    writeData(madctlForRotation(cfg_.rotation));

    writeCommand(CMD_COLMOD);
    writeData(COLMOD_16BIT, sizeof(COLMOD_16BIT));

    writeCommand(CMD_PORCTRL);
    writeData(0x0C);
    writeData(0x0C);

    writeCommand(CMD_GCTRL);
    writeData(0x35);

    writeCommand(CMD_VCOMS);
    writeData(0x1A);

    writeCommand(CMD_LCMCTRL);
    writeData(0x2C);

    writeCommand(CMD_VDVVRHEN);
    writeData(0x01);

    writeCommand(CMD_VRHS);
    writeData(0x0B);

    writeCommand(CMD_VDVSET);
    writeData(0x20);

    writeCommand(CMD_FRCTR2);
    writeData(0x0F);

    writeCommand(CMD_PWCTRL1);
    writeData(0xA4);
    writeData(0xA1);

    writeCommand(CMD_INVON);

    writeCommand(CMD_PVGAMCTRL);
    writeData(PVGAMCTRL, sizeof(PVGAMCTRL));

    writeCommand(CMD_NVGAMCTRL);
    writeData(NVGAMCTRL, sizeof(NVGAMCTRL));

    writeCommand(CMD_DISPON);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    setBacklight(true);
    return true;
}

void St7789Panel::reset()
{
    // High-low-high, 100ms each -- matches the proven-working reference
    // driver's timing exactly (see the comment above the command table).
    gpiod_line_set_value(resetLine_, 1);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    gpiod_line_set_value(resetLine_, 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    gpiod_line_set_value(resetLine_, 1);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

bool St7789Panel::spiTransfer(const uint8_t *data, size_t len)
{
    for (size_t off = 0; off < len; off += kSpiChunkBytes) {
        const size_t chunk = std::min(kSpiChunkBytes, len - off);
        struct spi_ioc_transfer tr = {};
        tr.tx_buf        = static_cast<__u64>(reinterpret_cast<uintptr_t>(data + off));
        tr.len            = static_cast<__u32>(chunk);
        tr.speed_hz       = cfg_.spiSpeedHz;
        tr.bits_per_word  = 8;
        if (ioctl(spiFd_, SPI_IOC_MESSAGE(1), &tr) < 0) {
            std::cerr << "St7789Panel: SPI transfer failed: " << strerror(errno) << std::endl;
            return false;
        }
    }
    return true;
}

void St7789Panel::writeCommand(uint8_t cmd)
{
    gpiod_line_set_value(dcLine_, 0);  // command mode
    spiTransfer(&cmd, 1);
}

void St7789Panel::writeData(const uint8_t *data, size_t len)
{
    gpiod_line_set_value(dcLine_, 1);  // data mode
    spiTransfer(data, len);
}

void St7789Panel::setAddressWindow(int x1, int y1, int x2, int y2)
{
    writeCommand(CMD_CASET);
    uint8_t caset[4] = {
        static_cast<uint8_t>(x1 >> 8), static_cast<uint8_t>(x1 & 0xFF),
        static_cast<uint8_t>(x2 >> 8), static_cast<uint8_t>(x2 & 0xFF),
    };
    writeData(caset, sizeof(caset));

    writeCommand(CMD_RASET);
    uint8_t raset[4] = {
        static_cast<uint8_t>(y1 >> 8), static_cast<uint8_t>(y1 & 0xFF),
        static_cast<uint8_t>(y2 >> 8), static_cast<uint8_t>(y2 & 0xFF),
    };
    writeData(raset, sizeof(raset));

    writeCommand(CMD_RAMWR);
}

void St7789Panel::flush(int x1, int y1, int x2, int y2, const uint16_t *pixels)
{
    setAddressWindow(x1, y1, x2, y2);

    const size_t pixelCount = static_cast<size_t>(x2 - x1 + 1) * (y2 - y1 + 1);
    // ST7789 is big-endian RGB565 over the wire; byte-swap into a scratch buffer.
    std::vector<uint8_t> be(pixelCount * 2);
    for (size_t i = 0; i < pixelCount; ++i) {
        be[i * 2]     = static_cast<uint8_t>(pixels[i] >> 8);
        be[i * 2 + 1] = static_cast<uint8_t>(pixels[i] & 0xFF);
    }
    writeData(be.data(), be.size());
}

void St7789Panel::setBacklight(bool on)
{
    if (backlightLine_)
        gpiod_line_set_value(backlightLine_, on ? 1 : 0);
}
