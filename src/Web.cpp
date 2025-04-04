#include "Web.h"
#include <WiFi.h>
#include <AsyncTCP.h>
#include "ESPAsyncWebServer.h"
#include <WiFi.h>
#include "main.h"
#include <ESPUI.h>
#include <Arduino.h>
#include "LightUtils.h"
#include "utilities/PreferencesManager.h"
#include "freertos/semphr.h"
#include <Preferences.h>
#include <ArduinoJson.h>

// Global game control variables
bool GAME_ENABLED = true;
bool SEQUENCE_LOCAL_ECHO = true;

// Mutex for API operations (kept for compatibility)
SemaphoreHandle_t apiMutex = NULL;

// The following API-related functions are kept for compatibility but are no longer used
// since the API endpoints have been removed

// API response helpers (unused)
void sendJsonResponse(AsyncWebServerRequest *request, JsonDocument &doc) {
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    serializeJson(doc, *response);
    request->send(response);
}

void sendErrorResponse(AsyncWebServerRequest *request, int code, const String &message) {
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    JsonDocument doc;
    doc.to<JsonObject>(); // Initialize as object
    doc["success"] = false;
    doc["error"] = message;
    serializeJson(doc, *response);
    request->send(response);
}

// API route handlers (unused)
void handleStatusRequest(AsyncWebServerRequest *request) {
    if (xSemaphoreTake(apiMutex, (TickType_t)1000) != pdTRUE) {
        sendErrorResponse(request, 503, "Server busy");
        return;
    }

    JsonDocument doc;
    doc.to<JsonObject>();
    doc["success"] = true;

    // System status
    doc["status"]["uptime"] = millis();
    doc["status"]["emergency_stop"] = false;

    // Lighting settings
    doc["lighting"]["brightness"] = lightUtils->getCfgBrightness();
    doc["lighting"]["program"] = lightUtils->getCfgProgram();
    doc["lighting"]["sin"] = lightUtils->getCfgSin();
    doc["lighting"]["updates"] = lightUtils->getCfgUpdates();
    doc["lighting"]["reverse"] = lightUtils->getCfgReverse() != 0;
    doc["lighting"]["fire"] = lightUtils->getCfgFire() != 0;
    doc["lighting"]["local_disable"] = lightUtils->getCfgLocalDisable() != 0;
    doc["lighting"]["auto"] = lightUtils->getCfgAuto() != 0;
    doc["lighting"]["auto_time"] = lightUtils->getCfgAutoTime();
    doc["lighting"]["reverse_second_row"] = lightUtils->getCfgReverseSecondRow() != 0;

    sendJsonResponse(request, doc);
    xSemaphoreGive(apiMutex);
}

void handleLightingCommand(AsyncWebServerRequest *request, const JsonVariant &json) {
    if (xSemaphoreTake(apiMutex, (TickType_t)500) != pdTRUE) {
        sendErrorResponse(request, 503, "Server busy");
        return;
    }

    JsonObject jsonObj = json.as<JsonObject>();
    JsonDocument response;
    response.to<JsonObject>();
    response["success"] = true;
    bool updated = false;

    if (jsonObj["brightness"].is<int>()) {
        int value = jsonObj["brightness"].as<int>();
        if (value >= 0 && value <= 255) {
            lightUtils->setCfgBrightness(value);
            updated = true;
            response["brightness"] = value;
        }
    }

    if (jsonObj["program"].is<int>()) {
        int value = jsonObj["program"].as<int>();
        if (value >= 1 && value <= 50) {
            lightUtils->setCfgProgram(value);
            updated = true;
            response["program"] = value;
        }
    }

    if (jsonObj["sin"].is<int>()) {
        int value = jsonObj["sin"].as<int>();
        if (value >= 0 && value <= 32) {
            lightUtils->setCfgSin(value);
            updated = true;
            response["sin"] = value;
        }
    }

    if (jsonObj["updates"].is<int>()) {
        int value = jsonObj["updates"].as<int>();
        if (value >= 1 && value <= 255) {
            lightUtils->setCfgUpdates(value);
            updated = true;
            response["updates"] = value;
        }
    }

    if (jsonObj["reverse"].is<bool>()) {
        bool value = jsonObj["reverse"].as<bool>();
        lightUtils->setCfgReverse(value ? 1 : 0);
        updated = true;
        response["reverse"] = value;
    }

    if (jsonObj["fire"].is<bool>()) {
        bool value = jsonObj["fire"].as<bool>();
        lightUtils->setCfgFire(value ? 1 : 0);
        updated = true;
        response["fire"] = value;
    }

    if (jsonObj["local_disable"].is<bool>()) {
        bool value = jsonObj["local_disable"].as<bool>();
        lightUtils->setCfgLocalDisable(value ? 1 : 0);
        updated = true;
        response["local_disable"] = value;
    }

    if (jsonObj["auto"].is<bool>()) {
        bool value = jsonObj["auto"].as<bool>();
        lightUtils->setCfgAuto(value ? 1 : 0);
        updated = true;
        response["auto"] = value;
    }

    if (jsonObj["auto_time"].is<int>()) {
        int value = jsonObj["auto_time"].as<int>();
        if (value >= 1 && value <= 3600) {
            lightUtils->setCfgAutoTime(value);
            updated = true;
            response["auto_time"] = value;
        }
    }

    if (jsonObj["reverse_second_row"].is<bool>()) {
        bool value = jsonObj["reverse_second_row"].as<bool>();
        lightUtils->setCfgReverseSecondRow(value ? 1 : 0);
        updated = true;
        response["reverse_second_row"] = value;
    }

    if (updated) {
        response["message"] = "Lighting settings updated";
    } else {
        response["message"] = "No settings changed";
    }

    AsyncResponseStream *resp = request->beginResponseStream("application/json");
    serializeJson(response, *resp);
    request->send(resp);

    xSemaphoreGive(apiMutex);
}

