#ifdef UNIT_TEST

#include <unity.h>
#include "mocks/mock_hardware.h"
#include <vector>

// External globals needed by TASKS functions
extern std::vector<std::vector<String>> tasks;
extern MockSD_MMC SD_MMC;
extern bool SDActive;
extern bool SAVE_POWER;
extern int POWER_SAVE_FREQ;

// Include the specific functions we want to test
void sortTasksByDueDate(std::vector<std::vector<String>> &tasks) {
  std::sort(tasks.begin(), tasks.end(), [](const std::vector<String> &a, const std::vector<String> &b) {
    return a[1] < b[1]; // Compare dueDate strings
  });
}

String convertDateFormat(String yyyymmdd) {
  if (yyyymmdd.length() != 8) {
    return String("Invalid");
  }

  String year = yyyymmdd.substring(2, 4);  // Get last two digits of the year
  String month = yyyymmdd.substring(4, 6);
  String day = yyyymmdd.substring(6, 8);

  return month + "/" + day + "/" + year;
}

void deleteTask(int index) {
  if (index >= 0 && index < tasks.size()) {
    tasks.erase(tasks.begin() + index);
  }
}

void updateTasksFile() {
  SDActive = true;
  
  // Clear the existing tasks file first
  SD_MMC.remove("/sys/tasks.txt");

  // Iterate through the tasks vector and append each task to the file
  std::string fileContent = "";
  for (size_t i = 0; i < tasks.size(); i++) {
    // Create a string from the task's attributes with "|" delimiter
    String taskInfo = tasks[i][0] + "|" + tasks[i][1] + "|" + tasks[i][2] + "|" + tasks[i][3];
    fileContent += taskInfo.c_str();
    if (i < tasks.size() - 1) fileContent += "\n";
  }
  
  SD_MMC.setFileContent("/sys/tasks.txt", fileContent.c_str());
  SDActive = false;
}

void addTask(String taskName, String dueDate, String priority, String completed) {
  tasks.push_back({taskName, dueDate, priority, completed});
  sortTasksByDueDate(tasks);
  updateTasksFile();
}

void updateTaskArray() {
  SDActive = true;
  
  std::string fileContent = SD_MMC.getFileContent("/sys/tasks.txt");
  tasks.clear(); // Clear the existing vector before loading the new data

  if (fileContent.empty()) {
    SDActive = false;
    return;
  }

  // Split content by lines
  size_t start = 0;
  size_t end = fileContent.find('\n');
  
  while (start < fileContent.length()) {
    std::string line;
    if (end != std::string::npos) {
      line = fileContent.substr(start, end - start);
      start = end + 1;
      end = fileContent.find('\n', start);
    } else {
      line = fileContent.substr(start);
      start = fileContent.length();
    }
    
    if (line.empty()) continue;
    
    String lineStr = String(line.c_str());
    
    // Split the line into individual parts using the delimiter '|'
    uint8_t delimiterPos1 = lineStr.indexOf('|');
    uint8_t delimiterPos2 = lineStr.indexOf('|', delimiterPos1 + 1);
    uint8_t delimiterPos3 = lineStr.indexOf('|', delimiterPos2 + 1);

    // Extract task name, due date, priority, and completed status
    String taskName  = lineStr.substring(0, delimiterPos1);
    String dueDate   = lineStr.substring(delimiterPos1 + 1, delimiterPos2);
    String priority  = lineStr.substring(delimiterPos2 + 1, delimiterPos3);
    String completed = lineStr.substring(delimiterPos3 + 1);

    // Add the task to the vector
    tasks.push_back({taskName, dueDate, priority, completed});
  }

  SDActive = false;
}

// Test setup and teardown
void setUp(void) {
    tasks.clear();
    SD_MMC.files.clear();
}

void tearDown(void) {
    tasks.clear();
    SD_MMC.files.clear();
}

// Test convertDateFormat function
void test_convertDateFormat_valid_date() {
    String result = convertDateFormat("20250115");
    TEST_ASSERT_EQUAL_STRING("01/15/25", result.c_str());
}

void test_convertDateFormat_another_valid_date() {
    String result = convertDateFormat("20231225");
    TEST_ASSERT_EQUAL_STRING("12/25/23", result.c_str());
}

void test_convertDateFormat_invalid_length() {
    String result = convertDateFormat("2025011");
    TEST_ASSERT_EQUAL_STRING("Invalid", result.c_str());
}

void test_convertDateFormat_empty_string() {
    String result = convertDateFormat("");
    TEST_ASSERT_EQUAL_STRING("Invalid", result.c_str());
}

void test_convertDateFormat_too_long() {
    String result = convertDateFormat("202501159");
    TEST_ASSERT_EQUAL_STRING("Invalid", result.c_str());
}

// Test sortTasksByDueDate function
void test_sortTasksByDueDate_single_task() {
    tasks.push_back({"Task 1", "20250115", "1", "0"});
    sortTasksByDueDate(tasks);
    
    TEST_ASSERT_EQUAL(1, tasks.size());
    TEST_ASSERT_EQUAL_STRING("Task 1", tasks[0][0].c_str());
}

void test_sortTasksByDueDate_multiple_tasks_already_sorted() {
    tasks.push_back({"Task A", "20250115", "1", "0"});
    tasks.push_back({"Task B", "20250120", "1", "0"});
    sortTasksByDueDate(tasks);
    
    TEST_ASSERT_EQUAL(2, tasks.size());
    TEST_ASSERT_EQUAL_STRING("Task A", tasks[0][0].c_str());
    TEST_ASSERT_EQUAL_STRING("Task B", tasks[1][0].c_str());
}

