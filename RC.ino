
void ircode_process(decode_results ircode) {
  dump(&ircode);
  if (progModeId != NO_PROG_MODE) {
  // If we are in PROG mode (Recording) store the new IR code and end PROG mode
    if (storeRCCode(progModeId)) {
      Serial.println(F("Stored "));
          
      // If sucessfull RC decode and storage --> also update the EEPROM
      storeEeprom(sizeof(StoredIRCodes), (byte *)&StoredIRCodes);
      progModeId = NO_PROG_MODE;
           
      // Tell MYS Controller that we're done recording
      send(msgIrRecordStat.set(0));
      send(msgIrRecord.set("ffffff"));
    }
  } else {
    // If we are in Playback mode just tell the MYS Controller we did receive an IR code
    if (ircode.decode_type != UNKNOWN) {
      if (ircode.value != REPEAT) {
      // Look if we found a stored preset 0 => not found
      byte num = lookUpPresetCode(&ircode);
        if (num) {
          // Send IR decode result to the MYS Controller
          Serial.print(F("Found code for preset #"));
          Serial.println(num);
          send(msgIrReceive.set(num));
          wait(HOLDTIME);
          send(msgIrReceive.set(255));
        }
      }
    }
  }  
}


/************************ receive **************/
void receive(const MyMessage &message) {
    Serial.print(F("New message: "));
    Serial.println(message.type);

   if (message.type == V_RGB) { // IR_RECORD V_VAR1
     String indexstring = message.getString();
     Serial.println(indexstring);
     indexstring.toCharArray(rgb, sizeof(rgb));
     Serial.println(rgb);
     progModeId = (byte) strtol(rgb, 0, 16);
     // Get IR record requets for index : paramvalue
     //progModeId = message.getByte() % MAX_STORED_IR_CODES;
     
     // Tell MYS Controller that we're now in recording mode
     send(msgIrRecordStat.set(1));
     send(msgIrRecord.set(indexstring));
      
     Serial.print(F("Record new IR for: "));
     Serial.println(progModeId);
   } else if (message.type == V_IR_SEND) {
     // Send an IR code from offset: paramvalue - no check for legal value
     Serial.print(F("Send IR preset: "));
     byte code = message.getByte() % MAX_IR_CODES;
     if (code == 0) {
       code = MAX_IR_CODES;
     }
     Serial.print(code);
     sendRCCode(code);
   }

   // Start receiving ir again...
   irrecv.enableIRIn(); 
}


byte lookUpPresetCode (decode_results *ircode)
{
    // Get rit of the RC5/6 toggle bit when looking up
    if (ircode->decode_type == RC5)  {
        ircode->value = ircode->value & 0x7FF;
    }
    if (ircode->decode_type == RC6)  {
        ircode->value = ircode->value & 0xFFFF7FFF;
    }
    for (byte index = 0; index < MAX_STORED_IR_CODES; index++)
    {
      if ( StoredIRCodes[index].code.type  == ircode->decode_type &&
           StoredIRCodes[index].code.value == ircode->value       &&
           StoredIRCodes[index].code.len   == ircode->bits)      {
          // The preset number starts with 1 so the last is stored as 0 -> fix this when looking up the correct index
          return (index == 0) ? MAX_STORED_IR_CODES : index;
      }  
    }
    
    for (byte index = 0; index < MAX_PRESET_IR_CODES; index++)
    {
      if ( PresetIRCodes[index].code.type  == ircode->decode_type &&
           PresetIRCodes[index].code.value == ircode->value       &&
           PresetIRCodes[index].code.len   == ircode->bits)      {
          // The preset number starts with 1 so the last is stored as 0 -> fix this when looking up the correct index
          return ((index == 0) ? MAX_PRESET_IR_CODES : index) + MAX_STORED_IR_CODES;
      }  
    }
    // not found so return 0
    return 0;
}

// Stores the code for later playback
bool storeRCCode(byte index) {

  if (ircode.decode_type == UNKNOWN) {
#ifdef IR_SUPPORT_UNKNOWN_CODES  
      Serial.println(F("Received unknown code, saving as raw"));
      // To store raw codes:
      // Drop first value (gap)
      // As of v1.3 of IRLib global values are already in microseconds rather than ticks
      // They have also been adjusted for overreporting/underreporting of marks and spaces
      byte rawCount = min(ircode.rawlen - 1, MY_RAWBUF);
      for (int i = 1; i <= rawCount; i++) {
        StoredIRCodes[index].raw.codes[i - 1] = ircode.rawbuf[i]; // Drop the first value
      };
      return true;
#else 
      return false;
    }
#endif

   if (ircode.value == REPEAT) {
       // Don't record a NEC repeat value as that's useless.
       Serial.println(F("repeat; ignoring."));
       return false;
   }
   // Get rit of the toggle bit when storing RC5/6 
   if (ircode.decode_type == RC5)  {
        ircode.value = ircode.value & 0x07FF;
   }
   if (ircode.decode_type == RC6)  {
        ircode.value = ircode.value & 0xFFFF7FFF;
   }

   StoredIRCodes[index].code.type      = ircode.decode_type;
   StoredIRCodes[index].code.value     = ircode.value;
   StoredIRCodes[index].code.address   = ircode.address;      // Used by Panasonic & Sharp [16-bits]
   StoredIRCodes[index].code.len       = ircode.bits;
   Serial.print(F(" value: 0x"));
   Serial.println(ircode.value, HEX);
   return true;
}


