#ifndef PhiloinDMD_H
#define PhiloinDMD_H

#include "Bitmap.h"
#include <SPI.h>

class PhiloinDMD : public Bitmap
{
public:
    explicit PhiloinDMD(int widthPanels = 1, int heightPanels = 1);
    ~PhiloinDMD();

    bool doubleBuffer() const { return _doubleBuffer; }
    void setDoubleBuffer(bool doubleBuffer);
    void swapBuffers();
    void swapBuffersAndCopy();

    void loop();
    void refresh();

    void begin();

    void setBrightness(uint16_t crh);
    static Color fromRGB(uint8_t r, uint8_t g, uint8_t b);

private:
    PhiloinDMD(const PhiloinDMD &other) : Bitmap(other) {}
    PhiloinDMD &operator=(const PhiloinDMD &) { return *this; }

    uint16_t cr;
    bool _doubleBuffer;
    uint8_t phase;
    uint8_t *fb0;
    uint8_t *fb1;
    uint8_t *displayfb;
    unsigned long lastRefresh;
};

#endif