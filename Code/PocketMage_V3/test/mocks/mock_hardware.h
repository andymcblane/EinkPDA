#ifndef MOCK_HARDWARE_H
#define MOCK_HARDWARE_H

#include <vector>
#include <map>
#include <string>
#include <algorithm>
#include <cstdint>
#include <cstring>

// Arduino compatibility layer for native testing
class MockString {
public:
    std::string data;
    
    MockString() {}
    MockString(const char* str) : data(str) {}
    MockString(const std::string& str) : data(str) {}
    
    const char* c_str() const { return data.c_str(); }
    size_t length() const { return data.length(); }
    int indexOf(char c, int start = 0) const {
        size_t pos = data.find(c, start);
        return (pos == std::string::npos) ? -1 : (int)pos;
    }
    MockString substring(int start, int end = -1) const {
        if (end == -1) end = data.length();
        return MockString(data.substr(start, end - start));
    }
    bool startsWith(const char* prefix) const {
        return data.substr(0, strlen(prefix)) == prefix;
    }
    MockString operator+(const MockString& other) const {
        return MockString(data + other.data);
    }
    MockString operator+(const char* other) const {
        return MockString(data + other);
    }
    bool operator==(const char* other) const {
        return data == other;
    }
    bool operator<(const MockString& other) const {
        return data < other.data;
    }
    MockString& operator=(const MockString& other) {
        data = other.data;
        return *this;
    }
    MockString& operator=(const char* other) {
        data = other;
        return *this;
    }
};

// Global operator for const char* + MockString
inline MockString operator+(const char* lhs, const MockString& rhs) {
    return MockString(std::string(lhs) + rhs.data);
}

typedef MockString String;
typedef uint8_t byte;

// Mock Arduino functions (declarations only)
unsigned long millis();
void delay(unsigned long ms);
void setCpuFrequencyMhz(uint32_t cpu_freq_mhz);

// Mock File class
class MockFile {
public:
    bool available() { return false; }
    String readStringUntil(char terminator) { return ""; }
    void close() {}
    bool print(const char* data) { return true; }
    bool println(const char* data) { return true; }
    operator bool() { return true; }
};

// Mock SD_MMC class
class MockSD_MMC {
public:
    std::map<std::string, std::string> files;
    
    MockFile open(const char* path, const char* mode) {
        return MockFile();
    }
    
    bool exists(const char* path) {
        return files.find(path) != files.end();
    }
    
    bool mkdir(const char* path) { return true; }
    bool remove(const char* path) { 
        files.erase(path);
        return true; 
    }
    
    void setFileContent(const char* path, const char* content) {
        files[path] = content;
    }
    
    std::string getFileContent(const char* path) {
        auto it = files.find(path);
        return it != files.end() ? it->second : "";
    }
};

// Mock Keypad
class MockKeypad {
public:
    void disableInterrupts() {}
    void enableInterrupts() {}
    void flush() {}
};

// Global mock instances
extern MockSD_MMC SD_MMC;
extern MockKeypad keypad;

// Mock display functions (declarations only)
void oledWord(String word, bool allowLarge = false, bool showInfo = true);
void oledLine(String line, bool doProgressBar = true);

// Mock serial
class MockSerial {
public:
    void println(const char* text) { (void)text; }
    void print(const char* text) { (void)text; }
};

extern MockSerial Serial;

// Mock global variables
extern bool DEBUG_VERBOSE;
extern bool SAVE_POWER;
extern int POWER_SAVE_FREQ;
extern bool SDActive;

#endif // MOCK_HARDWARE_H 