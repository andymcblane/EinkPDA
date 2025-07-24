// ooooooooooooo       .o.        .oooooo..o oooo    oooo  .oooooo..o //
// 8'   888   `8      .888.      d8P'    `Y8 `888   .8P'  d8P'    `Y8 //
//      888          .8"888.     Y88bo.       888  d8'    Y88bo.      //
//      888         .8' `888.     `"Y8888o.   88888[       `"Y8888o.  //
//      888        .88ooo8888.        `"Y88b  888`88b.         `"Y88b //
//      888       .8'     `888.  oo     .d8P  888  `88b.  oo     .d8P //
//     o888o     o88o     o8888o 8""88888P'  o888o  o888o 8""88888P'  //  
#include "globals.h"                                                 

#ifndef NATIVE_TEST
// Hardware-specific code
#else
// Native-specific code
#endif

// Functions with conditional logic
void TASKS_INIT() {
  CurrentAppState = TASKS;
  CurrentTasksState = TASKS0;
  forceSlowFullUpdate = true;
  newState = true;
}

void sortTasksByDueDate(std::vector<std::vector<String>> &tasks) {
  std::sort(tasks.begin(), tasks.end(), [](const std::vector<String> &a, const std::vector<String> &b) {
    return a[1] < b[1]; // Compare dueDate strings
  });
}

void addTask(String taskName, String dueDate, String priority, String completed) {
  updateTaskArray();
  tasks.push_back({taskName, dueDate, priority, completed});
  sortTasksByDueDate(tasks);
  updateTasksFile();
}

void updateTaskArray() {
  SDActive = true;
  setCpuFrequencyMhz(240);
  delay(50);
  
  File file = SD_MMC.open(TASKS_FILE, "r");
  if (!file) {
    #ifndef NATIVE_TEST
    Serial.println("Failed to open file for reading");
    #else
    std::cout << "Failed to open file for reading" << std::endl;
    #endif
    return;
  }

  tasks.clear();

  while (file.available()) {
    String line = file.readStringUntil('\n');
    line.trim();
    
    if (line.length() == 0) {
      continue;
    }

    int delimiterPos1 = line.indexOf('|');
    int delimiterPos2 = line.indexOf('|', delimiterPos1 + 1);
    int delimiterPos3 = line.indexOf('|', delimiterPos2 + 1);

    if (delimiterPos1 == -1 || delimiterPos2 == -1 || delimiterPos3 == -1) {
      continue; // Skip malformed lines
    }

    String taskName = line.substring(0, delimiterPos1);
    String dueDate = line.substring(delimiterPos1 + 1, delimiterPos2);
    String priority = line.substring(delimiterPos2 + 1, delimiterPos3);
    String completed = line.substring(delimiterPos3 + 1);

    tasks.push_back({taskName, dueDate, priority, completed});
  }

  file.close();

  if (SAVE_POWER) setCpuFrequencyMhz(POWER_SAVE_FREQ);
  SDActive = false;
}

void updateTasksFile() {
  SDActive = true;
  setCpuFrequencyMhz(240);
  delay(50);
  
  delFile(TASKS_FILE);

  for (size_t i = 0; i < tasks.size(); i++) {
    String taskInfo = tasks[i][0] + "|" + tasks[i][1] + "|" + tasks[i][2] + "|" + tasks[i][3];
    appendToFile(TASKS_FILE, taskInfo);
  }

  if (SAVE_POWER) setCpuFrequencyMhz(POWER_SAVE_FREQ);
  SDActive = false;
}

void deleteTask(int index) {
  if (index >= 0 && index < (int)tasks.size()) {
    tasks.erase(tasks.begin() + index);
  }
}

String convertDateFormat(String yyyymmdd) {
  if (yyyymmdd.length() != 8) {
    #ifndef NATIVE_TEST
    Serial.println(("INVALID DATE: " + yyyymmdd).c_str());
    #else
    std::cout << "INVALID DATE: " << yyyymmdd << std::endl;
    #endif
    return "Invalid";
  }

  String year = yyyymmdd.substring(2, 4);
  String month = yyyymmdd.substring(4, 6);
  String day = yyyymmdd.substring(6, 8);

  return month + "/" + day + "/" + year;
}

