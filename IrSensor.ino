/**
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2015 Sensnology AB
 * Full contributor list: https://github.com/mysensors/Arduino/graphs/contributors
 *
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 *******************************
 * Based on work by 
 *
 * And on example here:
 *  https://www.mysensors.org/build/ir
 * and
 *  https://github.com/mysensors/MySensorsArduinoExamples/blob/master/examples/IrSensor/IrSensor.ino
 *
 * Uses Ken Shirriff's IRRemote library:
 *    DESCRIPTION
 *    IRrecord: record and play back IR signals as a minimal 
 *    An IR detector/demodulator must be connected to the input RECV_PIN.
 *    An IR LED must be connected to the output PWM pin 3.
 *    The logic is:
 *    If a V_IR_RECORD is received the node enters in record mode and once a valid IR message has been received 
 *    it is stored in EEPROM. The first byte of the V_IR_RECORD message will be used as preset ID 
 *    If a V_IR_SEND the IR message beloning to the preset number of the first message byte is broadcasted
 *    Version 0.11 September, 2009
 *    Copyright 2009 Ken Shirriff
 *    http://arcfn.com
 */

#include "radio.h"
#include <SPI.h>
#include <MySensors.h>
#include <IRremote.h> 
#include "IrSensor.h"

// Enable debug prints
#define MY_DEBUG
#define MY_BAUD_RATE    9600

#define MY_NODE_ID      12


//*************** IR Remote Tx/Rx Options*************
//
int RECV_PIN = 8;             // pin for IR receiver
#define SENDDELAY             1000
#define HOLDTIME              200
#define INACTIVE              254
#define MAX_STORED_IR_CODES   10
#define RCDELAY               500
char rgb[7] = "ffffff";       // RGB value.


bool initialValueSent = false;
IRCode            StoredIRCodes[MAX_STORED_IR_CODES];
IRrecv            irrecv(RECV_PIN);
IRsend            irsend;
decode_results    ircode;
#define           NO_PROG_MODE 0xFF
byte              progModeId        = NO_PROG_MODE;
unsigned long     rcwaiting         = 0;
boolean           rcpaused          = false;
#define RECEIVE_CHILD_ID  1
#define SEND_CHILD_ID     1
#define RECORD_CHILD_ID   3
MyMessage msgIrReceive(RECEIVE_CHILD_ID, V_IR_RECEIVE);
MyMessage msgIrSend(SEND_CHILD_ID, V_IR_SEND);
MyMessage msgIrRecord(RECORD_CHILD_ID, V_RGB);
MyMessage msgIrRecordStat(RECORD_CHILD_ID, V_STATUS);


void setup()  
{  
  // Tell MYS Controller that we're NOT recording
  send(msgIrRecord.set(0));
  
  Serial.println(F("Recall EEPROM settings"));
  recallEeprom(sizeof(StoredIRCodes), (byte *)&StoredIRCodes);

  // Start the ir receiver
  irrecv.enableIRIn(); 
  
  Serial.println(F("Init done..."));
}

void presentation () 
{
  // Send the sketch version information to the gateway and Controller
  sendSketchInfo("IR RX/TX", "0.1");

  // Register a sensors to gw. Use binary light for test purposes.
  present(RECEIVE_CHILD_ID, S_IR);
  //present(SEND_CHILD_ID, S_IR);
  present(RECORD_CHILD_ID, S_RGB_LIGHT);
  
}

void controller_presentation()
{
  //send(msgIrReceive.set(0));
  //wait(SENDDELAY);
  //send(msgIrSend.set(0));
  //wait(SENDDELAY);
  send(msgIrRecordStat.set(0));
  wait(SENDDELAY);
  send(msgIrRecord.set(rgb));
  wait(SENDDELAY);
  initialValueSent = true;
  Serial.print("Sent initial values to controller");
}

void loop() 
{
  //For Home Assistant
  if (!initialValueSent) {
    controller_presentation();
  }

  //Get current timestamp
  unsigned long now = millis();

  //Rather than using delay() after processing RC input
  //ignore irrecv for RCDELAY miliseconds
  if ((rcpaused) && (now > rcwaiting)) {
    irrecv.resume();
    rcpaused=false;
  } else if (irrecv.decode(&ircode)) {
    ircode_process(ircode); // RC.ino
    rcpaused=true;
    rcwaiting = now + RCDELAY;
  }
}


