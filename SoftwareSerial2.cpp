#include <Arduino.h>
#include "SoftwareSerial2.h"
#include <variant.h>
#include <WInterrupts.h>
#include "gps.h"
SoftwareSerial1 *SoftwareSerial1::active_object = 0;
char SoftwareSerial1::_receive_buffer[_SS_MAX_RX_BUFF]; 
volatile uint16_t SoftwareSerial1::_receive_buffer_tail = 0;
volatile uint16_t SoftwareSerial1::_receive_buffer_head = 0;

static EExt_Interrupts DigitalPin_To_Interrupt(uint8_t pin)
{
#if (ARDUINO_SAMD_VARIANT_COMPLIANCE >= 10606)
    return g_APinDescription[pin].ulExtInt;
#else
    return digitalPinToInterrupt(pin);
#endif 
}

bool SoftwareSerial1::listen()
{
  if (!_rx_delay_stopbit)
    return false;

  if (active_object != this)
  {
    if (active_object)
      active_object->stopListening();

    _buffer_overflow = false;
    _receive_buffer_head = _receive_buffer_tail = 0;
    active_object = this;

    if(_inverse_logic)
        //Start bit high
        attachInterrupt(_receivePin, handle_interrupt, RISING);
    else
        //Start bit low
        attachInterrupt(_receivePin, handle_interrupt, FALLING);
    
    
    return true;
  }
 return false;
}
  
bool SoftwareSerial1::stopListening()
{
   if (active_object == this)
   {  
        EIC->INTENCLR.reg = EIC_INTENCLR_EXTINT( 1 << DigitalPin_To_Interrupt( _receivePin )) ;
     active_object = NULL;
     return true;
   }
  return false;
}


void SoftwareSerial1::recv()
{
    
  uint8_t d = 0;
  
  // If RX line is high, then we don't see any start bit
  // so interrupt is probably not for us
  if (_inverse_logic ? rx_pin_read() : !rx_pin_read())
  {

       EIC->INTENCLR.reg = EIC_INTENCLR_EXTINT( 1 << DigitalPin_To_Interrupt(_receivePin));
   
    // Wait approximately 1/2 of a bit width to "center" the sample
       delayMicroseconds(_rx_delay_centering);
   
    // Read each of the 8 bits
    for (uint8_t i=8; i > 0; --i)
    {
        
     delayMicroseconds(_rx_delay_intrabit);
      d >>= 1;
      if (rx_pin_read()){
        d |= 0x80;      
       }
    }
    if (_inverse_logic){
      d = ~d;
    }
    
    // if buffer full, set the overflow flag and return
    uint16_t next = (_receive_buffer_tail + 1) % _SS_MAX_RX_BUFF;
    if (next != _receive_buffer_head)
    {
      // save new data in buffer: tail points to where byte goes
      _receive_buffer[_receive_buffer_tail] = d; // save new byte
      _receive_buffer_tail = next;
    } 
    else 
    {
      _buffer_overflow = true;
    }

    // skip the stop bit
   delayMicroseconds(_rx_delay_stopbit); 

     EIC->INTENSET.reg = EIC_INTENSET_EXTINT( 1 << DigitalPin_To_Interrupt(_receivePin));
  }
}


uint32_t SoftwareSerial1::rx_pin_read()
{ 
  return _receivePortRegister->reg & digitalPinToBitMask(_receivePin);
}

/* static */
inline void SoftwareSerial1::handle_interrupt()
{
   if (active_object)
   {
     GpsstopListening();
     active_object->recv();
   }
}


// Constructor
SoftwareSerial1::SoftwareSerial1(uint8_t receivePin, uint8_t transmitPin, bool inverse_logic /* = false */) : 
  _rx_delay_centering(0),
  _rx_delay_intrabit(0),
  _rx_delay_stopbit(0),
  _tx_delay(0),
  _buffer_overflow(false),
  _inverse_logic(inverse_logic)
{   
  _receivePin = receivePin;
  _transmitPin = transmitPin;
}

// Destructor
SoftwareSerial1::~SoftwareSerial1()
{
  end();
}

void SoftwareSerial1::setTX(uint8_t tx)
{
  // First write, then set output. If we do this the other way around,
  // the pin would be output low for a short while before switching to
  // output hihg. Now, it is input with pullup for a short while, which
  // is fine. With inverse logic, either order is fine.
  digitalWrite(tx, _inverse_logic ? LOW : HIGH);
  pinMode(tx, OUTPUT);
  _transmitBitMask = digitalPinToBitMask(tx);
  PortGroup * port = digitalPinToPort(tx);
  _transmitPortRegister = portOutputReg(port);
 
}

