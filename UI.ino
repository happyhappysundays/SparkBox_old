// Update Icons across top of screen
void updateIcons() {
  
  // Show BT icon if connected
  if(isBTConnected){
    oled.drawXbm(btlogo_pos, 0, bt_width, bt_height, bt_bits);
    // ToDo: measure BT RSSI
    oled.drawXbm(rssi_pos, 0, rssi_width, rssi_height, rssi_bits);
  }
  // Update drive status icons once data available
  if(isStatusReceived || isTimeout){  
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
  // Measure battery periodically via a timer
  if (isTimeout) {
    vbat_result = analogRead(VBAT_AIN);
    isTimeout = false;
  }

  // Coarse cut-offs for visual guide to remaining capacity
  if (vbat_result < 100) {
    oled.drawXbm(bat_pos, 0, bat_width, bat_height, bat00_bits);
  }
  else if (vbat_result < 1500) {
    oled.drawXbm(bat_pos, 0, bat_width, bat_height, bat33_bits);
  }
  else if (vbat_result < 2500) {
    oled.drawXbm(bat_pos, 0, bat_width, bat_height, bat66_bits);
  }
  else if (vbat_result < 3500) {
    oled.drawXbm(bat_pos, 0, bat_width, bat_height, bat100_bits);
  } 
  else {
    oled.drawXbm(bat_pos, 0, bat_width, bat_height, batcharging_bits);
  } 
}

// Print out the requested preset data
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

// Pushbutton handling
void dopushbuttons(void)
{
  // Debounce and long press code
  for (i = 0; i < NUM_SWITCHES; i++) {
    // If the button pin reads LOW, the button is pressed (GND)
    if (digitalRead(sw_pin[i]) == LOW)
    {
      // If button was previously off, mark the button as active, and reset the timer
      if (buttonActive[i] == false){
          buttonActive[i] = true;
          buttonTimer[i] = millis();
       }
       
      // Calculate the button press duration by subtracting the button time from the boot time
      buttonPressDuration[i] = millis() - buttonTimer[i];
      
      // Mark the button as long-pressed if the button press duration exceeds the long press threshold
      if ((buttonPressDuration[i] > longPressThreshold) && (longPressActive[i] == false)) {
          longPressActive[i] = true;
          latchpress = true;
      }
    }
      
    // The button either hasn't been pressed, or has been released
    else {
      // Reset switch register here so that switch is not repeated    
      sw_val[i] = HIGH; 
      
      // If the button was marked as active, it was recently pressed
      if (buttonActive[i] == true){
        // Reset the long press active state
        if (longPressActive[i] == true){
          longPressActive[i] = false;
          latchpress = false;
        }
        
        // The button was previously active. We either need to debounce the press (noise) or register a normal/short press
        else {
          // if the button press duration exceeds our bounce threshold, then we register a short press
          if (buttonPressDuration[i] > debounceThreshold){
            sw_val[i] = LOW;
          }
        }
        
        // Reset the button active status
        buttonActive[i] = false;
      }
    }
  }    

  // OR all the long press flags so any of the four main footswitches can switch modes
  AnylongPressActive = (longPressActive[0] || longPressActive[1] || longPressActive[2] || longPressActive[3]);
 
  // Has any button been held down
  if (AnylongPressActive && (latchpress == true)){
      Serial.println("Switching pedal mode");
      isOLEDUpdate = true;
      latchpress = false;
      if (isPedalMode) isPedalMode = false;   // Toggle pedal mode
      else isPedalMode = true;
  }
}

// Refresh UI
void refreshUI(void)
{
  // if a change has been made or the timer timed out and we have the preset...
  // if ((isOLEDUpdate || isTimeout) && isHWpresetgot){
  if (isOLEDUpdate || isTimeout){  
    isOLEDUpdate = false;  
    oled.clear();
    oled.setFont(ArialMT_Plain_16);
    oled.setTextAlignment(TEXT_ALIGN_LEFT);
    if (!isPedalMode) {
      oled.drawString(0, 20, "Preset mode");
    }
    else {
      oled.drawString(0, 20, "Effect mode");    
    }
    oled.setFont(Roboto_Mono_Bold_52);
    oled.setTextAlignment(TEXT_ALIGN_CENTER);
    oled.drawString(110, 10, String(selected_preset+1));
    oled.setFont(ArialMT_Plain_10);
    oled.setTextAlignment(TEXT_ALIGN_LEFT);

    // Sanity length check on name string
    if (strlen(presets[5].Name) > MAXNAME){
       presets[5].Name[MAXNAME - 1]  = '\0';  // Rudely truncate
    }
    oled.drawString(0, 50, presets[5].Name);
    
    updateIcons();
    oled.display();
  }
}
