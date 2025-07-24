#include <unity.h>
#define NATIVE_TEST
#include "../include/globals.h"

// Define all global variables required by STOOL.cpp
std::vector<std::vector<String>> tasks;
bool SAVE_POWER = false;
int POWER_SAVE_FREQ = 40;
bool SDActive = false;
bool DEBUG_VERBOSE = false;
bool forceSlowFullUpdate = false;
bool newState = false;
int OLED_MAX_FPS = 30;
AppState CurrentAppState = HOME;
TasksState CurrentTasksState = TASKS0;
KBState CurrentKBState = NORMAL;
HOMEState CurrentHOMEState = HOME_HOME;
int selectedTask = 0;
int newTaskState = 0;
int editTaskState = 0;
String newTaskName = "";
String newTaskDueDate = "";
String currentLine = "";
String currentWord = "";

// STOOL-specific variables - declare as extern to use definitions from STOOL.cpp
extern std::vector<std::vector<String>> stoolEntries;
extern StoolState CurrentStoolState;
extern uint8_t selectedStoolEntry;
extern String newStoolNote;

// Add timing variables for keyboard processing
unsigned long KBBounceMillis = 0;
unsigned long OLEDFPSMillis = 0;
bool OLEDPowerSave = false;
bool disableTimeout = false;
int KB_COOLDOWN = 50;

// Mock hardware objects
MockSD_MMC SD_MMC;
MockDisplay display;
MockU8g2 u8g2;

// Mock function implementations
void setCpuFrequencyMhz(int freq) {}
void delay(int ms) {}
void refresh() {}
void oledWord(const String& word) { std::cout << "OLED: " << word << std::endl; }
void oledLine(const String& line, bool progress) { std::cout << "OLED Line: " << line << std::endl; }
void drawStatusBar(const String& s) { std::cout << "Status: " << s << std::endl; }

// Mock keyboard input for testing
char mockKeyInput = 0;
bool mockKeyReady = false;

char updateKeypress() {
  if (mockKeyReady) {
    mockKeyReady = false;
    return mockKeyInput;
  }
  return 0;
}

// Helper function to simulate key press
void simulateKeyPress(char key) {
  mockKeyInput = key;
  mockKeyReady = true;
}

// Helper function to reset timing variables
void resetTimingVars() {
  KBBounceMillis = 0;
  OLEDFPSMillis = 0;
}

int millis() {
  static int mockTime = 1000;
  mockTime += 100; // Advance time on each call
  return mockTime;
}

void delFile(const String& path) {
  std::remove(path.c_str());
}

void appendToFile(const String& path, const String& content) {
  std::ofstream f(path, std::ios::app);
  f << content << std::endl;
  f.close();
}

// Include only the core STOOL functions (not display functions)
#include "../src/STOOL.cpp"