void handleSystemCommand(AsyncWebServerRequest *request, const JsonVariant &json) {
    if (xSemaphoreTake(apiMutex, (TickType_t)500) != pdTRUE) {
        sendErrorResponse(request, 503, "Server busy");
        return;
    }

    JsonObject jsonObj = json.as<JsonObject>();
    JsonDocument response;
    response.to<JsonObject>();
    response["success"] = true;

    if (jsonObj["reboot"].is<bool>() && jsonObj["reboot"].as<bool>()) {
        response["message"] = "Rebooting system...";

        // Send response before rebooting
        AsyncResponseStream *resp = request->beginResponseStream("application/json");
        serializeJson(response, *resp);
        request->send(resp);

        xSemaphoreGive(apiMutex);

        // Delay to allow response to be sent
        delay(500);
        ESP.restart();
        return;
    }

    if (jsonObj["game_enabled"].is<bool>()) {
        bool enabled = jsonObj["game_enabled"].as<bool>();
        GAME_ENABLED = enabled;
        response["game_enabled"] = enabled;
        response["message"] = enabled ? "Game enabled" : "Game disabled";
    }
    else {
        response["success"] = false;
        response["error"] = "No valid command specified";
    }

    AsyncResponseStream *resp = request->beginResponseStream("application/json");
    serializeJson(response, *resp);
    request->send(resp);

    xSemaphoreGive(apiMutex);
}

// UI control variables
uint16_t status;
uint16_t controlMillis;
uint16_t networkInfo;
uint16_t resetConfigSwitch;
uint16_t resetRebootSwitch;

// Lighting control variables
uint16_t lightingBrightnessSlider;
uint16_t lightingProgramSelect;
uint16_t lightingSinSlider;
uint16_t lightingUpdatesSlider;
uint16_t lightingReverseSwitch;
uint16_t lightingFireSwitch;
uint16_t lightingLocalDisable;
uint16_t lightingAuto;
uint16_t lightingAutoTime;
uint16_t lightingReverseSecondRow;

// Fog control variables have been removed

// Manual control variables
uint16_t starManualSelect;
int starManualSelectValue = 0;

void slider(Control *sender, int type)
{
    Serial.print("Slider: ID: ");
    Serial.print(sender->id);
    Serial.print(", Value: ");
    Serial.println(sender->value);

    if (sender->id == lightingBrightnessSlider)
    {
        lightUtils->setCfgBrightness(sender->value.toInt());
    }
    else if (sender->id == lightingSinSlider)
    {
        lightUtils->setCfgSin(sender->value.toInt());
    }
    else if (sender->id == lightingUpdatesSlider)
    {
        lightUtils->setCfgUpdates(sender->value.toInt());
    }
    else if (sender->id == lightingAutoTime)
    {
        lightUtils->setCfgAutoTime(sender->value.toInt());
    }
    // Fog-related callbacks have been removed
    else
    {
        Serial.println("Unknown slider");
    }
}

void buttonCallback(Control *sender, int type)
{
    // All button callbacks related to Star and StarSequence have been removed
}

