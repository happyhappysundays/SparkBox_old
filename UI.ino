// Update Icons across top of screen
void updateIcons() {
  
  // Read RSSI from Spark
  iRSSI = ble_getRSSI();

  //Serial.print("RSSI = ");
  //Serial.println(iRSSI);
            
  // Show BT icon if connected
  // Use graduated scale based on the following
  // 0 bars (very poor) < -70db
  // 1 bar (poor) = -70db to -60db
  // 2 bars (fair) = -60db to -50db
  // 3 bars (good) = -40db to -50db
  // 4 bars (excellent) = > -40db
  if(isBTConnected){
    oled.drawXbm(btlogo_pos, 0, bt_width, bt_height, bt_bits);
    // Display BT RSSI icon depending on actual signal
    if (iRSSI > -40) {
      oled.drawXbm(rssi_pos, 0, rssi_width, rssi_height, rssi_4);
    }
    else if (iRSSI > -50) {
      oled.drawXbm(rssi_pos, 0, rssi_width, rssi_height, rssi_3);
    }
    else if (iRSSI > -60) {
      oled.drawXbm(rssi_pos, 0, rssi_width, rssi_height, rssi_2);
    }
     else if (iRSSI > -70) {
      oled.drawXbm(rssi_pos, 0, rssi_width, rssi_height, rssi_2);
    }
    // else no bars 
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
  // Battery icon control - measured periodically via a 1s timer
  // Average readings to reduce noise
  if (isTimeout) {
    vbat_result = analogRead(VBAT_AIN); // Read battery voltage
    temp = vbat_result;

    // While collecting data
    if (vbat_ring_count < VBAT_NUM) {
      vbat_ring_sum += vbat_result;
      vbat_ring_count++;
      vbat_result = vbat_ring_sum / vbat_ring_count;
    }
    // Once enough is gathered, do a decimating average
    else {
      vbat_ring_sum *= 0.9;
      vbat_ring_sum += vbat_result;
      vbat_result = vbat_ring_sum / VBAT_NUM;
    }
    /*
    Serial.print("Vbat = ");
    Serial.print(temp);      
    Serial.print(" Vbat avg = ");
    Serial.print(vbat_result);
    Serial.print(" Count = ");
    Serial.print(vbat_ring_count);
    Serial.print(" Sum = ");
    Serial.println(vbat_ring_sum);
   */ 
    chrg_result = analogRead(CHRG_AIN); // Check state of /CHRG output
    //Serial.print(", /CHRG = ");
    //Serial.println(chrg_result);
    isTimeout = false;
  }

  // Start by showing the empty icon. drawXBM writes OR on the screen so care
  // must be taken not to graphically block out some symbols. This is why the
  // battery full but not charging is the last in the chain.

  // No battery monitoring so just show the empty symbol
  #ifdef BATT_CHECK_0
  oled.drawXbm(bat_pos, 0, bat_width, bat_height, bat00_bits);
  #endif
  
  // For level-based charge detection (not very reliable)
  #ifdef BATT_CHECK_1
    if (vbat_result >= BATTERY_CHRG) {
      oled.drawXbm(bat_pos, 0, bat_width, bat_height, batcharging_bits);
    }
  #endif
      
  // If advanced charge detection available, and charge detected
  #ifdef BATT_CHECK_2
    if (chrg_result < CHRG_LOW) {
      oled.drawXbm(bat_pos, 0, bat_width, bat_height, batcharging_bits);
    }
  #endif
  
  // Basic battery detection available. Coarse cut-offs for visual 
  // guide to remaining capacity. Surprisingly complex feedback to user.
  // No bars = 0% (should not be discharged further)
  // Full symbol = >85%
  #ifndef BATT_CHECK_0
    else if (vbat_result < BATTERY_LOW) {
      oled.drawXbm(bat_pos, 0, bat_width, bat_height, bat00_bits);
    }
    else if (vbat_result < BATTERY_10) {
      oled.drawXbm(bat_pos, 0, bat_width, bat_height, bat10_bits);
    }
    else if (vbat_result < BATTERY_20) {
      oled.drawXbm(bat_pos, 0, bat_width, bat_height, bat20_bits);
    }
    else if (vbat_result < BATTERY_30) {
      oled.drawXbm(bat_pos, 0, bat_width, bat_height, bat30_bits);
    }    
    else if (vbat_result < BATTERY_40) {
      oled.drawXbm(bat_pos, 0, bat_width, bat_height, bat40_bits);
    }
    else if (vbat_result < BATTERY_50) {
      oled.drawXbm(bat_pos, 0, bat_width, bat_height, bat50_bits);
    }
    else if (vbat_result < BATTERY_60) {
      oled.drawXbm(bat_pos, 0, bat_width, bat_height, bat60_bits);
    }
    else if (vbat_result < BATTERY_70) {
      oled.drawXbm(bat_pos, 0, bat_width, bat_height, bat70_bits);
    }        
    else if (vbat_result < BATTERY_80) {
      oled.drawXbm(bat_pos, 0, bat_width, bat_height, bat80_bits);
    }
    else if (vbat_result < BATTERY_90) {
      oled.drawXbm(bat_pos, 0, bat_width, bat_height, bat90_bits);
    }
    else {
      oled.drawXbm(bat_pos, 0, bat_width, bat_height, bat100_bits);
    } 
  #endif
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
    // If the button pin reads HIGH, the button is pressed (VCC)
    if (digitalRead(sw_pin[i]) == HIGH)
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
      sw_val[i] = LOW; 
      
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
            sw_val[i] = HIGH;
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
    oled.drawString(110, 12, String(selected_preset+1));
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
