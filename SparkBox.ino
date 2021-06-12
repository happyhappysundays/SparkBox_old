#include "SSD1306Wire.h"            // https://github.com/ThingPulse/esp8266-oled-ssd1306
#include "BluetoothSerial.h"
#include "Spark.h"                  // Paul Hamshere's SparkIO library
#include "SparkIO.h"                // "
#include "SparkComms.h"             // "
#include "font.h"                   // Custom font
#include "bitmaps.h"                // Custom bitmaps (icons)

// Defines

#define PGM_NAME "SparkBox"
#define VERSION "0.3"
#define SWITCH_DEBOUNCE 400
#define NUM_SWITCHES 6
#define VBAT_AIN 32

// Globals

SSD1306Wire oled(0x3c, SDA, SCL);     // OLED Screen Definitions - ADDRESS, SDA, SCL 
SparkIO spark_io(false);              // Non-passthrough Spark IO (per Paul)
SparkComms spark_comms;
char str[STR_LEN];                    // Used for processing Spark commands from amp
char effect_names[4][STR_LEN];        // Track the effect names
unsigned int cmdsub;
SparkMessage msg;
SparkPreset preset;
SparkPreset presets[6];               // [5] = current preset
int8_t pre;                           // Internal current preset number
int8_t selected_preset;               // Reported current preset number
int i, j, p;
int pv;
int vbat_result;                      // For eventual battery monitoring
uint8_t b;

// Flags
bool isBTConnected;                    // Duh
bool isStatusReceived;                 // Status received from Spark
bool isOLEDUpdate;                     // Flag OLED needs refresh

unsigned long sw_last_milli[NUM_SWITCHES];  // Used for debouncing
int sw_val[NUM_SWITCHES];                   
int sw_pin[]{19,18,4,16,5,23};              // Switch gpio numbers
                                            // SW1 Toggle Drive 
                                            // SW2 Toggle Modulation
                                            // SW3 Toggle Delay
                                            // SW4 Toggle Reverb
                                            // SW5 Decrement preset
                                            // SW6 Increment preset

// Update Icons across top of screen
void updateIcons() {
  
  // Show BT icon if connected
  if(isBTConnected){
    oled.drawXbm(btlogo_pos, 0, bt_width, bt_height, bt_bits);
    // ToDo: measure BT RSSI
    oled.drawXbm(rssi_pos, 0, rssi_width, rssi_height, rssi_bits);
  }
  // Update drive status icons once data available
  if(isStatusReceived){  
    // Drive icon
    if (presets[5].effects[2].OnOff){
      oled.drawXbm(drive_pos, 0, icon_width, icon_height, drive_on_bits);
    }
    else {
      oled.drawXbm(drive_pos, 0, icon_width, icon_height, drive_off_bits);   
    }
    // Mod icon
    if (presets[5].effects[4].OnOff) {
      oled.drawXbm(mod_pos, 0, icon_width, icon_height, mod_on_bits);
    }
    else {
       oled.drawXbm(mod_pos, 0, icon_width, icon_height, mod_off_bits);   
    }
    // Delay icon
    if (presets[5].effects[5].OnOff) {
      oled.drawXbm(delay_pos, 0, icon_width, icon_height, delay_on_bits);
    }
    else {
       oled.drawXbm(delay_pos, 0, icon_width, icon_height, delay_off_bits);   
    }
    // Reverb icon
    if (presets[5].effects[6].OnOff) {
      oled.drawXbm(rev_pos, 0, icon_width, icon_height, rev_on_bits);
    }
    else {
       oled.drawXbm(rev_pos, 0, icon_width, icon_height, rev_off_bits);
    }
  }
  
  // Battery icon control
  // ToDo: measure battery periodically via a timer
  vbat_result = analogRead(VBAT_AIN);

  // Coarse cut-offs for visual guide to remaining capacity
  if (vbat_result < 100) {
    oled.drawXbm(bat_pos, 0, bat00_width, bat00_height, bat00_bits);
  }
  else if (vbat_result < 1500) {
    oled.drawXbm(bat_pos, 0, bat33_width, bat33_height, bat33_bits);
  }
  else if (vbat_result < 3500) {
    oled.drawXbm(bat_pos, 0, bat66_width, bat66_height, bat66_bits);
  }
  else {
    oled.drawXbm(bat_pos, 0, bat100_width, bat100_height, bat100_bits);
  } 
}

void dump_preset(SparkPreset preset) {
  int i,j;

  Serial.print(preset.curr_preset); Serial.print(" ");
  Serial.print(preset.preset_num); Serial.print(" ");
  Serial.print(preset.Name); Serial.print(" ");
  Serial.println(preset.Description);

  for (j=0; j<7; j++) {
    Serial.print("    ");
    Serial.print(preset.effects[j].EffectName); Serial.print(" ");
    if (preset.effects[j].OnOff == true) Serial.print(" On "); else Serial.print (" Off ");
    for (i = 0; i < preset.effects[j].NumParameters; i++) {
      Serial.print(preset.effects[j].Parameters[i]); Serial.print(" ");
    }
    Serial.println();
  }
}

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
  
  Serial.begin(115200);                 // Start serial debug console monitoring via ESP32
  while (!Serial);
  
  pre = 0; // Probably unnecessary

  // Reset switch debounce timers and state
  for (i = 0; i < NUM_SWITCHES; i++) {
    sw_last_milli[i] = 0;
    sw_val[i] = LOW;
  }

  //debug
  spark_io.get_hardware_preset_number();   // Try to use get_hardware_preset_number() to pre-load the correct number
  spark_io.get_preset_details(0x0100);     // Show the current preset details
}
  