void switchExample(Control *sender, int value)
{
    if (sender->id == lightingReverseSwitch)
    {
        lightUtils->setCfgReverse(sender->value.toInt());
    }
    else if (sender->id == lightingFireSwitch)
    {
        lightUtils->setCfgFire(sender->value.toInt());
    }
    else if (sender->id == lightingLocalDisable)
    {
        lightUtils->setCfgLocalDisable(sender->value.toInt());
    }
    else if (sender->id == lightingAuto)
    {
        lightUtils->setCfgAuto(sender->value.toInt());
    }
    // Add handler for reverse second row toggle
    else if (sender->id == lightingReverseSecondRow)
    {
        lightUtils->setCfgReverseSecondRow(sender->value.toInt());
    }
    else if (sender->id == resetConfigSwitch)
    {
        // TODO:
        //    - Give the user a chance to cancel the config reset.
        if (sender->value.toInt())
        {
            // PreferencesManager::clear();
            // PreferencesManager::save();
            delay(50);
            ESP.restart();
        }
    }
    else if (sender->id == resetRebootSwitch)
    {
        if (sender->value.toInt())
        {
            // Todo:
            //    - Give the user a chance to cancel the reboot.
            Serial.println("Rebooting device from web switch...");
            delay(50);
            ESP.restart();
        }
    }
    else
    {
        Serial.println("Unknown Switch");
    }

    switch (value)
    {
    case S_ACTIVE:
        Serial.print("Active:");
        break;

    case S_INACTIVE:
        Serial.print("Inactive");
        break;
    }

    Serial.print(" ");
    Serial.println(sender->id);
}

void selectExample(Control *sender, int value)
{
    Serial.print("Select: ID: ");
    Serial.print(sender->id);
    Serial.print(", Value: ");
    Serial.println(sender->value);

    if (sender->id == lightingProgramSelect)
    {
        lightUtils->setCfgProgram(sender->value.toInt());
    }
}