void test_sortTasksByDueDate_multiple_tasks_reverse_order() {
    tasks.push_back({"Task B", "20250120", "1", "0"});
    tasks.push_back({"Task A", "20250115", "1", "0"});
    sortTasksByDueDate(tasks);
    
    TEST_ASSERT_EQUAL(2, tasks.size());
    TEST_ASSERT_EQUAL_STRING("Task A", tasks[0][0].c_str());
    TEST_ASSERT_EQUAL_STRING("Task B", tasks[1][0].c_str());
}

void test_sortTasksByDueDate_three_tasks_mixed_order() {
    tasks.push_back({"Task B", "20250120", "1", "0"});
    tasks.push_back({"Task A", "20250115", "1", "0"});
    tasks.push_back({"Task C", "20250125", "1", "0"});
    sortTasksByDueDate(tasks);
    
    TEST_ASSERT_EQUAL(3, tasks.size());
    TEST_ASSERT_EQUAL_STRING("Task A", tasks[0][0].c_str());
    TEST_ASSERT_EQUAL_STRING("Task B", tasks[1][0].c_str());
    TEST_ASSERT_EQUAL_STRING("Task C", tasks[2][0].c_str());
}

// Test deleteTask function
void test_deleteTask_valid_index() {
    tasks.push_back({"Task 1", "20250115", "1", "0"});
    tasks.push_back({"Task 2", "20250120", "1", "0"});
    
    deleteTask(0);
    
    TEST_ASSERT_EQUAL(1, tasks.size());
    TEST_ASSERT_EQUAL_STRING("Task 2", tasks[0][0].c_str());
}

void test_deleteTask_invalid_negative_index() {
    tasks.push_back({"Task 1", "20250115", "1", "0"});
    
    deleteTask(-1);
    
    TEST_ASSERT_EQUAL(1, tasks.size());
    TEST_ASSERT_EQUAL_STRING("Task 1", tasks[0][0].c_str());
}

void test_deleteTask_invalid_large_index() {
    tasks.push_back({"Task 1", "20250115", "1", "0"});
    
    deleteTask(5);
    
    TEST_ASSERT_EQUAL(1, tasks.size());
    TEST_ASSERT_EQUAL_STRING("Task 1", tasks[0][0].c_str());
}

void test_deleteTask_empty_vector() {
    deleteTask(0);
    TEST_ASSERT_EQUAL(0, tasks.size());
}

// Test addTask function
void test_addTask_single_task() {
    addTask("Test Task", "20250115", "1", "0");
    
    TEST_ASSERT_EQUAL(1, tasks.size());
    TEST_ASSERT_EQUAL_STRING("Test Task", tasks[0][0].c_str());
    TEST_ASSERT_EQUAL_STRING("20250115", tasks[0][1].c_str());
    TEST_ASSERT_EQUAL_STRING("1", tasks[0][2].c_str());
    TEST_ASSERT_EQUAL_STRING("0", tasks[0][3].c_str());
}

void test_addTask_multiple_tasks_auto_sort() {
    addTask("Task B", "20250120", "1", "0");
    addTask("Task A", "20250115", "1", "0");
    
    TEST_ASSERT_EQUAL(2, tasks.size());
    TEST_ASSERT_EQUAL_STRING("Task A", tasks[0][0].c_str());
    TEST_ASSERT_EQUAL_STRING("Task B", tasks[1][0].c_str());
}

// Test file persistence
void test_updateTasksFile_and_updateTaskArray() {
    // Add tasks
    addTask("Task 1", "20250115", "1", "0");
    addTask("Task 2", "20250120", "2", "1");
    
    // Clear tasks vector to simulate restart
    tasks.clear();
    
    // Load tasks from "file"
    updateTaskArray();
    
    TEST_ASSERT_EQUAL(2, tasks.size());
    TEST_ASSERT_EQUAL_STRING("Task 1", tasks[0][0].c_str());
    TEST_ASSERT_EQUAL_STRING("20250115", tasks[0][1].c_str());
    TEST_ASSERT_EQUAL_STRING("Task 2", tasks[1][0].c_str());
    TEST_ASSERT_EQUAL_STRING("20250120", tasks[1][1].c_str());
}

void test_updateTaskArray_empty_file() {
    updateTaskArray();
    TEST_ASSERT_EQUAL(0, tasks.size());
}

// Main test runner
int main() {
    UNITY_BEGIN();
    
    // Date conversion tests
    RUN_TEST(test_convertDateFormat_valid_date);
    RUN_TEST(test_convertDateFormat_another_valid_date);
    RUN_TEST(test_convertDateFormat_invalid_length);
    RUN_TEST(test_convertDateFormat_empty_string);
    RUN_TEST(test_convertDateFormat_too_long);
    
    // Sorting tests
    RUN_TEST(test_sortTasksByDueDate_single_task);
    RUN_TEST(test_sortTasksByDueDate_multiple_tasks_already_sorted);
    RUN_TEST(test_sortTasksByDueDate_multiple_tasks_reverse_order);
    RUN_TEST(test_sortTasksByDueDate_three_tasks_mixed_order);
    
    // Delete tests
    RUN_TEST(test_deleteTask_valid_index);
    RUN_TEST(test_deleteTask_invalid_negative_index);
    RUN_TEST(test_deleteTask_invalid_large_index);
    RUN_TEST(test_deleteTask_empty_vector);
    
    // Add task tests
    RUN_TEST(test_addTask_single_task);
    RUN_TEST(test_addTask_multiple_tasks_auto_sort);
    
    // File persistence tests
    RUN_TEST(test_updateTasksFile_and_updateTaskArray);
    RUN_TEST(test_updateTaskArray_empty_file);
    
    return UNITY_END();
}

#endif // UNIT_TEST 