#include <unity.h>
#define NATIVE_TEST
#include "../include/globals.h"

// Define FILEWIZ-specific enums and variables that aren't in globals.h for native tests
enum FileWizState { WIZ0_, WIZ1_, WIZ1_YN, WIZ2_R, WIZ2_C, WIZ3_ };

// Define all global variables required by FILEWIZ.cpp
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

// FILEWIZ-specific variables
FileWizState CurrentFileWizState = WIZ0_;
String workingFile = "";
String filesList[MAX_FILES];
uint8_t fileIndex = 0;

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

// Mock keypad
struct MockKeypad {
  void disableInterrupts() {}
  void enableInterrupts() {}
};
MockKeypad keypad;

// Mock fileWizardallArray
const unsigned char fileWizardallArray[3][1] = {{0}, {0}, {0}};

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

// Mock file operations for FILEWIZ
void renFile(const String& oldFile, const String& newFile) {
  std::cout << "RENAMING: " << oldFile << " -> " << newFile << std::endl;
  // Simulate the SD filesystem behavior by handling the "/" prefix
  String oldPath = oldFile;
  String newPath = newFile;
  if (!oldPath.startsWith("/")) oldPath = "/" + oldPath;
  if (!newPath.startsWith("/")) newPath = "/" + newPath;
  
  // For testing, we'll use the local filesystem but handle the paths correctly
  std::string oldStr = oldPath.c_str();
  std::string newStr = newPath.c_str();
  
  // Remove leading "/" for local filesystem operations
  if (oldStr[0] == '/') oldStr = oldStr.substr(1);
  if (newStr[0] == '/') newStr = newStr.substr(1);
  
  std::cout << "LOCAL RENAME: " << oldStr << " -> " << newStr << std::endl;
  std::rename(oldStr.c_str(), newStr.c_str());
}

void copyFile(const String& oldFile, const String& newFile) {
  std::cout << "COPYING: " << oldFile << " -> " << newFile << std::endl;
  // Simulate the SD filesystem behavior by handling the "/" prefix
  String oldPath = oldFile;
  String newPath = newFile;
  if (!oldPath.startsWith("/")) oldPath = "/" + oldPath;
  if (!newPath.startsWith("/")) newPath = "/" + newPath;
  
  // For testing, we'll use the local filesystem but handle the paths correctly
  std::string oldStr = oldPath.c_str();
  std::string newStr = newPath.c_str();
  
  // Remove leading "/" for local filesystem operations
  if (oldStr[0] == '/') oldStr = oldStr.substr(1);
  if (newStr[0] == '/') newStr = newStr.substr(1);
  
  std::cout << "LOCAL COPY: " << oldStr << " -> " << newStr << std::endl;
  std::ifstream src(oldStr, std::ios::binary);
  std::ofstream dst(newStr, std::ios::binary);
  dst << src.rdbuf();
  src.close();
  dst.close();
}

// Mock listDir function with correct signature
void listDir(MockSD_MMC &fs, const char *dirname) {
  // Simulate finding some files
  filesList[0] = "test1.txt";
  filesList[1] = "test2.txt";
  filesList[2] = "document.txt";
  for (int i = 3; i < MAX_FILES; i++) {
    filesList[i] = "-";
  }
}

// Mock fs namespace
namespace fs {
  struct FS {};
}

// Include only the core FILEWIZ functions (not display functions)
#include "../src/FILEWIZ.cpp"