#ifndef NATIVE_TEST
// Only include complex UI functions for ESP32 build
void processKB_TASKS() {
#else
// Include simplified version for testing
void processKB_TASKS() {
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

  switch (CurrentTasksState) {
    case TASKS0:
      CurrentKBState = FUNC;
      //Make keyboard only updates after cooldown
      if (currentMillis - KBBounceMillis >= KB_COOLDOWN) {  
        inchar = updateKeypress();
        //No char recieved
        if (inchar == 0);
        //BKSP Recieved
        else if (inchar == 127 || inchar == 8 || inchar == 12) {
          CurrentAppState = HOME;
          currentLine     = "";
          CurrentKBState  = NORMAL;
          CurrentHOMEState = HOME_HOME;
          newState = true;
          break;
        }
        // NEW TASK
        else if (inchar == '/' || inchar == 'n' || inchar == 'N') {
          CurrentTasksState = TASKS0_NEWTASK;
          CurrentKBState = NORMAL;
          newTaskState = 0;
          newState = true;
          break;
        }
        // SELECT A TASK
        else if (inchar >= '0' && inchar <= '9') {
          int taskIndex = (inchar == '0') ? 10 : (inchar - '1');  // Adjust for 1-based input

          // SET SELECTED TASK
          if (taskIndex < tasks.size()) {
            selectedTask = taskIndex;
            // GO TO TASKS1
            CurrentTasksState = TASKS1;
            editTaskState = 0;
            newState = true;
          }
        }

        currentMillis = millis();
        //Make sure oled only updates at 60fps
        if (currentMillis - OLEDFPSMillis >= (1000/OLED_MAX_FPS)) {
          OLEDFPSMillis = currentMillis;
          #ifndef NATIVE_TEST
          oledWord(currentWord);
          #endif
        }
        KBBounceMillis = currentMillis;
      }
      break;
    case TASKS0_NEWTASK:
      if (newTaskState == 1) CurrentKBState = FUNC;

      if (currentMillis - KBBounceMillis >= KB_COOLDOWN) {  
        inchar = updateKeypress();
        // HANDLE INPUTS
        //No char recieved
        if (inchar == 0);                                        
        //SHIFT Recieved
        else if (inchar == 17) {                                  
          if (CurrentKBState == SHIFT) CurrentKBState = NORMAL;
          else CurrentKBState = SHIFT;
        }
        //FN Recieved
        else if (inchar == 18) {                                  
          if (CurrentKBState == FUNC) CurrentKBState = NORMAL;
          else CurrentKBState = FUNC;
        }
        //Space Recieved
        else if (inchar == 32) {                                  
          currentLine += " ";
        }
        //ESC / CLEAR Recieved
        else if (inchar == 20) {                                  
          currentLine = "";
        }
        //BKSP Recieved
        else if (inchar == 8 || inchar == 12) {                  
          if (currentLine.length() > 0) {
            currentLine.remove(currentLine.length() - 1);
          }
        }
        //ENTER Recieved
        else if (inchar == 13) {                          
          // ENTER INFORMATION BASED ON STATE
          switch (newTaskState) {
            case 0: // ENTER TASK NAME
              newTaskName = currentLine;
              currentLine = "";
              newTaskState = 1;
              newState = true;
              break;
            case 1: // ENTER DUE DATE
              String testDate = convertDateFormat(currentLine);
              // DATE IS VALID
              if (testDate != "Invalid") {
                newTaskDueDate = currentLine;

                // ADD NEW TASK
                addTask(newTaskName, newTaskDueDate, "0", "0");
                #ifndef NATIVE_TEST
                oledWord("New Task Added");
                delay(1000);
                #endif

                // RETURN
                currentLine = "";
                newTaskState = 0;
                CurrentTasksState = TASKS0;
                newState = true;
              }
              // DATE IS INVALID
              else {
                #ifndef NATIVE_TEST
                oledWord("Invalid Date");
                delay(1000);
                #endif
                currentLine = "";
              }
              break;
          }
        } 

        else {
          currentLine += inchar;
          if (inchar >= 48 && inchar <= 57) {}  //Only leave FN on if typing numbers
          else if (CurrentKBState != NORMAL) {
            CurrentKBState = NORMAL;
          }
        }

        currentMillis = millis();
        //Make sure oled only updates at 60fps
        if (currentMillis - OLEDFPSMillis >= (1000/OLED_MAX_FPS)) {
          OLEDFPSMillis = currentMillis;
          #ifndef NATIVE_TEST
          oledLine(currentLine, false);
          #endif
        }
      }
      break;
    case TASKS1:
      disableTimeout = false;

      CurrentKBState = FUNC;
      currentMillis = millis();
      //Make sure oled only updates at 60fps
      if (currentMillis - KBBounceMillis >= KB_COOLDOWN) {  
        char inchar = updateKeypress();
        //No char recieved
        if (inchar == 0);
        //BKSP Recieved
        else if (inchar == 127 || inchar == 8 || inchar == 12) {
          CurrentTasksState = TASKS0;
          forceSlowFullUpdate = true;
          newState = true;
          break;
        }
        // SELECT A TASK
        else if (inchar >= '1' && inchar <= '4') {
          if (inchar == '1') {      // RENAME TASK

          }
          else if (inchar == '2') { // CHANGE DUE DATE

          }
          else if (inchar == '3') { // DELETE TASK
            deleteTask(selectedTask);
            updateTasksFile();
            
            CurrentTasksState = TASKS0;
            forceSlowFullUpdate = true;
            newState = true;
          }
          else if (inchar == '4') { // COPY TASK

          }
          
        }

        currentMillis = millis();
        //Make sure oled only updates at 60fps
        if (currentMillis - OLEDFPSMillis >= (1000/OLED_MAX_FPS)) {
          OLEDFPSMillis = currentMillis;
          #ifndef NATIVE_TEST
          oledWord(currentWord);
          #endif
        }
        KBBounceMillis = currentMillis;
      }
      break;
  
  }
}

#ifndef NATIVE_TEST
void einkHandler_TASKS() {
#else
// Skip einkHandler_TASKS for native testing - it has too many hardware dependencies
/*
void einkHandler_TASKS() {
#endif
  switch (CurrentTasksState) {
    case TASKS0:
      if (newState) {
        newState = false;
        display.setRotation(3);
        display.setFullWindow();
        display.fillScreen(GxEPD_WHITE);

        // DRAW APP
        display.drawBitmap(0, 0, tasksApp0, 320, 218, GxEPD_BLACK);

        // DRAW FILE LIST
        updateTaskArray();
        sortTasksByDueDate(tasks);

        if (!tasks.empty()) {
          if (DEBUG_VERBOSE) Serial.println("Printing Tasks");

          drawStatusBar("Select (0-9),New Task (N)");

          int loopCount = std::min((int)tasks.size(), MAX_FILES);
          for (int i = 0; i < loopCount; i++) {
            display.setFont(&FreeSerif9pt7b);
            // PRINT TASK NAME
            display.setCursor(29, 54 + (17 * i));
            display.print(tasks[i][0].c_str());
            // PRINT TASK DUE DATE
            display.setCursor(231, 54 + (17 * i));
            display.print(convertDateFormat(tasks[i][1]).c_str());
            Serial.print(tasks[i][0].c_str()); Serial.println(convertDateFormat(tasks[i][1]).c_str());
          }
        }
        else drawStatusBar("No Tasks! Add New Task (N)");

        refresh();
      }
      break;
      case TASKS0_NEWTASK:
        if (newState) {
          newState = false;
          display.setRotation(3);
          display.setFullWindow();
          display.fillScreen(GxEPD_WHITE);

          // DRAW APP
          display.drawBitmap(0, 0, tasksApp0, 320, 218, GxEPD_BLACK);

          // DRAW FILE LIST
          updateTaskArray();
          sortTasksByDueDate(tasks);

          if (!tasks.empty()) {
            if (DEBUG_VERBOSE) Serial.println("Printing Tasks");

            int loopCount = std::min((int)tasks.size(), MAX_FILES);
            for (int i = 0; i < loopCount; i++) {
              display.setFont(&FreeSerif9pt7b);
              // PRINT TASK NAME
              display.setCursor(29, 54 + (17 * i));
              display.print(tasks[i][0].c_str());
              // PRINT TASK DUE DATE
              display.setCursor(231, 54 + (17 * i));
              display.print(convertDateFormat(tasks[i][1]).c_str());
              Serial.print(tasks[i][0].c_str()); Serial.println(convertDateFormat(tasks[i][1]).c_str());
            }
          }
          switch (newTaskState) {
            case 0:
              drawStatusBar("Enter Task Name:");
              break;
            case 1:
              drawStatusBar("Due Date (YYYYMMDD):");
              break;
          }

          refresh();
        }
        break;
    case TASKS1:
      if (newState) {
        newState = false;
        display.setRotation(3);
        display.setFullWindow();
        display.fillScreen(GxEPD_WHITE);

        // DRAW APP
        drawStatusBar("T:" + tasks[selectedTask][0]);
        display.drawBitmap(0, 0, tasksApp1, 320, 218, GxEPD_BLACK);

        refresh();
      }
      break;
    
  }
}
#ifdef NATIVE_TEST
*/
#endif