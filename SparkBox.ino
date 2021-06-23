//******************************************************************************************
// SparkBox - BT pedal board for the Spark 40 amp - David Thompson 2021
// Supports four-switch pedals. Hold any of the effect buttons down for 1s to switch
// between Preset mode (1 to 4) and Effect mode (Drive, Mod, Delay, Reverb)
//******************************************************************************************
#include "SSD1306Wire.h"            // https://github.com/ThingPulse/esp8266-oled-ssd1306
#include "BluetoothSerial.h"
#include "Spark.h"                  // Paul Hamshere's SparkIO library https://github.com/paulhamsh/SparkIO
#include "SparkIO.h"                // "
#include "SparkComms.h"             // "
#include "font.h"                   // Custom font
#include "bitmaps.h"                // Custom bitmaps (icons)
#include "UI.h"                     // Any UI-related defines

#define PGM_NAME "SparkBox"
#define VERSION "0.4"
#define MAXNAME 20
//#define HELTEC

// ToDo: BT RSSI interrogation code
// First you send an INQM command (Set inquiry access mode), e.g. AT+INQM=1,9,48
// 1 specifies access mode RSSI, 9 specifies the max # of devices to be discovered, and 48 is a timeout value
// Follow this by a INQ command (Query Nearby Discoverable Devices) AT+INQ
// It will respond with lines like: +INQ:1234:56:0,1F1F,FFC0
// The last parameter in the returned string is the RSSI value. 
// Different makers of Bluetooth modules encode the RSSI value in different ways. 
// This module always returns 16-bit negative numbers in the range FF80 (or maybe FFC0) to FFFF.
// Note try adding "\r\n" to these strings
//
// size_t write(const uint8_t *buffer, size_t size);
// spark_comms.bt->write(RSSI_INQM, sizeof(RSSI_INQM));
//
// const char RSSI_INQM = "AT+INQM=1,9,48\r\n";  // RSSI enquiry code
// const char RSSI_INQ = "AT+INQ\r\n"; 

#ifndef HELTEC
//SSD1306Wire oled(0x3c, SDA, SCL);     // Default OLED Screen Definitions - ADDRESS, SDA, SCL 
SSD1306Wire oled(0x3c, 4, 15);
#else
SSD1306Wire oled(0x3c, 4, 15, 16);    // Heltec Screen Definitions - ADDRESS, SDA, SCL and RST for Heltec boards
#endif

SparkIO spark_io(false);              // Non-passthrough Spark IO (per Paul)
SparkComms spark_comms;
char str[STR_LEN];                    // Used for processing Spark commands from amp
unsigned int cmdsub;
SparkMessage msg;                     // SparkIO messsage/preset variables
SparkPreset preset;
SparkPreset presets[6];               // [5] = current preset
int8_t pre;                           // Internal current preset number
int8_t selected_preset;               // Reported current preset number
int i, j, p;                          // Makes these local later...
hw_timer_t * timer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;
volatile boolean isTimeout = false;   // Update battery icon flag
volatile boolean isRSSIupdate = false;// Update RSSI display flag

// Timer interrupt handler
void IRAM_ATTR onTime() {
   portENTER_CRITICAL_ISR(&timerMux);
   isTimeout = true;
   isRSSIupdate = true;
   portEXIT_CRITICAL_ISR(&timerMux);
}

