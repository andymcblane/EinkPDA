// oooooooooo.   .oooooo..o ooooooooooooo   .oooooo.     .oooooo.   ooooo      
// `888'   `Y8b d8P'    `Y8 8'   888   `8  d8P'  `Y8b   d8P'  `Y8b  `888'      
//  888     888 Y88bo.           888      888      888 888      888  888       
//  888oooo888'  `"Y8888o.       888      888      888 888      888  888       
//  888    `88b      `"Y88b      888      888      888 888      888  888       
//  888    .88P oo     .d8P      888      `88b    d88' `88b    d88'  888       
// o888bood8P'  8""88888P'      o888o      `Y8bood8P'   `Y8bood8P'  o888o      
#include "globals.h"

#ifndef NATIVE_TEST
// Hardware-specific code
#else
// Native-specific code
#endif

// Bristol Stool Chart App Implementation
// Storage format: type|timestamp|note
// Example: 4|20250115143000|Normal consistency

// Global variables
std::vector<std::vector<String>> stoolEntries;
StoolState CurrentStoolState = STOOL0;
uint8_t selectedStoolEntry = 0;
String newStoolNote = "";

void STOOL_INIT() {
  CurrentAppState = STOOL;
  CurrentStoolState = STOOL0;
  forceSlowFullUpdate = true;
  newState = true;
  selectedStoolEntry = 0;
  newStoolNote = "";
  updateStoolArray();
}

String getStoolTypeDescription(int type) {
  switch(type) {
    case 1: return "Hard lumps";
    case 2: return "Lumpy sausage";
    case 3: return "Cracked sausage";
    case 4: return "Smooth sausage";
    case 5: return "Soft blobs";
    case 6: return "Fluffy pieces";
    case 7: return "Watery";
    default: return "Invalid";
  }
}

String getCurrentTimestamp() {
  #ifndef NATIVE_TEST
  DateTime now = rtc.now();
  return String(now.year()) + 
         (now.month() < 10 ? "0" : "") + String(now.month()) +
         (now.day() < 10 ? "0" : "") + String(now.day()) +
         (now.hour() < 10 ? "0" : "") + String(now.hour()) +
         (now.minute() < 10 ? "0" : "") + String(now.minute()) +
         (now.second() < 10 ? "0" : "") + String(now.second());
  #else
  // Mock timestamp for testing - increment to ensure different timestamps
  static int mockSeconds = 0;
  mockSeconds++;
  String baseTime = "2025011514300";
  String seconds = String(mockSeconds % 60);
  if (seconds.length() == 1) seconds = "0" + seconds;
  return baseTime + seconds;
  #endif
}

String formatStoolTimestamp(String timestamp) {
  if (timestamp.length() != 14) {
    return "Invalid";
  }
  
  String month = timestamp.substring(4, 6);
  String day = timestamp.substring(6, 8);
  String year = timestamp.substring(2, 4);
  String hour = timestamp.substring(8, 10);
  String minute = timestamp.substring(10, 12);
  
  return month + "/" + day + "/" + year + " " + hour + ":" + minute;
}

void addStoolEntry(int type, String note) {
  if (type < 1 || type > 7) {
    #ifndef NATIVE_TEST
    Serial.println("Invalid stool type");
    #else
    std::cout << "Invalid stool type: " << type << std::endl;
    #endif
    return;
  }
  
  String timestamp = getCurrentTimestamp();
  updateStoolArray();
  stoolEntries.push_back({String(type), timestamp, note});
  
  // Sort by timestamp (most recent first)
  std::sort(stoolEntries.begin(), stoolEntries.end(), 
    [](const std::vector<String> &a, const std::vector<String> &b) {
      return a[1] > b[1]; // Compare timestamp strings (descending)
    });
  
  updateStoolFile();
}

