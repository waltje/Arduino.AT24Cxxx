#ifndef AT24CXXX_H
# define AT24CXXX_H

# include <Wire.h>


#define AT24C_ADDRESS_0 (0x50)
#define AT24C_ADDRESS_1 (0x51)
#define AT24C_ADDRESS_2 (0x52)
#define AT24C_ADDRESS_3 (0x53)
#define AT24C_ADDRESS_4 (0x54)
#define AT24C_ADDRESS_5 (0x55)
#define AT24C_ADDRESS_6 (0x56)
#define AT24C_ADDRESS_7 (0x57)


class AT24Cxxx {
  public:
    /**
     * The meaning of the error values are:
     *  0 .. success
     *  1 .. length too long for buffer
     *  2 .. address sent, NACK received - typically means no device at the address
     *  3 .. data sent, NACK received
     *  4 .. other twi error (lost bus arbitration, bus error, ..)
     */
    enum at24c_error {
      ERR_OK = 0,
      ERR_TOOBIG,
      ERR_NACK,
      ERR_DNACK,
      ERR_BUS
    };

    AT24Cxxx(uint8_t address, TwoWire& theWire, int writeDelay, uint16_t size, uint8_t pageSize);
    void setWire(TwoWire& theWire);
    void setWire(TwoWire *theWire);

    /**
     * Returns result from the last performed operation.
     */
    uint8_t getLastError(void);

    uint16_t length(void);
    int readBuffer(uint16_t address, uint8_t *data, size_t len);
    int writeBuffer(uint16_t address, const uint8_t *data, size_t len);
    uint8_t read(int idx);
    void write(int idx, uint8_t val);
    void update(int idx, uint8_t val);

    template< typename T > T &get(int idx, T &t) {
      readBuffer(idx, (uint8_t*)&t, sizeof(T));
      return t;
    }

    template< typename T > const T &put(int idx, const T &t) {
      writeBuffer(idx, (uint8_t*)&t, sizeof(T));
      return t;
    }

  protected:
    virtual void writeAddress(uint16_t address);
    uint8_t _i2cAddress;
    TwoWire *_wire;

  private:
    int rawWriteBuffer(uint16_t address, const uint8_t* data, size_t len);
    int _size;
    uint8_t _writeDelay;
    uint8_t _lastError;
    uint8_t _pageSize;
};


#endif  /*AT24CXXX_H*/