void setup() {
  // Initialize device OLED display, and flip screen, as OLED library starts upside-down
  oled.init();
  oled.flipScreenVertically();

  // Set pushbutton inputs to pull-ups
  for (i = 0; i < NUM_SWITCHES; i++) {
    pinMode(sw_pin[i], INPUT_PULLUP);
  }
  
  // Read Vbat input
  vbat_result = analogRead(VBAT_AIN);

  // Show welcome message
  oled.clear();
  oled.setFont(ArialMT_Plain_24);
  oled.setTextAlignment(TEXT_ALIGN_CENTER);
  oled.drawString(64, 16, PGM_NAME);
  oled.setFont(ArialMT_Plain_16);
  oled.setTextAlignment(TEXT_ALIGN_CENTER);
  oled.drawString(64, 42, VERSION);
  oled.display();

  Serial.begin(115200);                 // Start serial debug console monitoring via ESP32
  while (!Serial);

  spark_io.comms = &spark_comms;        // Create SparkIO comms and connect
  spark_comms.start_bt();
  spark_comms.bt->register_callback(btEventCallback); // Register BT disconnect handler

  isPedalMode = false;                  // Preset mode

  timer = timerBegin(0, 80, true);            // Setup timer
  timerAttachInterrupt(timer, &onTime, true); // Attach to our handler
  timerAlarmWrite(timer, 1000000, true);      // Once per second, autoreload
  timerAlarmEnable(timer);                    // Start timer
}

