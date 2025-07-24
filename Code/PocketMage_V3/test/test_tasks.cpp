
#include <unity.h>
#define NATIVE_TEST
#include "../include/globals.h"

// Only include the functions we need for testing, not the display functions
// We'll include TASKS.cpp but skip the einkHandler_TASKS function

// Define all global variables required by TASKS.cpp
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

// Include only the core TASKS functions (not display functions)
#include "../src/TASKS.cpp"

// Black-box E2E Test: Focus on functionality, not internal state
void test_e2e_user_tasks_flow() {
  std::cout << "\n=== BLACK-BOX E2E: User Tasks Flow ===" << std::endl;
  
  // === SETUP: Clean slate ===
  tasks.clear();
  delFile(TASKS_FILE);
  TASKS_INIT();
  
  // === USER STORY 1: Create multiple tasks ===
  std::cout << "Creating tasks..." << std::endl;
  
  // Create first task: "Buy milk" due 2025-01-20
  simulateKeyPress('n');
  resetTimingVars();
  processKB_TASKS();
  
  currentLine = "Buy milk";
  simulateKeyPress(13);
  resetTimingVars();
  processKB_TASKS();
  
  currentLine = "20250120";
  simulateKeyPress(13);
  resetTimingVars();
  processKB_TASKS();
  
  // Verify first task exists
  TEST_ASSERT_EQUAL(1, tasks.size());
  TEST_ASSERT_EQUAL_STRING("Buy milk", tasks[0][0].c_str());
  
  // Create second task: "Doctor appointment" due 2025-01-15 (earlier date)
  simulateKeyPress('n');
  resetTimingVars();
  processKB_TASKS();
  
  currentLine = "Doctor appointment";
  simulateKeyPress(13);
  resetTimingVars();
  processKB_TASKS();
  
  currentLine = "20250115";
  simulateKeyPress(13);
  resetTimingVars();
  processKB_TASKS();
  
  // Verify tasks are auto-sorted by due date
  TEST_ASSERT_EQUAL(2, tasks.size());
  TEST_ASSERT_EQUAL_STRING("Doctor appointment", tasks[0][0].c_str()); // Earlier date first
  TEST_ASSERT_EQUAL_STRING("Buy milk", tasks[1][0].c_str());
  
  // Create third task: "Pay rent" due 2025-01-25
  simulateKeyPress('n');
  resetTimingVars();
  processKB_TASKS();
  
  currentLine = "Pay rent";
  simulateKeyPress(13);
  resetTimingVars();
  processKB_TASKS();
  
  currentLine = "20250125";
  simulateKeyPress(13);
  resetTimingVars();
  processKB_TASKS();
  
  // Verify all three tasks in correct order
  TEST_ASSERT_EQUAL(3, tasks.size());
  TEST_ASSERT_EQUAL_STRING("Doctor appointment", tasks[0][0].c_str()); // 01/15
  TEST_ASSERT_EQUAL_STRING("Buy milk", tasks[1][0].c_str());           // 01/20
  TEST_ASSERT_EQUAL_STRING("Pay rent", tasks[2][0].c_str());           // 01/25
  
  // === USER STORY 2: Handle invalid input gracefully ===
  std::cout << "Testing invalid input handling..." << std::endl;
  
  simulateKeyPress('n');
  resetTimingVars();
  processKB_TASKS();
  
  currentLine = "Invalid task";
  simulateKeyPress(13);
  resetTimingVars();
  processKB_TASKS();
  
  // Try invalid date
  currentLine = "baddate";
  simulateKeyPress(13);
  resetTimingVars();
  processKB_TASKS();
  
  // Should still have only 3 tasks (invalid one not created)
  TEST_ASSERT_EQUAL(3, tasks.size());
  
  // Cancel task creation by providing valid date
  currentLine = "20250130";
  simulateKeyPress(13);
  resetTimingVars();
  processKB_TASKS();
  
  // Now should have 4 tasks
  TEST_ASSERT_EQUAL(4, tasks.size());
  TEST_ASSERT_EQUAL_STRING("Invalid task", tasks[3][0].c_str()); // Last in sort order
  
  // === USER STORY 3: Delete tasks ===
  std::cout << "Testing task deletion..." << std::endl;
  
  // Delete the second task (Buy milk)
  simulateKeyPress('2'); // Select second task (1-indexed)
  resetTimingVars();
  processKB_TASKS();
  
  simulateKeyPress('3'); // Delete option
  resetTimingVars();
  processKB_TASKS();
  
  // Verify "Buy milk" is gone
  TEST_ASSERT_EQUAL(3, tasks.size());
  TEST_ASSERT_EQUAL_STRING("Doctor appointment", tasks[0][0].c_str());
  TEST_ASSERT_EQUAL_STRING("Pay rent", tasks[1][0].c_str());
  TEST_ASSERT_EQUAL_STRING("Invalid task", tasks[2][0].c_str());
  
  // Verify "Buy milk" is not in any task
  bool buyMilkFound = false;
  for (const auto& task : tasks) {
    if (task[0] == "Buy milk") {
      buyMilkFound = true;
      break;
    }
  }
  TEST_ASSERT_FALSE(buyMilkFound);
  
  // === USER STORY 4: File persistence ===
  std::cout << "Testing file persistence..." << std::endl;
  
  // Save current tasks for comparison
  std::vector<std::vector<String>> tasksBeforeRestart = tasks;
  
  // Simulate app restart
  tasks.clear();
  TEST_ASSERT_EQUAL(0, tasks.size()); // Confirm memory is cleared
  
  // Reload from file
  updateTaskArray();
  
  // Verify all data persisted correctly
  TEST_ASSERT_EQUAL(tasksBeforeRestart.size(), tasks.size());
  for (size_t i = 0; i < tasksBeforeRestart.size(); i++) {
    TEST_ASSERT_EQUAL_STRING(tasksBeforeRestart[i][0].c_str(), tasks[i][0].c_str());
    TEST_ASSERT_EQUAL_STRING(tasksBeforeRestart[i][1].c_str(), tasks[i][1].c_str());
  }
  
  // === USER STORY 5: Navigation flow ===
  std::cout << "Testing navigation..." << std::endl;
  
  // User can exit to home from main tasks view
  simulateKeyPress(127); // Backspace
  resetTimingVars();
  processKB_TASKS();
  
  // Should be back in HOME app (we don't care about internal state, just that we're not in tasks anymore)
  // We can verify this by checking that CurrentAppState changed from TASKS
  TEST_ASSERT_EQUAL(HOME, CurrentAppState);
  
  // Re-enter tasks app for final verification
  TASKS_INIT();
  TEST_ASSERT_EQUAL(TASKS, CurrentAppState);
  
  // Tasks should still be there
  TEST_ASSERT_EQUAL(3, tasks.size());
  TEST_ASSERT_EQUAL_STRING("Doctor appointment", tasks[0][0].c_str());
  
  // === USER STORY 6: Date formatting works ===
  std::cout << "Testing date display formatting..." << std::endl;
  
  // Test that our date conversion works for display
  String formatted = convertDateFormat("20250115");
  TEST_ASSERT_EQUAL_STRING("01/15/25", formatted.c_str());
  
  String invalidFormatted = convertDateFormat("invalid");
  TEST_ASSERT_EQUAL_STRING("Invalid", invalidFormatted.c_str());
  
  // === CLEANUP ===
  delFile(TASKS_FILE);
  tasks.clear();
  
  std::cout << "=== BLACK-BOX E2E: User Tasks Flow PASSED! ===" << std::endl;
}

// Unity test runner
void setUp(void) {
  // Reset state before each test
  tasks.clear();
  CurrentAppState = HOME;
  CurrentTasksState = TASKS0;
  newState = false;
  forceSlowFullUpdate = false;
}

void tearDown(void) {
  // Clean up after each test
  delFile(TASKS_FILE);
  tasks.clear();
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  
  // Single comprehensive e2e test
  RUN_TEST(test_e2e_user_tasks_flow);
  
  return UNITY_END();
} 