void updateStoolArray() {
  SDActive = true;
  setCpuFrequencyMhz(240);
  delay(50);
  
  File file = SD_MMC.open(STOOL_FILE, "r");
  if (!file) {
    #ifndef NATIVE_TEST
    Serial.println("Failed to open stool file for reading");
    #else
    std::cout << "Failed to open stool file for reading" << std::endl;
    #endif
    return;
  }

  stoolEntries.clear();

  while (file.available()) {
    String line = file.readStringUntil('\n');
    line.trim();
    
    if (line.length() == 0) {
      continue;
    }

    int delimiterPos1 = line.indexOf('|');
    int delimiterPos2 = line.indexOf('|', delimiterPos1 + 1);

    if (delimiterPos1 == -1 || delimiterPos2 == -1) {
      continue; // Skip malformed lines
    }

    String type = line.substring(0, delimiterPos1);
    String timestamp = line.substring(delimiterPos1 + 1, delimiterPos2);
    String note = line.substring(delimiterPos2 + 1);

    stoolEntries.push_back({type, timestamp, note});
  }

  file.close();

  // Sort by timestamp (most recent first)
  std::sort(stoolEntries.begin(), stoolEntries.end(), 
    [](const std::vector<String> &a, const std::vector<String> &b) {
      return a[1] > b[1]; // Compare timestamp strings (descending)
    });

  if (SAVE_POWER) setCpuFrequencyMhz(POWER_SAVE_FREQ);
  SDActive = false;
}

void updateStoolFile() {
  SDActive = true;
  setCpuFrequencyMhz(240);
  delay(50);
  
  delFile(STOOL_FILE);

  for (size_t i = 0; i < stoolEntries.size(); i++) {
    String entryInfo = stoolEntries[i][0] + "|" + stoolEntries[i][1] + "|" + stoolEntries[i][2];
    appendToFile(STOOL_FILE, entryInfo);
  }

  if (SAVE_POWER) setCpuFrequencyMhz(POWER_SAVE_FREQ);
  SDActive = false;
}

void deleteStoolEntry(int index) {
  if (index >= 0 && index < (int)stoolEntries.size()) {
    stoolEntries.erase(stoolEntries.begin() + index);
    updateStoolFile();
  }
}