void loop() {
  // Check if amp is connected to device
  if(!isBTConnected) {
    Serial.println("Connecting...");
    //isRestarted = true;
    isHWpresetgot = false;

    // Show reconnection message
    oled.clear();
    oled.setFont(ArialMT_Plain_24);
    oled.setTextAlignment(TEXT_ALIGN_CENTER);
    oled.drawString(64, 16, "Connecting");
    oled.setFont(ArialMT_Plain_16);
    oled.setTextAlignment(TEXT_ALIGN_CENTER);
    oled.drawString(64, 42, "Please wait");
    oled.display();
    
    spark_comms.connect_to_spark();
    isBTConnected = true;
    Serial.println("Connected");

    delay(500); //debug
    spark_io.get_hardware_preset_number();   // Try to use get_hardware_preset_number() to pre-load the correct number
    spark_io.get_preset_details(0x7F);       // Get the preset details for 0-3
    spark_io.get_preset_details(0x0100);     // Get the current preset details
  } 
  // Device connected
  else {
    // We'd like to update the screen even before any user input, but only once
    // To do this reliably we have to interrogate the hardware preset number until we've recieved it
 /*   if (!isHWpresetgot){
      spark_io.get_hardware_preset_number();   // Try to use get_hardware_preset_number() to pre-load the correct number
      delay(500);
    }
    */
    spark_io.process();
  
    // Messages from the amp
    if (spark_io.get_message(&cmdsub, &msg, &preset)) { //there is something there
      
      isStatusReceived = true;    // Hopefully this means we have the status
      isOLEDUpdate = true;        // Flag screen update
  
      Serial.print("From Spark: ");
      Serial.println(cmdsub, HEX);
      sprintf(str, "< %4.4x", cmdsub);
  
      // Generic ack from Spark
      if (cmdsub == 0x0301) {
        p = preset.preset_num;
        j = preset.curr_preset;
        if (p == 0x7f)       
          p = 4;
        if (j == 0x01)
          p = 5;
        presets[p] = preset;
        dump_preset(preset);
      }
  
      // Preset changed on amp
      if (cmdsub == 0x0338) {
        selected_preset = msg.param2;
        presets[5] = presets[selected_preset];
        Serial.print("Change to preset: ");
        Serial.println(selected_preset, HEX);
        spark_io.get_preset_details(0x0100);
      }      
      
      // Store current preset in amp
      if (cmdsub == 0x0327) {
        selected_preset = msg.param2;
        if (selected_preset == 0x7f) 
          selected_preset=4;
        presets[selected_preset] = presets[5];
        Serial.print("Store in preset: ");
        Serial.println(selected_preset, HEX);
      }
  
      // Current hardware preset from amp
      if (cmdsub == 0x0310) {
        selected_preset = msg.param2;
        j = msg.param1;
        if (selected_preset == 0x7f) 
          selected_preset = 4;
        if (j == 0x01) 
          selected_preset = 5;
        presets[5] = presets[selected_preset];
        Serial.print("Hardware preset is: ");
        Serial.println(selected_preset, HEX);
        isHWpresetgot = true;
      }
    } // get-message
  
    // Update button-led preset number on receipt of hardware preset number
    pre = selected_preset;
  
    // Process user input
    dopushbuttons();
/*
    // Six-button switch support
    // Process command to increment preset PB5
    if (sw_val[5] == LOW) {      
      // Next preset
      pre++;
      if (pre > 3) pre = 0;
      spark_io.change_hardware_preset(pre);
      spark_io.get_preset_details(0x0100);
    }
  
    // Process command to decrement preset PB6
    else if (sw_val[4] == LOW) {  
      // Previous preset
      pre--;
      if (pre < 0) pre = 3;
      spark_io.change_hardware_preset(pre);
      spark_io.get_preset_details(0x0100);
    }

    // Remaining four button support
    // Preset mode (SW1-4 directly switch to a preset)
    else*/ if ((sw_val[0] == LOW)&&(!isPedalMode)) {  
      pre = 0;
      spark_io.change_hardware_preset(pre);
      spark_io.get_preset_details(0x0100);
    }
    else if ((sw_val[1] == LOW)&&(!isPedalMode)) {  
      pre = 1;
      spark_io.change_hardware_preset(pre);
      spark_io.get_preset_details(0x0100);
    }
    else if ((sw_val[2] == LOW)&&(!isPedalMode)) {  
      pre = 2;
      spark_io.change_hardware_preset(pre);
      spark_io.get_preset_details(0x0100);
    }
    else if ((sw_val[3] == LOW)&&(!isPedalMode)) {  
      pre = 3;
      spark_io.change_hardware_preset(pre);
      spark_io.get_preset_details(0x0100);
    }
    
    // Effect mode (SW1-4 switch effects on/off)
    // Drive
    else if ((sw_val[0] == LOW)&&(isPedalMode)) {    
      if (presets[5].effects[2].OnOff == true) {
        spark_io.turn_effect_onoff(presets[5].effects[2].EffectName,false);
        presets[5].effects[2].OnOff = false;
      }
      else {
        spark_io.turn_effect_onoff(presets[5].effects[2].EffectName,true);
        presets[5].effects[2].OnOff = true;
      }
    } 
    
    // Modulation
    else if ((sw_val[1] == LOW)&&(isPedalMode)) {    
      if (presets[5].effects[4].OnOff == true) {
        spark_io.turn_effect_onoff(presets[5].effects[4].EffectName,false);
        presets[5].effects[4].OnOff = false;
      }
      else {
        spark_io.turn_effect_onoff(presets[5].effects[4].EffectName,true);
        presets[5].effects[4].OnOff = true;
      }
    }
  
    // Delay
    else if ((sw_val[2] == LOW)&&(isPedalMode)) {   
      if (presets[5].effects[5].OnOff == true) {
        spark_io.turn_effect_onoff(presets[5].effects[5].EffectName,false);
        presets[5].effects[5].OnOff = false;
      }
      else {
        spark_io.turn_effect_onoff(presets[5].effects[5].EffectName,true);
        presets[5].effects[5].OnOff = true;
      }
    }
  
    // Reverb
    else if ((sw_val[3] == LOW)&&(isPedalMode)) {   
      if (presets[5].effects[6].OnOff == true) {
        spark_io.turn_effect_onoff(presets[5].effects[6].EffectName,false);
        presets[5].effects[6].OnOff = false;
      }
      else {
        spark_io.turn_effect_onoff(presets[5].effects[6].EffectName,true);
        presets[5].effects[6].OnOff = true;
      }
    } 
    
    // Update hardware preset number with button-led preset number
    selected_preset = pre;
  
    // Refresh screen when necessary
    refreshUI();
    
  } // Connected
} // loop()

// BT disconnect callback
void btEventCallback(esp_spp_cb_event_t event, esp_spp_cb_param_t *param){
  if(event == ESP_SPP_CLOSE_EVT ){    // On BT connection close
    isBTConnected = false;            // Clear connected flag
    Serial.println("Lost BT connection");
    pre = 0; //debug
  }
}
