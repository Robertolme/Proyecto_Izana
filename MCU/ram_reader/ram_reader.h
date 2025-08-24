#ifndef RAM_READER_H
#define RAM_READER_H

#include <Arduino.h>
#include <SPI.h>

class RamReader {
public:
    RamReader(int8_t pinCS, SPIClass* spi = &SPI);
    void begin();
    bool isReady();
    uint8_t readByte(uint32_t addr);
    void readBlock(uint32_t addr, uint8_t* buffer, size_t len);
    void end();

private:
    int8_t _cs;
    SPIClass* _spi;
    uint32_t _ramSize;
    bool _initialized;
};

#endif // RAM_READER_H