// Black-box E2E Test: Focus on functionality, not internal state
void test_e2e_user_stool_flow() {
  std::cout << "\n=== BLACK-BOX E2E: User Stool Flow ===" << std::endl;
  
  // === SETUP: Clean slate ===
  stoolEntries.clear();
  delFile(STOOL_FILE);
  STOOL_INIT();
  
  // === USER STORY 1: Add multiple stool entries ===
  std::cout << "Adding stool entries..." << std::endl;
  
  // Add first entry: Type 4 (normal)
  addStoolEntry(4, "Normal morning");
  TEST_ASSERT_EQUAL(1, stoolEntries.size());
  TEST_ASSERT_EQUAL_STRING("4", stoolEntries[0][0].c_str());
  TEST_ASSERT_EQUAL_STRING("Normal morning", stoolEntries[0][2].c_str());
  
  // Add second entry: Type 7 (diarrhea) - should be newer and appear first due to sorting
  addStoolEntry(7, "After spicy food");
  TEST_ASSERT_EQUAL(2, stoolEntries.size());
  // Verify entries are sorted by timestamp (most recent first)
  TEST_ASSERT_EQUAL_STRING("7", stoolEntries[0][0].c_str()); // Most recent
  TEST_ASSERT_EQUAL_STRING("4", stoolEntries[1][0].c_str()); // Older entry
  
  // Add third entry: Type 1 (constipated)
  addStoolEntry(1, "Hard to pass");
  TEST_ASSERT_EQUAL(3, stoolEntries.size());
  TEST_ASSERT_EQUAL_STRING("1", stoolEntries[0][0].c_str()); // Most recent
  
  // === USER STORY 2: Test invalid stool type ===
  std::cout << "Testing invalid stool type..." << std::endl;
  
  size_t beforeCount = stoolEntries.size();
  addStoolEntry(8, "Invalid type"); // Should be rejected
  TEST_ASSERT_EQUAL(beforeCount, stoolEntries.size()); // No change
  
  addStoolEntry(0, "Invalid type"); // Should be rejected
  TEST_ASSERT_EQUAL(beforeCount, stoolEntries.size()); // No change
  
  // === USER STORY 3: Test stool type descriptions ===
  std::cout << "Testing stool type descriptions..." << std::endl;
  
  TEST_ASSERT_EQUAL_STRING("Hard lumps", getStoolTypeDescription(1).c_str());
  TEST_ASSERT_EQUAL_STRING("Lumpy sausage", getStoolTypeDescription(2).c_str());
  TEST_ASSERT_EQUAL_STRING("Cracked sausage", getStoolTypeDescription(3).c_str());
  TEST_ASSERT_EQUAL_STRING("Smooth sausage", getStoolTypeDescription(4).c_str());
  TEST_ASSERT_EQUAL_STRING("Soft blobs", getStoolTypeDescription(5).c_str());
  TEST_ASSERT_EQUAL_STRING("Fluffy pieces", getStoolTypeDescription(6).c_str());
  TEST_ASSERT_EQUAL_STRING("Watery", getStoolTypeDescription(7).c_str());
  TEST_ASSERT_EQUAL_STRING("Invalid", getStoolTypeDescription(8).c_str());
  
  // === USER STORY 4: Test timestamp formatting ===
  std::cout << "Testing timestamp formatting..." << std::endl;
  
  String formatted = formatStoolTimestamp("20250115143000");
  TEST_ASSERT_EQUAL_STRING("01/15/25 14:30", formatted.c_str());
  
  String invalidFormatted = formatStoolTimestamp("invalid");
  TEST_ASSERT_EQUAL_STRING("Invalid", invalidFormatted.c_str());
  
  // === USER STORY 5: File persistence ===
  std::cout << "Testing file persistence..." << std::endl;
  
  // Save current entries for comparison
  std::vector<std::vector<String>> entriesBeforeRestart = stoolEntries;
  
  // Simulate app restart
  stoolEntries.clear();
  TEST_ASSERT_EQUAL(0, stoolEntries.size()); // Confirm memory is cleared
  
  // Reload from file
  updateStoolArray();
  
  // Verify all data persisted correctly
  TEST_ASSERT_EQUAL(entriesBeforeRestart.size(), stoolEntries.size());
  for (size_t i = 0; i < entriesBeforeRestart.size(); i++) {
    TEST_ASSERT_EQUAL_STRING(entriesBeforeRestart[i][0].c_str(), stoolEntries[i][0].c_str());
    TEST_ASSERT_EQUAL_STRING(entriesBeforeRestart[i][1].c_str(), stoolEntries[i][1].c_str());
    TEST_ASSERT_EQUAL_STRING(entriesBeforeRestart[i][2].c_str(), stoolEntries[i][2].c_str());
  }
  
  // === USER STORY 6: Delete entries ===
  std::cout << "Testing entry deletion..." << std::endl;
  
  size_t beforeDeleteCount = stoolEntries.size();
  
  // Delete the second entry (index 1)
  deleteStoolEntry(1);
  TEST_ASSERT_EQUAL(beforeDeleteCount - 1, stoolEntries.size());
  
  // Verify the middle entry was removed
  bool foundDeletedEntry = false;
  for (const auto& entry : stoolEntries) {
    if (entry[0] == "7" && entry[2] == "After spicy food") {
      foundDeletedEntry = true;
      break;
    }
  }
  TEST_ASSERT_FALSE(foundDeletedEntry);
  
  // Test deleting invalid index
  size_t beforeInvalidDeleteCount = stoolEntries.size();
  deleteStoolEntry(-1); // Invalid index
  TEST_ASSERT_EQUAL(beforeInvalidDeleteCount, stoolEntries.size());
  
  deleteStoolEntry(100); // Out of bounds
  TEST_ASSERT_EQUAL(beforeInvalidDeleteCount, stoolEntries.size());
  
  // === USER STORY 7: Keyboard interaction flow ===
  std::cout << "Testing keyboard interaction..." << std::endl;
  
  // Start fresh for keyboard testing
  STOOL_INIT();
  TEST_ASSERT_EQUAL(STOOL, CurrentAppState);
  TEST_ASSERT_EQUAL(STOOL0, CurrentStoolState);
  
  // Test adding entry via keyboard (pressing '5')
  size_t beforeKBCount = stoolEntries.size();
  simulateKeyPress('5');
  resetTimingVars();
  processKB_STOOL();
  TEST_ASSERT_EQUAL(beforeKBCount + 1, stoolEntries.size());
  TEST_ASSERT_EQUAL_STRING("5", stoolEntries[0][0].c_str()); // Should be most recent
  
  // Test selecting entry for viewing (pressing 'a' for first entry)
  simulateKeyPress('a');
  resetTimingVars();
  processKB_STOOL();
  TEST_ASSERT_EQUAL(STOOL1, CurrentStoolState);
  TEST_ASSERT_EQUAL(0, selectedStoolEntry);
  
  // Test going back from detail view
  simulateKeyPress(127); // Backspace
  resetTimingVars();
  processKB_STOOL();
  TEST_ASSERT_EQUAL(STOOL0, CurrentStoolState);
  
  // Test editing an entry
  simulateKeyPress('a'); // Select first entry
  resetTimingVars();
  processKB_STOOL();
  
  simulateKeyPress('e'); // Edit
  resetTimingVars();
  processKB_STOOL();
  TEST_ASSERT_EQUAL(STOOL1_EDIT, CurrentStoolState);
  
  // Add some text to the note
  simulateKeyPress('T');
  resetTimingVars();
  processKB_STOOL();
  simulateKeyPress('e');
  resetTimingVars();
  processKB_STOOL();
  simulateKeyPress('s');
  resetTimingVars();
  processKB_STOOL();
  simulateKeyPress('t');
  resetTimingVars();
  processKB_STOOL();
  
  // Save the edit
  simulateKeyPress(13); // Enter
  resetTimingVars();
  processKB_STOOL();
  TEST_ASSERT_EQUAL(STOOL1, CurrentStoolState);
  
  // Verify the note was updated
  TEST_ASSERT_EQUAL_STRING("Test", stoolEntries[selectedStoolEntry][2].c_str());
  
  // Test deleting via keyboard
  simulateKeyPress('d'); // Delete
  resetTimingVars();
  processKB_STOOL();
  TEST_ASSERT_EQUAL(STOOL0, CurrentStoolState);
  
  // === USER STORY 8: Navigation to home ===
  std::cout << "Testing navigation to home..." << std::endl;
  
  // Test exiting to home
  simulateKeyPress(127); // Backspace
  resetTimingVars();
  processKB_STOOL();
  TEST_ASSERT_EQUAL(HOME, CurrentAppState);
  
  // Re-enter stool app
  STOOL_INIT();
  TEST_ASSERT_EQUAL(STOOL, CurrentAppState);
  TEST_ASSERT_EQUAL(STOOL0, CurrentStoolState);
  
  // === USER STORY 9: Test adding entries for all Bristol types ===
  std::cout << "Testing all Bristol stool types..." << std::endl;
  
  // Clear for comprehensive type testing
  stoolEntries.clear();
  delFile(STOOL_FILE);
  
  for (int type = 1; type <= 7; type++) {
    String note = "Type " + String(type) + " test";
    addStoolEntry(type, note);
    TEST_ASSERT_EQUAL(type, stoolEntries.size());
    
    // Find the entry (it might not be at index 0 due to sorting)
    bool found = false;
    for (const auto& entry : stoolEntries) {
      if (entry[0] == String(type) && entry[2] == note) {
        found = true;
        break;
      }
    }
    TEST_ASSERT_TRUE(found);
  }
  
  // === CLEANUP ===
  delFile(STOOL_FILE);
  stoolEntries.clear();
  
  std::cout << "=== BLACK-BOX E2E: User Stool Flow PASSED! ===" << std::endl;
}

// Unity test runner
void setUp(void) {
  // Reset state before each test
  stoolEntries.clear();
  CurrentAppState = HOME;
  CurrentStoolState = STOOL0;
  newState = false;
  forceSlowFullUpdate = false;
  selectedStoolEntry = 0;
  newStoolNote = "";
}

void tearDown(void) {
  // Clean up after each test
  delFile(STOOL_FILE);
  stoolEntries.clear();
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  
  // Single comprehensive e2e test
  RUN_TEST(test_e2e_user_stool_flow);
  
  return UNITY_END();
} 