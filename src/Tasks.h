#ifndef TASKS_H
#define TASKS_H

#include <Arduino.h>

// Task function declarations
void taskSetup();

void TaskLightUtils(void *pvParameters);
// TaskEnable has been removed
void TaskModes(void *pvParameters);
void TaskWeb(void *pvParameters);
void TaskI2CMonitor(void *pvParameters);
void TaskMonitor(void *pvParameters);
void TaskWiFiConnection(void *pvParameters);
void TaskScreen(void *pvParameters);  // New screen task
void gameTask(void *pvParameters);
void taskButton(void *pvParameters);

// Task monitoring functions
void updateTaskStats(const char* name, UBaseType_t watermark, BaseType_t coreId);
void registerTaskForMonitoring(const char* name, UBaseType_t watermark, BaseType_t coreId, UBaseType_t initialStackSize);

#endif // TASKS_H