void sendRCCode(byte index) {
   IRCode *pIr = ((index <= MAX_STORED_IR_CODES) ? &StoredIRCodes[index % MAX_STORED_IR_CODES] : &PresetIRCodes[index - MAX_STORED_IR_CODES - 1]);
   
#ifdef IR_SUPPORT_UNKNOWN_CODES  
   if(pIr->code.type == UNKNOWN) {
      // Assume 38 KHz
      irsend.sendRaw(pIr->raw.codes, pIr->raw.count, 38);
      Serial.println(F("Sent raw"));
      return;
   }
#endif

   Serial.print(F(" - sent "));
   Serial.print(Type2String(pIr->code.type));
   if (pIr->code.type == RC5) {
       // For RC5 and RC6 there is a toggle bit for each succesor IR code sent alway toggle this bit, needs to repeat the command 3 times with 100 mS pause
       pIr->code.value ^= 0x0800;
       for (byte i=0; i < 3; i++) {
         if (i > 0) { delay(100); } 
         irsend.sendRC5(pIr->code.value, pIr->code.len);
       }
    } 
    else if (pIr->code.type == RC6) {
       // For RC5 and RC6 there is a toggle bit for each succesor IR code sent alway toggle this bit, needs to repeat the command 3 times with 100 mS pause
       if (pIr->code.len == 20) {
              pIr->code.value ^= 0x10000;
       }
       for (byte i=0; i < 3; i++) {
         if (i > 0) { delay(100); } 
         irsend.sendRC6(pIr->code.value, pIr->code.len);
       }
   }
   else if (pIr->code.type == NEC) {
       irsend.sendNEC(pIr->code.value, pIr->code.len);
    } 
    else if (pIr->code.type == SONY) {
       irsend.sendSony(pIr->code.value, pIr->code.len);
    } 
    else if (pIr->code.type == PANASONIC) {
       irsend.sendPanasonic(pIr->code.address, pIr->code.value);
       Serial.print(AddrTxt);
       Serial.println(pIr->code.address, HEX);
    }
    else if (pIr->code.type == JVC) {
       irsend.sendJVC(pIr->code.value, pIr->code.len, false);
    }
    else if (pIr->code.type == SAMSUNG) {
       irsend.sendSAMSUNG(pIr->code.value, pIr->code.len);
    }
    else if (pIr->code.type == WHYNTER) {
       irsend.sendWhynter(pIr->code.value, pIr->code.len);
    }
    else if (pIr->code.type == AIWA_RC_T501) {
       irsend.sendAiwaRCT501(pIr->code.value);
    }
    else if (pIr->code.type == LG || pIr->code.type == SANYO || pIr->code.type == MITSUBISHI) {
       Serial.println(NATxt);
       return;
    }
    else if (pIr->code.type == DISH) {
      // need to repeat the command 4 times with 100 mS pause
      for (byte i=0; i < 4; i++) {
         if (i > 0) { delay(100); } 
           irsend.sendDISH(pIr->code.value, pIr->code.len);
      }
    }
    else if (pIr->code.type == SHARP) {
       irsend.sendSharp(pIr->code.address, pIr->code.value);
       Serial.print(AddrTxt);
       Serial.println(pIr->code.address, HEX);
    }
    else if (pIr->code.type == DENON) {
       irsend.sendDenon(pIr->code.value, pIr->code.len);
    }
    else {
      // No valid IR type, found it does not make sense to broadcast
      Serial.println(NATxt);
      return; 
    }
    Serial.print(" ");
    Serial.println(pIr->code.value, HEX);
}    

// Dumps out the decode_results structure.
void dump(decode_results *results) {
    int count = results->rawlen;
    
    Serial.print(F("Received : "));
    Serial.print(results->decode_type, DEC);
    Serial.print(F(" "));
    Serial.print(Type2String(results->decode_type));
  
    if (results->decode_type == PANASONIC) {  
      Serial.print(AddrTxt);
      Serial.print(results->address,HEX);
      Serial.print(ValueTxt);
    }
    Serial.print(F(" "));
    Serial.print(results->value, HEX);
    Serial.print(F(" ("));
    Serial.print(results->bits, DEC);
    Serial.println(F(" bits)"));
  
    if (results->decode_type == UNKNOWN) {
      Serial.print(F("Raw ("));
      Serial.print(count, DEC);
      Serial.print(F("): "));
  
      for (int i = 0; i < count; i++) {
        if ((i % 2) == 1) {
          Serial.print(results->rawbuf[i]*USECPERTICK, DEC);
        } 
        else {
          Serial.print(-(int)results->rawbuf[i]*USECPERTICK, DEC);
        }
        Serial.print(" ");
      }
      Serial.println("");
    }
}


// Store IR record struct in EEPROM   
void storeEeprom(byte len, byte *buf)
{
    saveState(0, len);
    for (byte i = 1; i < min(len, 100); i++, buf++)
    {
       saveState(i, *buf);
    }
}

void recallEeprom(byte len, byte *buf)
{
    if (loadState(0) != len)
    {
       Serial.print(F("Corrupt EEPROM preset values and Clear EEPROM"));
       for (byte i = 1; i < min(len, 100); i++, buf++)
       {
           *buf = 0;
           storeEeprom(len, buf);
       }
       return;
    }
    for (byte i = 1; i < min(len, 100); i++, buf++)
    {
       *buf = loadState(i);
    }
}


