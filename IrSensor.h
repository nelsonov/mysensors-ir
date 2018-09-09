#ifndef IrSensor_h
#define IrSensor_h

#define MY_RAWBUF  50
const char * TYPE2STRING[] = {
    "UNKONWN",
    "RC5",
    "RC6",
    "NEC",
    "Sony",
    "Panasonic",
    "JVC",
    "SAMSUNG",
    "Whynter",
    "AIWA RC T501",
    "LG",
    "Sanyo",
    "Mitsubishi",
    "Dish",
    "Sharp",
    "Denon"
};
#define Type2String(x)   TYPE2STRING[x < 0 ? 0 : x]
#define AddrTxt          F(" addres: 0x")
#define ValueTxt         F(" value: 0x")
#define NATxt            F(" - not implemented/found")

// Raw or unknown codes requires an Arduino with a larger memory like a MEGA and some changes to store in EEPROM (now max 255 bytes)
// #define IR_SUPPORT_UNKNOWN_CODES
typedef union
{
  struct
  {
    decode_type_t type;            // The type of code
    unsigned long value;           // The data bits if type is not raw
    int           len;             // The length of the code in bits
    unsigned int  address;         // Used by Panasonic & Sharp [16-bits]
  } code;
#ifdef IR_SUPPORT_UNKNOWN_CODES      
  struct
  {
    decode_type_t type;             // The type of code
    unsigned int  codes[MY_RAWBUF];
    byte          count;           // The number of interval samples
  } raw;
#endif
} IRCode;


// Manual Preset IR values -- these are working demo values
// VERA call: luup.call_action("urn:schemas-arduino-cc:serviceId:ArduinoIr1", "SendIrCode", {Index=15}, <device number>)
// One can add up to 240 preset codes (if your memory lasts) to see to correct data connect the Arduino with this plug in and
// look at the serial monitor while pressing the desired RC button
IRCode PresetIRCodes[] = {
    { { RC5, 0x01,       12, 0 }},  // 11 - RC5 key "1" 
    { { RC5, 0x02,       12, 0 }},  // 12 - RC5 key "2"
    { { RC5, 0x03,       12, 0 }},  // 13 - RC5 key "3"
    { { NEC, 0xFF30CF,   32, 0 }},  // 14 - NEC key "1"
    { { NEC, 0xFF18E7,   32, 0 }},  // 15 - NEC key "2"
    { { NEC, 0xFF7A85,   32, 0 }},  // 16 - NEC key "3"
    { { NEC, 0xFF10EF,   32, 0 }},  // 17 - NEC key "4"
    { { NEC, 0xFF38C7,   32, 0 }},  // 18 - NEC key "5"
    { { RC6, 0x800F2401, 36, 0 }},  // 19 - RC6 key "1" MicroSoft Mulitmedia RC
    { { RC6, 0x800F2402, 36, 0 }}   // 20 - RC6 key "2" MicroSoft Mulitmedia RC
};
#define MAX_PRESET_IR_CODES  (sizeof(PresetIRCodes)/sizeof(IRCode))
#define MAX_IR_CODES (MAX_STORED_IR_CODES + MAX_PRESET_IR_CODES)

void receive(const MyMessage &message);
byte lookUpPresetCode (decode_results *ircode);
bool storeRCCode(byte index);
void sendRCCode(byte index);
void dump(decode_results *results);
void storeEeprom(byte len, byte *buf);
void recallEeprom(byte len, byte *buf);

#endif // IrSensor_h
