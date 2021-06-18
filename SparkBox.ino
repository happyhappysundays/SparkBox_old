//******************************************************************************************
// SparkBox - BT pedal board for the Spark 40 amp - David Thompson 2021
// Supports four-switch pedals. Hold any of the effect buttons down for 1s to switch
// between Preset mode (1 to 4) and Effect mode (Drive, Mod, Delay, Reverb)
// If you have more pushbuttons, switches 5 and 6 switch presets so the board can be left
// in Effect mode.
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
#define VERSION "0.34"
#define MAXNAME 20

SSD1306Wire oled(0x3c, SDA, SCL);     // OLED Screen Definitions - ADDRESS, SDA, SCL 
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
int pv, pvsum, sum;

void setup() {
  // Initialize device OLED display, and flip screen, as OLED library starts upside-down
  oled.init();
  oled.flipScreenVertically();

  // Read Vbat input
  vbat_result = analogRead(VBAT_AIN);

  // Show message on device screen
  oled.clear();
  oled.setFont(ArialMT_Plain_24);
  oled.setTextAlignment(TEXT_ALIGN_CENTER);
  oled.drawString(64, 16, PGM_NAME);
  oled.setFont(ArialMT_Plain_16);
  oled.setTextAlignment(TEXT_ALIGN_CENTER);
  oled.drawString(64, 42, VERSION);
  oled.display();
 
  spark_io.comms = &spark_comms;        // Create SparkIO comms and connect
  spark_comms.start_bt();
  spark_comms.connect_to_spark();
  isBTConnected = true;
  isPedalMode = false;                  // Preset mode
    
  Serial.begin(115200);                 // Start serial debug console monitoring via ESP32
  while (!Serial);
  
  pre = 0; // Probably unnecessary

  //debug
  spark_io.get_hardware_preset_number();   // Try to use get_hardware_preset_number() to pre-load the correct number
  spark_io.get_preset_details(0x7F);       // Get the preset details for 0-3
  spark_io.get_preset_details(0x0100);     // Get the current preset details
}
  
void loop() {

  spark_io.process();

  // Messages from the amp
  if (spark_io.get_message(&cmdsub, &msg, &preset)) { //there is something there
    
    isStatusReceived = true;    // Hopefully this means we have the status
    isOLEDUpdate = true;        //debug - Flag screen update

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
      dump_preset(preset);//debug
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
    }
  } // get-message

  // Update button-led preset number on receipt of hardware preset number
  pre = selected_preset;

  // Process user input
  dopushbuttons();
  
  // Process command to increment preset PB5
  if (sw_val[5] == HIGH) {      
    // Next preset
    pre++;
    if (pre > 3) pre = 0;
    spark_io.change_hardware_preset(pre);
    spark_io.get_preset_details(0x0100);
  }

  // Process command to decrement preset PB6
  else if (sw_val[4] == HIGH) {  
    // Previous preset
    pre--;
    if (pre < 0) pre = 3;
    spark_io.change_hardware_preset(pre);
    spark_io.get_preset_details(0x0100);
  }

  // Preset mode (SW1-4 directly switch to a preset)
  else if ((sw_val[0] == HIGH)&&(!isPedalMode)) {  
    pre = 0;
    spark_io.change_hardware_preset(pre);
    spark_io.get_preset_details(0x0100);
  }
  else if ((sw_val[1] == HIGH)&&(!isPedalMode)) {  
    pre = 1;
    spark_io.change_hardware_preset(pre);
    spark_io.get_preset_details(0x0100);
  }
  else if ((sw_val[2] == HIGH)&&(!isPedalMode)) {  
    pre = 2;
    spark_io.change_hardware_preset(pre);
    spark_io.get_preset_details(0x0100);
  }
  else if ((sw_val[3] == HIGH)&&(!isPedalMode)) {  
    pre = 3;
    spark_io.change_hardware_preset(pre);
    spark_io.get_preset_details(0x0100);
  }
  
  // Effect mode (SW1-4 switch effects on/off)
  // Drive
  else if ((sw_val[0] == HIGH)&&(isPedalMode)) {    
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
  else if ((sw_val[1] == HIGH)&&(isPedalMode)) {    
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
  else if ((sw_val[2] == HIGH)&&(isPedalMode)) {   
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
  else if ((sw_val[3] == HIGH)&&(isPedalMode)) {   
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
}
