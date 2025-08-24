
#include "ram_reader.h"

RamReader::RamReader(int8_t pinCS, SPIClass* spi)
	: _cs(pinCS), _spi(spi), _ramSize(0), _initialized(false) {}

void RamReader::begin() {
	pinMode(_cs, OUTPUT);
	digitalWrite(_cs, HIGH);
	_spi->begin();
	_initialized = true;
}

bool RamReader::isReady() {
	return _initialized;
}

uint8_t RamReader::readByte(uint32_t addr) {
	// Implementación real pendiente
	uint8_t data = 0;
	// ...
	return data;
}

void RamReader::readBlock(uint32_t addr, uint8_t* buffer, size_t len) {
	// Implementación real pendiente
	// ...
}

void RamReader::end() {
	_spi->end();
	_initialized = false;
}
