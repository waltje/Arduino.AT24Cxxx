#include <Arduino.h>
#include "at24cxxx.h"


constexpr size_t MAX_ALLOWED_LEN_IN_REQUESTFROM = 255;


AT24Cxxx::AT24Cxxx(uint8_t address, TwoWire& theWire, int writeDelay, uint16_t size, uint8_t pageSize) :
  _i2cAddress(address),
  _wire(&theWire),
  _size(size),
  _writeDelay(writeDelay),
  _pageSize(pageSize)
{
  _lastError = ERR_OK;
}


void
AT24Cxxx::setWire(TwoWire& theWire)
{
  _wire = &theWire;
}


void
AT24Cxxx::setWire(TwoWire *theWire)
{
  _wire = theWire;
}


// Writes both the chip address and the memory address to the I2C bus.
// Since the way this is done varies between the different chips, this
// function is extracted as a virtual template method which can be
// overridden by the different chips.
void
AT24Cxxx::writeAddress(uint16_t address)
{
  _wire->beginTransmission(_i2cAddress);
  _wire->write((uint8_t)((address >> 8) & 0xff));
  _wire->write((uint8_t)(address & 0xff));
}


int
AT24Cxxx::readBuffer(uint16_t address, uint8_t *data, size_t len)
{
  uint8_t* dataPointer = data;
  size_t lenRemaining = len;
  uint16_t nextAddress = address;
  int totalread = 0;
  uint8_t numberOfReads = 0;

  _lastError = ERR_OK;
  if (len == 0)
    return 0;

  do {
    // Since underlying layers will limit how many bytes we can actually read
    // in one go, we will try to read as many as possible, but see from the
    // result how many were actually read, and make multiple reads until
    // we have all data.
    writeAddress(nextAddress);

    _lastError = _wire->endTransmission();
    if (_lastError != ERR_OK) {
      // If we got a hard error from the TwoWire bus,
      // there is no point in continuing
      break;
    }

    size_t bytesToRead = min(lenRemaining, MAX_ALLOWED_LEN_IN_REQUESTFROM);
    size_t readBytes = _wire->requestFrom(_i2cAddress, bytesToRead);
    size_t i;

    for (i = 0; (i < readBytes) && _wire->available(); i++)
      dataPointer[i] = _wire->read();

    totalread += i;
    lenRemaining -= i;
    dataPointer += i;
    nextAddress += i;
  } while ((lenRemaining > 0) && (++numberOfReads < len));

  return totalread;
}


int
AT24Cxxx::rawWriteBuffer(uint16_t address, const uint8_t *data, size_t len)
{
  size_t written = 0;

  _lastError = ERR_OK;
  writeAddress(address);

  for (written = 0; written < len; written++) {
    // Not using twoWire's built in buffer write since it hides errors from write.
    if (_wire->write(data[written]) != 1) {
      // An error here (not 1) indicates that we have reached the end of the 
      // internal write buffer, so no more data can be written.
      // Stop filling the buffer, write what we got and return the number written
      break;
    }
  }

  _lastError = _wire->endTransmission();

  // The AT24Cxxx chips needs 5-20 ms time after write (tWR Write Cycle Time)
  // to become available again for new operations.
  // It is possible to poll the chip to ask if it is ready, but this is hard to
  // do through the TwoWire-API, so instead we just do a hard wait to ensure
  // that the chip is available again before we finish the operation.
  delay(_writeDelay);

  return _lastError == 0 ? written : 0;
}


int
AT24Cxxx::writeBuffer(uint16_t address, const uint8_t *data, size_t len)
{
  const uint8_t* dataToWrite = data;
  size_t lenRemaining = len;
  uint16_t nextAddress = address;
  int totalWritten = 0;
  size_t numberOfWrites = 0;

  do {
    // Since page write to the AT24Cxxx chips only works within the pages
    // we must make sure to split our writes on page borders.
    // Therefore we start by finding out how far it is to the next page
    // border and only write as many bytes in one write operation.
    uint16_t locationOnPage = nextAddress % _pageSize;
    size_t maxBytesToWrite = _pageSize - locationOnPage;
    size_t bytesToWrite = min(maxBytesToWrite, lenRemaining);

    // Note, due to other internal buffer sizes in the TwoWire libraries
    // we may not be able to write the whole message. Therefore we keep track
    // on how many bytes were actually written and use that number when
    // calculating what to write next
    size_t written = rawWriteBuffer(nextAddress, dataToWrite, bytesToWrite);

    if (getLastError() != ERR_OK) {
      // If we got a hard error from the TwoWire bus, there is no point to continue
      break;
    }

    totalWritten += written;
    lenRemaining -= written;
    dataToWrite += written;
    nextAddress += written;
  } while ((lenRemaining > 0) && (++numberOfWrites < len));

  return totalWritten;
}


uint8_t 
AT24Cxxx::getLastError(void)
{
  return _lastError;
}


uint16_t
AT24Cxxx::length(void)
{
  return _size;
}


uint8_t
AT24Cxxx::read(int idx)
{
  uint8_t result;

  readBuffer(idx, &result, 1);

  return result;
}


void
AT24Cxxx::write(int idx, uint8_t val)
{
  uint8_t data = val;

  writeBuffer(idx, &data, 1);
}


void
AT24Cxxx::update(int idx, uint8_t val)
{
  if (val != read(idx))
    write(idx, val);
}