void SoftwareSerial1::setRX(uint8_t rx)
{
  pinMode(rx, INPUT);
  if (!_inverse_logic)
    digitalWrite(rx, HIGH);  // pullup for normal logic!
  _receivePin = rx;
  _receiveBitMask = digitalPinToBitMask(rx);
  PortGroup * port = digitalPinToPort(rx);
  _receivePortRegister = portInputReg(port);

}


void SoftwareSerial1::begin(long speed)
{   
    setTX(_transmitPin);
    setRX(_receivePin);
    // Precalculate the various delays
    //Calculate the distance between bit in micro seconds
  uint32_t bit_delay = (float(1)/speed)*1000000;
 
  _tx_delay = bit_delay;
  
  // Only setup rx when we have a valid PCINT for this pin
  if (DigitalPin_To_Interrupt(_receivePin)!=NOT_AN_INTERRUPT) {
      //Wait 1/2 bit - 2 micro seconds (time for interrupt to be served)
       _rx_delay_centering = (bit_delay/2) - 2;
      //Wait 1 bit - 2 micro seconds (time in each loop iteration)
       _rx_delay_intrabit = bit_delay - 2;
      //Wait 1 bit (the stop one) 
       _rx_delay_stopbit = bit_delay; 
       
      delayMicroseconds(_tx_delay);
      }
      listen();
}

void SoftwareSerial1::end()
{
  stopListening();
}
  
int SoftwareSerial1::read()
{
  if (!isListening()){
    return -1;}


  // Empty buffer?
  if (_receive_buffer_head == _receive_buffer_tail){
    return -1;}

  // Read from "head"
  uint8_t d = _receive_buffer[_receive_buffer_head]; // grab next byte
  _receive_buffer_head = (_receive_buffer_head + 1) % _SS_MAX_RX_BUFF;
  return d;
}  


int SoftwareSerial1::available()
{
  if (!isListening())
    return 0;
  
  return (_receive_buffer_tail + _SS_MAX_RX_BUFF - _receive_buffer_head) % _SS_MAX_RX_BUFF;
}


size_t SoftwareSerial1::write(uint8_t b)
{
  if (_tx_delay == 0) {
    setWriteError();
    return 0;
  }

  // By declaring these as local variables, the compiler will put them
  // in registers _before_ disabling interrupts and entering the
  // critical timing sections below, which makes it a lot easier to
  // verify the cycle timings
  volatile PORT_OUT_Type *reg = _transmitPortRegister;
  uint32_t reg_mask = _transmitBitMask;
  uint32_t inv_mask = ~_transmitBitMask;
  bool inv = _inverse_logic;
  uint16_t delay = _tx_delay;
  
  if (inv)
    b = ~b;
 // turn off interrupts for a clean txmit
   EIC->INTENCLR.reg = EIC_INTENCLR_EXTINT( 1 << DigitalPin_To_Interrupt( _receivePin ));

  // Write the start bit
  if (inv)
    reg->reg |= reg_mask;
  else
    reg->reg &= inv_mask;

  delayMicroseconds(delay);


  // Write each of the 8 bits
  for (uint8_t i = 8; i > 0; --i)
  {
    if (b & 1) // choose bit
      reg->reg |= reg_mask; // send 1
    else
      reg->reg &= inv_mask; // send 0

  delayMicroseconds(delay); 
    b >>= 1;
  }

  // restore pin to natural state
  if (inv)
    reg->reg &= inv_mask;
  else
    reg->reg |= reg_mask;
  

   EIC->INTENSET.reg = EIC_INTENSET_EXTINT( 1 << DigitalPin_To_Interrupt( _receivePin ) ) ;
  
  delayMicroseconds(delay);  
  
  return 1;
}

void SoftwareSerial1::flush()
{
  if (!isListening())
    return;

  EIC->INTENCLR.reg = EIC_INTENCLR_EXTINT( 1 << DigitalPin_To_Interrupt( _receivePin ) ) ;
  
  _receive_buffer_head = _receive_buffer_tail = 0;

   EIC->INTENSET.reg = EIC_INTENSET_EXTINT( 1 << DigitalPin_To_Interrupt( _receivePin ) ) ;
  
}

int SoftwareSerial1::peek()
{
  if (!isListening())
    return -1;

  // Empty buffer?
  if (_receive_buffer_head == _receive_buffer_tail)
    return -1;

  // Read from "head"
  return _receive_buffer[_receive_buffer_head];
}
