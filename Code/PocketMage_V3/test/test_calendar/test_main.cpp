#ifdef NATIVE_TEST
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
// Native test-specific CalendarState and variables
enum CalendarState { MONTH, WEEK };
CalendarState CurrentCalendarState = MONTH;
// Mock Serial
struct MockSerialStream : public std::ostream {
  MockSerialStream() : std::ostream(std::cout.rdbuf()) {}
  void println(const char* s) { std::cout << s << std::endl; }
  void println(const std::string& s) { std::cout << s << std::endl; }
};
static MockSerialStream Serial;
// Mock isDigit
inline bool isDigit(char c) { return c >= '0' && c <= '9'; }
// Mock calendar_allArray
const unsigned char dummyBitmap[1] = {0};
const unsigned char* calendar_allArray[11] = {dummyBitmap, dummyBitmap, dummyBitmap, dummyBitmap, dummyBitmap, dummyBitmap, dummyBitmap, dummyBitmap, dummyBitmap, dummyBitmap, dummyBitmap};
// Mock FreeSerifBold9pt7b and FreeSerif9pt7b
struct DummyFont {};
DummyFont FreeSerifBold9pt7b;
DummyFont FreeSerif9pt7b;
// Mock _eventMarker0 and _eventMarker1
const unsigned char _eventMarker0[1] = {0};
const unsigned char _eventMarker1[1] = {0};
// Patch DateTime to support operator- if not already defined
struct DateTime {
  int _year, _month, _day;
  DateTime(int y, int m, int d) : _year(y), _month(m), _day(d) {}
  int year() const { return _year; }
  int month() const { return _month; }
  int day() const { return _day; }
  int dayOfTheWeek() const { return (_year + _month + _day) % 7; }
  struct Diff {
    int _days;
    Diff(int d) : _days(d) {}
    int days() const { return _days; }
  };
  Diff operator-(const DateTime& other) const {
    int days1 = _year * 365 + _month * 30 + _day;
    int days2 = other._year * 365 + other._month * 30 + other._day;
    return Diff(days1 - days2);
  }
};
struct MockRTC { DateTime now() { return DateTime(2025, 1, 15); } };
MockRTC rtc;
#endif
#define NATIVE_TEST
#include <unity.h>
#include "../include/globals.h"
// Define all global variables required by CALENDAR.cpp
// extern bool SAVE_POWER;
// extern int POWER_SAVE_FREQ;
// extern bool SDActive;
// extern bool DEBUG_VERBOSE;
// extern bool forceSlowFullUpdate;
// extern bool newState;
// extern int OLED_MAX_FPS;
// extern AppState CurrentAppState;
// Define all global variables and mocks required by CALENDAR.cpp (copied from test_tasks)
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

// Remove all externs for variables and functions that are defined here
// Directly define all timing variables, hardware mocks, and file helpers

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
void setCpuFrequencyMhz(int) {}
void delay(int) {}
void refresh() {}
void oledWord(const String& word) { std::cout << "OLED: " << word << std::endl; }
void oledLine(const String& line, bool) { std::cout << "OLED Line: " << line << std::endl; }
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
  mockTime += 100;
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

// Provide definitions for native test build
#ifdef NATIVE_TEST
#define EVENTS_FILE "sys/events.txt"
#else
#define EVENTS_FILE "/sys/events.txt"
#endif

// Include only the core CALENDAR functions (not display functions)
#include "../src/CALENDAR.cpp"

// Black-box E2E Test: Focus on functionality, not internal state
void test_e2e_user_calendar_flow() {
  std::cout << "\n=== BLACK-BOX E2E: User Calendar Flow ===" << std::endl;

  // Ensure sys/ directory exists for file operations
  #ifdef NATIVE_TEST
  system("mkdir -p sys");
  // Create empty events file if it does not exist
  std::ofstream f(EVENTS_FILE, std::ios::app); f.close();
  #endif

  // === SETUP: Clean slate ===
  delFile(EVENTS_FILE);
  CALENDAR_INIT();

  // === USER STORY 1: Add multiple events ===
  std::cout << "Adding events..." << std::endl;
  addEvent("Doctor Appt", "20250115", "0900", "60", "", "Annual checkup");
  addEvent("Project Due", "20250120", "2359", "0", "", "Final report");
  addEvent("Birthday", "20250110", "0000", "0", "YEARLY 0110", "Cake!");

  // Verify events are sorted by date
  TEST_ASSERT_EQUAL(3, calendarEvents.size());
  TEST_ASSERT_EQUAL_STRING("Birthday", calendarEvents[0][0].c_str());
  TEST_ASSERT_EQUAL_STRING("Doctor Appt", calendarEvents[1][0].c_str());
  TEST_ASSERT_EQUAL_STRING("Project Due", calendarEvents[2][0].c_str());

  // === USER STORY 2: File persistence ===
  std::cout << "Testing file persistence..." << std::endl;
  std::vector<std::vector<String>> eventsBeforeRestart = calendarEvents;
  calendarEvents.clear();
  TEST_ASSERT_EQUAL(0, calendarEvents.size());
  updateEventArray();
  TEST_ASSERT_EQUAL(eventsBeforeRestart.size(), calendarEvents.size());
  for (size_t i = 0; i < eventsBeforeRestart.size(); i++) {
    TEST_ASSERT_EQUAL_STRING(eventsBeforeRestart[i][0].c_str(), calendarEvents[i][0].c_str());
    TEST_ASSERT_EQUAL_STRING(eventsBeforeRestart[i][1].c_str(), calendarEvents[i][1].c_str());
  }

  // === USER STORY 3: Delete event ===
  std::cout << "Testing event deletion..." << std::endl;
  deleteEvent(1); // Remove "Doctor Appt"
  updateEventsFile();
  TEST_ASSERT_EQUAL(2, calendarEvents.size());
  TEST_ASSERT_EQUAL_STRING("Birthday", calendarEvents[0][0].c_str());
  TEST_ASSERT_EQUAL_STRING("Project Due", calendarEvents[1][0].c_str());

  // === USER STORY 4: Navigation flow ===
  std::cout << "Testing navigation..." << std::endl;
  // Simulate user pressing HOME from calendar
  simulateKeyPress(12); // HOME key
  resetTimingVars();
  processKB_CALENDAR();
  TEST_ASSERT_EQUAL(HOME, CurrentAppState);

  // Re-enter calendar app
  CALENDAR_INIT();
  TEST_ASSERT_EQUAL(CALENDAR, CurrentAppState);
  TEST_ASSERT_EQUAL(MONTH, CurrentCalendarState);

  // === USER STORY 5: Add event with repeat and check ===
  std::cout << "Testing repeat event logic..." << std::endl;
  addEvent("Yoga Class", "20250117", "1800", "60", "WEEKLY Fr", "Stretch");
  int count = 0;
  for (const auto& ev : calendarEvents) if (ev[0] == "Yoga Class") count++;
  TEST_ASSERT_EQUAL(1, count);

  // === CLEANUP ===
  delFile(EVENTS_FILE);
  calendarEvents.clear();
  std::cout << "=== BLACK-BOX E2E: User Calendar Flow PASSED! ===" << std::endl;
}

// Add Unity test runner
void setUp(void) {
  // Reset state before each test if needed
}

void tearDown(void) {
  // Clean up after each test if needed
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_e2e_user_calendar_flow);
  return UNITY_END();
}