#ifndef NATIVE_TEST
// Only include complex UI functions for ESP32 build
void processKB_STOOL() {
#else
// Include simplified version for testing
void processKB_STOOL() {
#endif
  if (OLEDPowerSave) {
    #ifndef NATIVE_TEST
    u8g2.setPowerSave(0);
    #endif
    OLEDPowerSave = false;
  }
  int currentMillis = millis();
  disableTimeout = false;
  char inchar;

  switch (CurrentStoolState) {
    case STOOL0:
      CurrentKBState = FUNC;
      //Make keyboard only updates after cooldown
      if (currentMillis - KBBounceMillis >= KB_COOLDOWN) {  
        inchar = updateKeypress();
        //No char received
        if (inchar == 0);
        //BKSP Received - go back to home
        else if (inchar == 127 || inchar == 8 || inchar == 12) {
          CurrentAppState = HOME;
          newState = true;
          forceSlowFullUpdate = true;
          KBBounceMillis = currentMillis;
        }
        //Number pressed (1-7 for stool type)
        else if (inchar >= '1' && inchar <= '7') {
          int stoolType = inchar - '0';
          newStoolNote = "";
          addStoolEntry(stoolType, newStoolNote);
          newState = true;
          forceSlowFullUpdate = true;
          KBBounceMillis = currentMillis;
        }
        //View/edit entries
        else if (inchar >= 'a' && inchar <= 'z') {
          int entryIndex = inchar - 'a';
          if (entryIndex < (int)stoolEntries.size()) {
            selectedStoolEntry = entryIndex;
            CurrentStoolState = STOOL1;
            newState = true;
            forceSlowFullUpdate = true;
            KBBounceMillis = currentMillis;
          }
        }
      }
      break;

    case STOOL1:
      CurrentKBState = NORMAL;
      if (currentMillis - KBBounceMillis >= KB_COOLDOWN) {
        inchar = updateKeypress();
        if (inchar == 0);
        //Go back to main view
        else if (inchar == 127 || inchar == 8) {
          CurrentStoolState = STOOL0;
          newState = true;
          forceSlowFullUpdate = true;
          KBBounceMillis = currentMillis;
        }
        //Delete entry
        else if (inchar == 'd' || inchar == 'D') {
          deleteStoolEntry(selectedStoolEntry);
          CurrentStoolState = STOOL0;
          newState = true;
          forceSlowFullUpdate = true;
          KBBounceMillis = currentMillis;
        }
        //Edit note
        else if (inchar == 'e' || inchar == 'E') {
          if (selectedStoolEntry < stoolEntries.size()) {
            newStoolNote = stoolEntries[selectedStoolEntry][2];
            CurrentStoolState = STOOL1_EDIT;
            newState = true;
            forceSlowFullUpdate = true;
            KBBounceMillis = currentMillis;
          }
        }
      }
      break;

    case STOOL1_EDIT:
      CurrentKBState = NORMAL;
      if (currentMillis - KBBounceMillis >= KB_COOLDOWN) {
        inchar = updateKeypress();
        if (inchar == 0);
        //Save changes
        else if (inchar == 13) { // Enter key
          if (selectedStoolEntry < stoolEntries.size()) {
            stoolEntries[selectedStoolEntry][2] = newStoolNote;
            updateStoolFile();
          }
          CurrentStoolState = STOOL1;
          newState = true;
          forceSlowFullUpdate = true;
          KBBounceMillis = currentMillis;
        }
        //Cancel editing
        else if (inchar == 127 || inchar == 8) {
          CurrentStoolState = STOOL1;
          newState = true;
          forceSlowFullUpdate = true;
          KBBounceMillis = currentMillis;
        }
        //Add characters to note
        else if (inchar >= 32 && inchar <= 126) {
          newStoolNote += inchar;
          KBBounceMillis = currentMillis;
        }
        //Backspace in note
        else if (inchar == 127 && newStoolNote.length() > 0) {
          newStoolNote.remove(newStoolNote.length() - 1);
          KBBounceMillis = currentMillis;
        }
      }
      break;
  }
}

#ifndef NATIVE_TEST
// Display handler for ESP32
void einkHandler_STOOL() {
  if (!newState) return;
  
  display.setRotation(0);
  display.setFullWindow();
  display.fillScreen(GxEPD_WHITE);
  display.setFont(&FreeSerif9pt7b);
  display.setTextColor(GxEPD_BLACK);
  
  switch (CurrentStoolState) {
    case STOOL0:
      // Main stool tracking view
      display.setCursor(10, 25);
      display.print("Bristol Stool Chart");
      
      display.setCursor(10, 50);
      display.print("Enter type (1-7):");
      
      // Show stool type descriptions
      for (int i = 1; i <= 7; i++) {
        display.setCursor(10, 50 + (i * 20));
        display.print(String(i) + ": " + getStoolTypeDescription(i));
      }
      
      // Show recent entries
      display.setCursor(10, 210);
      display.print("Recent entries (a-z to view):");
      
      for (size_t i = 0; i < stoolEntries.size() && i < 5; i++) {
        display.setCursor(10, 230 + (i * 15));
        char label = 'a' + i;
        display.print(String(label) + ": Type " + stoolEntries[i][0] + 
                     " " + formatStoolTimestamp(stoolEntries[i][1]));
      }
      break;
      
    case STOOL1:
      // View individual entry
      if (selectedStoolEntry < stoolEntries.size()) {
        display.setCursor(10, 25);
        display.print("Stool Entry Details");
        
        display.setCursor(10, 55);
        display.print("Type: " + stoolEntries[selectedStoolEntry][0] + 
                     " (" + getStoolTypeDescription(stoolEntries[selectedStoolEntry][0].toInt()) + ")");
        
        display.setCursor(10, 80);
        display.print("Time: " + formatStoolTimestamp(stoolEntries[selectedStoolEntry][1]));
        
        display.setCursor(10, 105);
        display.print("Note: " + stoolEntries[selectedStoolEntry][2]);
        
        display.setCursor(10, 280);
        display.print("E: Edit  D: Delete  Bksp: Back");
      }
      break;
      
    case STOOL1_EDIT:
      // Edit note
      display.setCursor(10, 25);
      display.print("Edit Note");
      
      display.setCursor(10, 55);
      display.print("Type: " + stoolEntries[selectedStoolEntry][0]);
      
      display.setCursor(10, 80);
      display.print("Note: " + newStoolNote + "_");
      
      display.setCursor(10, 280);
      display.print("Enter: Save  Bksp: Cancel");
      break;
  }
  
  statusBar("STOOL");
  refresh();
  newState = false;
}
#endif 