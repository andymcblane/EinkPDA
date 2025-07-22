#ifdef UNIT_TEST

#include "mocks/mock_hardware.h"
#include <vector>

// Mock global instances
MockSD_MMC SD_MMC;
MockKeypad keypad;
MockSerial Serial;

// Mock global variables
bool DEBUG_VERBOSE = false;
bool SAVE_POWER = false;
int POWER_SAVE_FREQ = 40;
bool SDActive = false;

// Tasks-related globals
std::vector<std::vector<String>> tasks;
uint8_t selectedTask = 0;

// Mock Arduino function implementations
unsigned long millis() { return 1000; }
void delay(unsigned long ms) { (void)ms; }
void setCpuFrequencyMhz(uint32_t cpu_freq_mhz) { (void)cpu_freq_mhz; }

// Mock display function implementations
void oledWord(String word, bool allowLarge, bool showInfo) {
    (void)word; (void)allowLarge; (void)showInfo;
}

void oledLine(String line, bool doProgressBar) {
    (void)line; (void)doProgressBar;
}

// File operations mocks
void appendToFile(String path, String inText) {
    if (!path.startsWith("/")) path = "/" + path;
    std::string existing = SD_MMC.getFileContent(path.c_str());
    existing += inText.c_str();
    existing += "\n";
    SD_MMC.setFileContent(path.c_str(), existing.c_str());
}

void delFile(String fileName) {
    if (!fileName.startsWith("/")) fileName = "/" + fileName;
    SD_MMC.remove(fileName.c_str());
}

// File system functions for testing
void writeFile(MockSD_MMC &fs, const char *path, const char *message) {
    fs.setFileContent(path, message);
}

String readFileToString(MockSD_MMC &fs, const char *path) {
    std::string content = fs.getFileContent(path);
    return String(content.c_str());
}

void deleteFile(MockSD_MMC &fs, const char *path) {
    fs.remove(path);
}

#endif // UNIT_TEST 