// Black-box E2E Test: Focus on functionality, not internal state
void test_e2e_user_filewiz_flow() {
  std::cout << "\n=== BLACK-BOX E2E: User FileWiz Flow ===" << std::endl;
  
  // === SETUP: Clean slate ===
  workingFile = "";
  CurrentFileWizState = WIZ0_;
  CurrentAppState = HOME;
  
  // Create test files
  std::ofstream("test1.txt") << "Test file 1 content";
  std::ofstream("test2.txt") << "Test file 2 content";
  std::ofstream("document.txt") << "Document content";
  
  // Manually populate filesList since listDir is only called in display handler
  filesList[0] = "test1.txt";
  filesList[1] = "test2.txt";
  filesList[2] = "document.txt";
  for (int i = 3; i < MAX_FILES; i++) {
    filesList[i] = "-";
  }
  
  FILEWIZ_INIT();
  
  // === USER STORY 1: Select a file ===
  std::cout << "Testing file selection..." << std::endl;
  
  // Select first file (test1.txt)
  simulateKeyPress('1');
  resetTimingVars();
  processKB_FILEWIZ();
  
  TEST_ASSERT_EQUAL(WIZ1_, CurrentFileWizState);
  TEST_ASSERT_EQUAL_STRING("test1.txt", workingFile.c_str());
  
  // === USER STORY 2: Rename a file ===
  std::cout << "Testing file rename..." << std::endl;
  
  // Choose rename option (option 1)
  simulateKeyPress('1');
  resetTimingVars();
  processKB_FILEWIZ();
  
  TEST_ASSERT_EQUAL(WIZ2_R, CurrentFileWizState);
  
  // Enter new filename
  currentWord = "renamed_file";
  simulateKeyPress(13); // Enter
  resetTimingVars();
  processKB_FILEWIZ();
  
  // Verify file was renamed
  TEST_ASSERT_EQUAL(WIZ0_, CurrentFileWizState);
  std::cout << "Checking if renamed_file.txt exists..." << std::endl;
  bool renamedExists = std::ifstream("renamed_file.txt").good();
  std::cout << "Renamed file exists: " << (renamedExists ? "YES" : "NO") << std::endl;
  TEST_ASSERT_TRUE(renamedExists);
  TEST_ASSERT_FALSE(std::ifstream("test1.txt"));
  
  // Update filesList to reflect the rename operation
  filesList[0] = "renamed_file.txt";
  
  // === USER STORY 3: Copy a file ===
  std::cout << "Testing file copy..." << std::endl;
  
  // Select second file (test2.txt)
  simulateKeyPress('2');
  resetTimingVars();
  processKB_FILEWIZ();
  
  TEST_ASSERT_EQUAL(WIZ1_, CurrentFileWizState);
  TEST_ASSERT_EQUAL_STRING("test2.txt", workingFile.c_str());
  
  // Choose copy option (option 3)
  simulateKeyPress('3');
  resetTimingVars();
  processKB_FILEWIZ();
  
  TEST_ASSERT_EQUAL(WIZ2_C, CurrentFileWizState);
  
  // Enter new filename for copy
  currentWord = "test2_copy";
  simulateKeyPress(13); // Enter
  resetTimingVars();
  processKB_FILEWIZ();
  
  // Verify file was copied
  TEST_ASSERT_EQUAL(WIZ0_, CurrentFileWizState);
  TEST_ASSERT_TRUE(std::ifstream("test2.txt")); // Original still exists
  TEST_ASSERT_TRUE(std::ifstream("test2_copy.txt")); // Copy exists
  
  // Update filesList to reflect the copy operation
  filesList[2] = "test2_copy.txt";
  
  // === USER STORY 4: Delete a file ===
  std::cout << "Testing file deletion..." << std::endl;
  
  // Select third file (test2_copy.txt)
  simulateKeyPress('3');
  resetTimingVars();
  processKB_FILEWIZ();
  
  TEST_ASSERT_EQUAL(WIZ1_, CurrentFileWizState);
  TEST_ASSERT_EQUAL_STRING("test2_copy.txt", workingFile.c_str());
  
  // Choose delete option (option 2)
  simulateKeyPress('2');
  resetTimingVars();
  processKB_FILEWIZ();
  
  TEST_ASSERT_EQUAL(WIZ1_YN, CurrentFileWizState);
  
  // Confirm deletion with 'Y'
  simulateKeyPress('y');
  resetTimingVars();
  processKB_FILEWIZ();
  
  // Verify file was deleted
  TEST_ASSERT_EQUAL(WIZ0_, CurrentFileWizState);
  TEST_ASSERT_FALSE(std::ifstream("test2_copy.txt"));
  
  // Update filesList to reflect the deletion
  filesList[2] = "-"; // test2_copy.txt was deleted
  
  // === USER STORY 5: Cancel deletion ===
  std::cout << "Testing deletion cancellation..." << std::endl;
  
  // Select the second file (test2.txt)
  simulateKeyPress('2');
  resetTimingVars();
  processKB_FILEWIZ();
  
  TEST_ASSERT_EQUAL(WIZ1_, CurrentFileWizState);
  TEST_ASSERT_EQUAL_STRING("test2.txt", workingFile.c_str());
  
  // Choose delete option
  simulateKeyPress('2');
  resetTimingVars();
  processKB_FILEWIZ();
  
  TEST_ASSERT_EQUAL(WIZ1_YN, CurrentFileWizState);
  
  // Cancel deletion with 'N'
  simulateKeyPress('n');
  resetTimingVars();
  processKB_FILEWIZ();
  
  // Verify file still exists
  TEST_ASSERT_EQUAL(WIZ1_, CurrentFileWizState);
  TEST_ASSERT_TRUE(std::ifstream("test2.txt"));
  
  // === USER STORY 6: Navigation flow ===
  std::cout << "Testing navigation..." << std::endl;
  
  // Go back to file list
  simulateKeyPress(127); // Backspace
  resetTimingVars();
  processKB_FILEWIZ();
  
  TEST_ASSERT_EQUAL(WIZ0_, CurrentFileWizState);
  
  // Exit to home
  simulateKeyPress(127); // Backspace
  resetTimingVars();
  processKB_FILEWIZ();
  
  TEST_ASSERT_EQUAL(HOME, CurrentAppState);
  
  // Re-enter filewiz
  FILEWIZ_INIT();
  TEST_ASSERT_EQUAL(FILEWIZ, CurrentAppState);
  TEST_ASSERT_EQUAL(WIZ0_, CurrentFileWizState);
  
  // === USER STORY 7: Invalid file selection ===
  std::cout << "Testing invalid file selection..." << std::endl;
  
  // Try to select file 9 (should be "-" or empty)
  simulateKeyPress('9');
  resetTimingVars();
  processKB_FILEWIZ();
  
  // Should still be in WIZ0_ state
  TEST_ASSERT_EQUAL(WIZ0_, CurrentFileWizState);
  
  // === USER STORY 8: Cancel rename operation ===
  std::cout << "Testing rename cancellation..." << std::endl;
  
  // Select a file
  simulateKeyPress('2'); // test2.txt
  resetTimingVars();
  processKB_FILEWIZ();
  
  // Choose rename
  simulateKeyPress('1');
  resetTimingVars();
  processKB_FILEWIZ();
  
  // Cancel rename with backspace
  simulateKeyPress(12); // Ctrl+L (cancel)
  resetTimingVars();
  processKB_FILEWIZ();
  
  // Should be back to WIZ1_
  TEST_ASSERT_EQUAL(WIZ1_, CurrentFileWizState);
  
  // === CLEANUP ===
  std::remove("test2.txt");
  std::remove("test2_copy.txt");
  std::remove("renamed_file.txt");
  
  std::cout << "=== BLACK-BOX E2E: User FileWiz Flow PASSED! ===" << std::endl;
}

// Unity test runner
void setUp(void) {
  // Reset state before each test
  workingFile = "";
  CurrentFileWizState = WIZ0_;
  CurrentAppState = HOME;
  CurrentKBState = NORMAL;
  newState = false;
  forceSlowFullUpdate = false;
  currentWord = "";
  currentLine = "";
  
  // Reset filesList
  for (int i = 0; i < MAX_FILES; i++) {
    filesList[i] = "-";
  }
}

void tearDown(void) {
  // Clean up after each test
  std::remove("test1.txt");
  std::remove("test2.txt");
  std::remove("test2_copy.txt");
  std::remove("renamed_file.txt");
  std::remove("document.txt");
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  
  // Single comprehensive e2e test
  RUN_TEST(test_e2e_user_filewiz_flow);
  
  return UNITY_END();
} 