void loop() {

  spark_io.process();

  // Messages from the amp
  if (spark_io.get_message(&cmdsub, &msg, &preset)) { //there is something there
    // Hopefully this means we have the status
    isStatusReceived = true;

    //debug - Flag screen update
    isOLEDUpdate = true;

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

    // Amp model changed on amp
    if (cmdsub == 0x0306) {
      strcpy(presets[5].effects[3].EffectName, msg.str2);
      Serial.print("Change to amp model ");
      Serial.println(presets[5].effects[3].EffectName);
    }

    // Preset changed on amp
    if (cmdsub == 0x0338) {
      selected_preset = msg.param2;
      presets[5] = presets[selected_preset];
      Serial.print("Change to preset: ");
      Serial.println(selected_preset, HEX);
      //dump_preset(preset[5]);//debug
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

    // Update local effect names with those in preset
    strcpy(effect_names[0],presets[5].effects[2].EffectName);
    strcpy(effect_names[1],presets[5].effects[4].EffectName);
    strcpy(effect_names[2],presets[5].effects[5].EffectName);
    strcpy(effect_names[3],presets[5].effects[6].EffectName);

  } // get-message

  // Update button-led preset number on receipt of hardware preset number
  pre = selected_preset;
  
  // Check switch inputs
  for (i = 0; i < NUM_SWITCHES; i++) {
    // Reset switch state
    sw_val[i] = LOW;
    // See if switch still depressed
    if (sw_last_milli[i] != 0) {
      if (millis() - sw_last_milli[i] > SWITCH_DEBOUNCE) 
        sw_last_milli[i] = 0;
    }
    // Debounce check passed
    if (sw_last_milli[i] == 0) {
      pv = digitalRead(sw_pin[i]);
      sw_val[i] = pv;
      // Timestamp switch press
      if (pv == HIGH) sw_last_milli[i] = millis();
    }
  }

  // Process command to increment preset
  if (sw_val[5] == HIGH) {      
    // Next preset
    pre++;
    if (pre > 3) pre = 0;
    spark_io.change_hardware_preset(pre);
    presets[5] = presets[pre];                // Update current with newly selected
    spark_io.get_preset_details(0x0100);
  }

  // Process command to decrement preset
   else if (sw_val[4] == HIGH) {  
    // Previous preset
    pre--;
    if (pre < 0) pre = 3;
    spark_io.change_hardware_preset(pre);
    presets[5] = presets[pre];                // Update current with newly selected
    spark_io.get_preset_details(0x0100);
  }

  // Process effect toggling
  // Drive
  else if (sw_val[0] == HIGH) {    
    Serial.print(effect_names[0]);
    Serial.print(" is ");
    Serial.print(presets[5].effects[2].OnOff);
    Serial.print(" Toggling ");

    if (presets[5].effects[2].OnOff == true) {
      spark_io.turn_effect_onoff(effect_names[0],false);
      presets[5].effects[2].OnOff = false;
    }
    else {
      spark_io.turn_effect_onoff(effect_names[0],true);
      presets[5].effects[2].OnOff = true;
    }
  } 
  
  // Modulation
  else if (sw_val[1] == HIGH) {    
    Serial.print(effect_names[1]);
    Serial.print(" is ");
    Serial.print(presets[5].effects[4].OnOff);
    Serial.print(" Toggling ");

    if (presets[5].effects[4].OnOff == true) {
      spark_io.turn_effect_onoff(effect_names[1],false);
      presets[5].effects[4].OnOff = false;
    }
    else {
      spark_io.turn_effect_onoff(effect_names[1],true);
      presets[5].effects[4].OnOff = true;
    }
  }

  // Delay
  else if (sw_val[2] == HIGH) {    
    Serial.print(effect_names[2]);
    Serial.print(" is ");
    Serial.print(presets[5].effects[5].OnOff);
    Serial.print(" Toggling ");
      
    if (presets[5].effects[5].OnOff == true) {
      spark_io.turn_effect_onoff(effect_names[2],false);
      presets[5].effects[5].OnOff = false;
    }
    else {
      spark_io.turn_effect_onoff(effect_names[2],true);
      presets[5].effects[5].OnOff = true;
    }
  }

    // Reverb
  else if (sw_val[3] == HIGH) {   
    Serial.print(effect_names[3]);
    Serial.print(" is ");
    Serial.print(presets[5].effects[6].OnOff);
    Serial.print(" Toggling ");
      
    if (presets[5].effects[6].OnOff == true) {
      spark_io.turn_effect_onoff(effect_names[3],false);
      presets[5].effects[6].OnOff = false;
    }
    else {
      spark_io.turn_effect_onoff(effect_names[3],true);
      presets[5].effects[6].OnOff = true;
    }
  } 
  
  // Update hardware preset number with button-led preset number
  selected_preset = pre;

  // Refresh screen when necessary
  if (isOLEDUpdate){
    isOLEDUpdate = false;  
    oled.clear();
    oled.setFont(ArialMT_Plain_16);
    oled.setTextAlignment(TEXT_ALIGN_LEFT);
    oled.drawString(0, 20, "Preset");
    oled.setFont(Roboto_Mono_Bold_52);
    oled.setTextAlignment(TEXT_ALIGN_CENTER);
    oled.drawString(110, 10, String(selected_preset+1));
    oled.setFont(ArialMT_Plain_10);
    oled.setTextAlignment(TEXT_ALIGN_LEFT);
    oled.drawString(0, 50, presets[5].Name); // might need to size-limit this later on
    updateIcons();
    oled.display();
  }
  
  // Preset may have changed so update effect names
  strcpy(effect_names[0],presets[5].effects[2].EffectName);
  strcpy(effect_names[1],presets[5].effects[4].EffectName);
  strcpy(effect_names[2],presets[5].effects[5].EffectName);
  strcpy(effect_names[3],presets[5].effects[6].EffectName);
}