void webSetup()
{
    // Add tabs
    uint16_t mainTab = ESPUI.addControl(ControlType::Tab, "Main", "Main");
    uint16_t lightingTab = ESPUI.addControl(ControlType::Tab, "Lighting", "Lighting");
    uint16_t sysInfoTab = ESPUI.addControl(ControlType::Tab, "System Info", "System Info");
    uint16_t resetTab = ESPUI.addControl(ControlType::Tab, "Reset", "Reset");

    // Add status label above all tabs
    status = ESPUI.addControl(ControlType::Label, "Status:", "Unknown Status", ControlColor::Turquoise);

    // Move uptime to System Info tab
    controlMillis = ESPUI.addControl(ControlType::Label, "Uptime", "0", ControlColor::Emerald, sysInfoTab);
    networkInfo = ESPUI.addControl(ControlType::Label, "Network Info", "", ControlColor::Emerald, sysInfoTab);

    //--- Lighting Tab ---
    lightingBrightnessSlider = ESPUI.addControl(ControlType::Slider, "Brightness", String(lightUtils->getCfgBrightness()), ControlColor::Alizarin, lightingTab, &slider);
    ESPUI.addControl(Min, "", "0", None, lightingBrightnessSlider);
    ESPUI.addControl(Max, "", "255", None, lightingBrightnessSlider);

    lightingProgramSelect = ESPUI.addControl(ControlType::Select, "Program", String(lightUtils->getCfgProgram()), ControlColor::Alizarin, lightingTab, &selectExample);
    ESPUI.addControl(ControlType::Option, "Rainbow", "1", ControlColor::Alizarin, lightingProgramSelect);
    ESPUI.addControl(ControlType::Option, "Rainbow With Glitter", "2", ControlColor::Alizarin, lightingProgramSelect);
    ESPUI.addControl(ControlType::Option, "Confetti", "3", ControlColor::Alizarin, lightingProgramSelect);
    ESPUI.addControl(ControlType::Option, "Sinelon", "4", ControlColor::Alizarin, lightingProgramSelect);
    ESPUI.addControl(ControlType::Option, "Juggle", "5", ControlColor::Alizarin, lightingProgramSelect);
    ESPUI.addControl(ControlType::Option, "BPM", "6", ControlColor::Alizarin, lightingProgramSelect);
    ESPUI.addControl(ControlType::Option, "Fire2012", "7", ControlColor::Alizarin, lightingProgramSelect);
    ESPUI.addControl(ControlType::Option, "Fire2012 With Palette", "8", ControlColor::Alizarin, lightingProgramSelect);
    ESPUI.addControl(ControlType::Option, "Solid Rainbow", "9", ControlColor::Alizarin, lightingProgramSelect);
    ESPUI.addControl(ControlType::Option, "Cloud", "10", ControlColor::Alizarin, lightingProgramSelect);
    ESPUI.addControl(ControlType::Option, "Lava", "11", ControlColor::Alizarin, lightingProgramSelect);
    ESPUI.addControl(ControlType::Option, "Ocean", "12", ControlColor::Alizarin, lightingProgramSelect);
    ESPUI.addControl(ControlType::Option, "Forest", "13", ControlColor::Alizarin, lightingProgramSelect);
    ESPUI.addControl(ControlType::Option, "Rainbow Stripes", "14", ControlColor::Alizarin, lightingProgramSelect);
    ESPUI.addControl(ControlType::Option, "Party", "15", ControlColor::Alizarin, lightingProgramSelect);
    ESPUI.addControl(ControlType::Option, "Heat", "16", ControlColor::Alizarin, lightingProgramSelect);
    ESPUI.addControl(ControlType::Option, "CloudColors_p", "17", ControlColor::Alizarin, lightingProgramSelect);
    ESPUI.addControl(ControlType::Option, "OceanColors_p", "18", ControlColor::Alizarin, lightingProgramSelect);
    ESPUI.addControl(ControlType::Option, "ForestColors_p", "19", ControlColor::Alizarin, lightingProgramSelect);
    ESPUI.addControl(ControlType::Option, "All White", "20", ControlColor::Alizarin, lightingProgramSelect);
    ESPUI.addControl(ControlType::Option, "All Red", "21", ControlColor::Alizarin, lightingProgramSelect);
    ESPUI.addControl(ControlType::Option, "All Green", "22", ControlColor::Alizarin, lightingProgramSelect);
    ESPUI.addControl(ControlType::Option, "All Blue", "23", ControlColor::Alizarin, lightingProgramSelect);
    ESPUI.addControl(ControlType::Option, "All Purple", "24", ControlColor::Alizarin, lightingProgramSelect);
    ESPUI.addControl(ControlType::Option, "All Cyan", "25", ControlColor::Alizarin, lightingProgramSelect);
    ESPUI.addControl(ControlType::Option, "All Yellow", "26", ControlColor::Alizarin, lightingProgramSelect);
    ESPUI.addControl(ControlType::Option, "White Dot", "50", ControlColor::Alizarin, lightingProgramSelect);

    lightingUpdatesSlider = ESPUI.addControl(ControlType::Slider, "Updates Per Second", String(lightUtils->getCfgUpdates()), ControlColor::Alizarin, lightingTab, &slider);
    ESPUI.addControl(Min, "", "1", None, lightingUpdatesSlider);
    ESPUI.addControl(Max, "", "255", None, lightingUpdatesSlider);

    lightingSinSlider = ESPUI.addControl(ControlType::Slider, "Sin", String(lightUtils->getCfgSin()), ControlColor::Alizarin, lightingTab, &slider);
    ESPUI.addControl(Min, "", "0", None, lightingSinSlider);
    ESPUI.addControl(Max, "", "32", None, lightingSinSlider);

    lightingReverseSwitch = ESPUI.addControl(ControlType::Switcher, "Reverse", String(lightUtils->getCfgReverse()), ControlColor::Alizarin, lightingTab, &switchExample);
    lightingFireSwitch = ESPUI.addControl(ControlType::Switcher, "Fire", String(lightUtils->getCfgFire()), ControlColor::Alizarin, lightingTab, &switchExample);
    lightingLocalDisable = ESPUI.addControl(ControlType::Switcher, "Local Disable", String(lightUtils->getCfgLocalDisable()), ControlColor::Alizarin, lightingTab, &switchExample);

    lightingAuto = ESPUI.addControl(ControlType::Switcher, "Auto Light Program Selection", String(lightUtils->getCfgAuto()), ControlColor::Alizarin, lightingTab, &switchExample);
    lightingAutoTime = ESPUI.addControl(ControlType::Slider, "Auto Time", String(lightUtils->getCfgAutoTime() ? lightUtils->getCfgAutoTime() : 30), ControlColor::Alizarin, lightingAuto, &slider);
    ESPUI.addControl(Min, "", "1", None, lightingAutoTime);
    ESPUI.addControl(Max, "", "3600", None, lightingAutoTime);

    // Add reverse second row toggle
    lightingReverseSecondRow = ESPUI.addControl(ControlType::Switcher, "Reverse Second Row", String(lightUtils->getCfgReverseSecondRow()), ControlColor::Alizarin, lightingTab, &switchExample);

    // System Info Tab

    // Reset tab
    ESPUI.addControl(ControlType::Label, "**WARNING**", "Don't even think of doing anything in this tab unless you want to break something!!", ControlColor::Sunflower, resetTab);
    resetConfigSwitch = ESPUI.addControl(ControlType::Switcher, "Reset Configurations", "0", ControlColor::Sunflower, resetTab, &switchExample);
    resetRebootSwitch = ESPUI.addControl(ControlType::Switcher, "Reboot", "0", ControlColor::Sunflower, resetTab, &switchExample);

    // Enable this option if you want sliders to be continuous (update during move) and not discrete (update on stop)
    // ESPUI.sliderContinuous = true;

    // Optionally use HTTP BasicAuth
    // ESPUI.server->addHandler(new CaptiveRequestHandler()).setFilter(ON_AP_FILTER); // only when requested from AP
    // ESPUI->server->begin();

    ESPUI.captivePortal = false; // Disable captive portal

    // ESPUI.list(); // List all files on LittleFS, for info
    ESPUI.begin("NOVA Core");

    // API endpoints have been removed
    // Create API mutex (kept for compatibility with handler functions)
    apiMutex = xSemaphoreCreateMutex();
}

