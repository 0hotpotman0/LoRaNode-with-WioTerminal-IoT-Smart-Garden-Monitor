#ifndef SoftwareSerial2_h
#define SoftwareSerial2_h

#include <inttypes.h>
#include <Stream.h>
#include <variant.h>

/******************************************************************************
* Definitions
******************************************************************************/

#define portOutputReg(port)   ( &(port->OUT) )
#define portInputReg(port)    ( &(port->IN) )
#define portModeReg(port)     ( &(port->DIR) )

#define _SS_MAX_RX_BUFF 1200 // RX buffer size
#ifndef GCC_VERSION
#define GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#endif

class SoftwareSerial1 : public Stream
{
private:
  // per object data
  uint8_t _transmitPin;
  uint8_t _receivePin;
  uint32_t _receiveBitMask;
  volatile PORT_IN_Type *_receivePortRegister;
  uint32_t _transmitBitMask;
  volatile PORT_OUT_Type *_transmitPortRegister;

  // Expressed as 4-cycle delays (must never be 0!)
  uint16_t _rx_delay_centering;
  uint16_t _rx_delay_intrabit;
  uint16_t _rx_delay_stopbit;
  uint16_t _tx_delay;

  uint16_t _buffer_overflow:1;
  uint16_t _inverse_logic:1;

  // static data
  static char _receive_buffer[_SS_MAX_RX_BUFF]; 
  static volatile uint16_t _receive_buffer_tail;
  static volatile uint16_t _receive_buffer_head;
  static SoftwareSerial1 *active_object;

  // private methods
  void recv() __attribute__((__always_inline__));
  uint32_t rx_pin_read();
  void tx_pin_write(uint8_t pin_state) __attribute__((__always_inline__));
  void setTX(uint8_t transmitPin);
  void setRX(uint8_t receivePin);
  void setRxIntMsk(bool enable) __attribute__((__always_inline__));


public:
  // public methods
  SoftwareSerial1(uint8_t receivePin, uint8_t transmitPin, bool inverse_logic = false);
  ~SoftwareSerial1();
  void begin(long speed);
  bool listen();
  void end();
  bool isListening() { return this == active_object; }
  bool stopListening();
  bool overflow() { bool ret = _buffer_overflow; if (ret) _buffer_overflow = false; return ret; }
  int peek();

  virtual size_t write(uint8_t byte);
  virtual int read();
  virtual int available();
  virtual void flush();
  operator bool() { return true; }
  
  using Print::write;

  // public only for easy access by interrupt handlers
  static inline void handle_interrupt() __attribute__((__always_inline__));
};

// Arduino 0012 workaround
#undef int
#undef char
#undef long
#undef byte
#undef float
#undef abs
#undef round

#endif
 
