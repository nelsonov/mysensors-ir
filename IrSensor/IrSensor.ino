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

// Enable debug prints
#define MY_DEBUG
#define MY_BAUD_RATE    9600
#define MY_NODE_ID      13

#include "radio.h"
#include <SPI.h>
#include <MySensors.h>
#include <IRremote.h> 
#include "IrSensor.h"

#define CURRENTVERSION 0.5


//*************** IR Remote Tx/Rx Options*************
//
int RECV_PIN = 8;             // pin for IR receiver
#define SENDDELAY             1000
#define HOLDTIME              200
#define INACTIVE              254
#define MAX_STORED_IR_CODES   40
#define RCDELAY               500
char rgb[7] = "ffffff";       // RGB value.


bool initialValueSent = false;
IRCode            StoredIRCodes[MAX_STORED_IR_CODES];
IRrecv            irrecv(RECV_PIN);
IRsend            irsend;
decode_results    ircode;
#define           NO_PROG_MODE 0xFF
byte              progModeId            = NO_PROG_MODE;
unsigned long     PIRwaiting            = 0;
unsigned long     rcwaiting             = 0;
boolean           rcpaused              = false;
#define           PIR_REFRESH           120000; // milliseconds between reports
#define           PIRPIN                2       // PIR. Only pin 2 or 3
volatile int      PIRstate           = false;
volatile int      PIRprevious          = false;

#define RECEIVE_CHILD_ID  1
#define SEND_CHILD_ID     2
#define RECORD_CHILD_ID   3
#define PIR_CHILD_ID      5
MyMessage msgIrReceive(RECEIVE_CHILD_ID, V_IR_RECEIVE);
MyMessage msgIrReceiveTx(RECEIVE_CHILD_ID, V_IR_SEND);
MyMessage msgIrReceiveStatus(RECEIVE_CHILD_ID, V_STATUS);
MyMessage msgIrSend(SEND_CHILD_ID, V_IR_SEND);
MyMessage msgIrSendRx(SEND_CHILD_ID, V_IR_RECEIVE);
MyMessage msgIrSendLight(SEND_CHILD_ID, V_DIMMER);
MyMessage msgIrSendStatus(SEND_CHILD_ID, V_STATUS);
//MyMessage msgIrRecord(RECORD_CHILD_ID, V_CUSTOM);
MyMessage msgIrRecord(RECORD_CHILD_ID, V_IR_RECORD);
MyMessage msgIrRecordLight(RECORD_CHILD_ID, V_DIMMER);
MyMessage msgIrRecordStat(RECORD_CHILD_ID, V_STATUS);
MyMessage msgPIR(PIR_CHILD_ID, V_TRIPPED);
MyMessage msgPIRArmed(PIR_CHILD_ID, V_ARMED);



void setup()  
{  
  // Tell MYS Controller that we're NOT recording
  send(msgIrRecord.set(0));
  
  Serial.println(F("Recall EEPROM settings"));
  recallEeprom(sizeof(StoredIRCodes), (byte *)&StoredIRCodes);

  // Start the ir receiver
  irrecv.enableIRIn();

  attachInterrupt(digitalPinToInterrupt(PIRPIN), PIRinterrupt, CHANGE);

  PIRprevious = digitalRead(PIRPIN);

  controller_presentation();

  Serial.println(F("Init done..."));
}

void presentation () 
{
  // Send the sketch version information to the gateway and Controller
  sendSketchInfo("IR RX/TX", "CURRENTVERSION");

  // Register a sensors to gw. Use binary light for test purposes.
  present(RECEIVE_CHILD_ID, S_IR);
  present(SEND_CHILD_ID, S_IR);
  present(RECORD_CHILD_ID, S_DIMMER);
  present(PIR_CHILD_ID, S_MOTION);
}

void controller_presentation()
{
  send(msgIrReceive.set(1));
  wait(SENDDELAY);
  send(msgIrReceiveTx.set(UNDEFINED));
  wait(SENDDELAY);
  send(msgIrReceiveStatus.set(1));
  wait(SENDDELAY);
  send(msgIrSend.set(1));
  wait(SENDDELAY);
  send(msgIrSendRx.set(UNDEFINED));
  wait(SENDDELAY);
  send(msgIrSendLight.set(1));
  wait(SENDDELAY);
  send(msgIrSendStatus.set(1));
  wait(SENDDELAY);
  send(msgIrRecordLight.set(1));
  wait(SENDDELAY);
  send(msgIrRecordStat.set(0));
  wait(SENDDELAY);
  send(msgPIR.set(0));
  wait(SENDDELAY);
  send(msgPIRArmed.set(1));
  wait(SENDDELAY);
  initialValueSent = true;
  Serial.println("Sent initial values to controller");
}

void loop() 
{

  //Get current timestamp
  unsigned long now = millis();

  if (PIRstate != PIRprevious) {
    PIRprevious=PIRstate;
    send(msgPIR.set(PIRstate==HIGH ? 1 : 0));
    Serial.print("State change, sending: ");
    Serial.println(msgPIR.getInt());
    PIRwaiting = now + PIR_REFRESH;
  } else if ((now > PIRwaiting) || (PIRwaiting==0)) {
    send(msgPIR.set(PIRstate==HIGH ? 1 : 0));
    Serial.print("Refreshing state, sending: ");
    Serial.println(msgPIR.getInt());
    PIRwaiting = now + PIR_REFRESH;
  }
  

  //ignore irrecv for RCDELAY miliseconds
  if ((rcpaused) && (now > rcwaiting)) {
    rcpaused=false;
  } else if (irrecv.decode(&ircode)) {
    ircode_process(ircode); // RC.ino
    irrecv.resume(); // resets state, clears ircode
    rcpaused=true;
    rcwaiting = now + RCDELAY;
  }
}

void PIRinterrupt () {
  PIRstate = digitalRead(PIRPIN);
}