ControlColor getColorForName(const char *colorName)
{
    if (!colorName)
        return ControlColor::Dark;

    String color = String(colorName);
    color.toLowerCase();

    if (color == "red")
        return ControlColor::Alizarin; // Red
    if (color == "green")
        return ControlColor::Emerald; // Green
    if (color == "blue")
        return ControlColor::Peterriver; // Blue
    if (color == "yellow")
        return ControlColor::Sunflower; // Yellow
    if (color == "white")
        return ControlColor::Wetasphalt; // White represented as gray

    return ControlColor::Dark;
}

void webLoop()
{
    // Initialize static variables
    static unsigned long oldTime = 0;
    static bool switchState = false;
    static const unsigned long UPDATE_INTERVAL = 1000; // Update every 1 second
    static bool isUpdating = false;                    // Guard against recursion
    static SemaphoreHandle_t webMutex = xSemaphoreCreateMutex();

    // Rate limit updates and prevent recursion
    unsigned long currentMillis = millis();
    if (currentMillis - oldTime < UPDATE_INTERVAL || isUpdating)
    {
        return;
    }

    // Try to take mutex with timeout
    if (xSemaphoreTake(webMutex, (TickType_t)100) != pdTRUE)
    {
        return; // Couldn't get mutex, skip this update
    }

    isUpdating = true;

    // Wrap UI updates in try-catch
    try
    {
        oldTime = currentMillis;

        // Use stack-based string for time formatting
        char timeStr[32];
        unsigned long seconds = (currentMillis / 1000) % 60;
        unsigned long minutes = (currentMillis / (1000 * 60)) % 60;
        unsigned long hours = (currentMillis / (1000 * 60 * 60)) % 24;
        unsigned long days = (currentMillis / (1000 * 60 * 60 * 24));
        snprintf(timeStr, sizeof(timeStr), "%lud %luh %lum %lus", days, hours, minutes, seconds);

        // Update uptime display 
        if (controlMillis && ESPUI.getControl(controlMillis)) {
            ESPUI.updateControlValue(controlMillis, timeStr);
        }

        // Update network info display
        if (networkInfo && ESPUI.getControl(networkInfo)) {
            String networkStatus = "MAC: " + WiFi.macAddress() + "<br>";
            networkStatus += "AP IP: " + WiFi.softAPIP().toString() + "<br>";
            networkStatus += "Client IP: " + WiFi.localIP().toString();
            ESPUI.updateControlValue(networkInfo, networkStatus);
        }

        // Update status message
        Control* statusControl = status ? ESPUI.getControl(status) : nullptr;
        if (statusControl) {
            const char* statusMsg;
            ControlColor statusColor = ControlColor::Emerald;

            ESPUI.updateControlValue(status, statusMsg);
            statusControl->color = statusColor;
            ESPUI.updateControl(statusControl);
        }
    }
    catch (...) {
        Serial.println("Error occurred during webLoop update");
    }

    isUpdating = false;
    xSemaphoreGive(webMutex);
}