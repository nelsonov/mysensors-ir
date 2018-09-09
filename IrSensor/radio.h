#ifndef radio_h
#define radio_h


/***************nRF24***********************/
// Default Pinout
//  GND  <-->   GND     Black
// 3.3V  <-->   3.3V    Red
//    9  <-->   CE      Orange
//   10  <-->   CSN/CS  Yellow
//   11  <-->   MOSI    Blue
//   12  <-->   MISO    Violet
//   13  <-->   SCK     Green
//    2  <-->   IRQ     Gray  (Optional)
//#define   MY_RADIO_RF24
#define   MY_RADIO_NRF24
#define   MY_RF24_CE_PIN  9   // Default
#define   MY_RF24_CS_PIN  10  // Default
//#define   MY_RF24_IRQ_PIN     // OPTIONAL
//#define   MY_RF24_POWER_PIN   // OPTIONAL
//#define   MY_RF24_CHANNEL     // OPTIONAL
//#define   MY_RF24_DATARATE    // OPTIONAL
#define   MY_RF24_SPI_SPEED   (2*1000000ul)
//#define   MY_DEBUG_VERBOSE_RF24
#define   MY_TRANSPORT_UPLINK_CHECK_DISABLED
/**********END nRF24*************************/

#endif // radio_h
