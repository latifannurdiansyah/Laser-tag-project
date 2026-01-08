#include <Arduino.h>
#include <IRremote.hpp>

// =============================
// Configuration
// =============================
#define IR_SEND_PIN     2
#define BUTTON_PIN      5
#define STATUS_LED      LED_BUILTIN
#define SERIAL_BAUD     115200

#define TASK_PRIORITY_BUTTON    1
#define TASK_PRIORITY_SERIAL    1
#define TASK_PRIORITY_IR        2

#define TASK_STACK_SIZE_BUTTON  2048
#define TASK_STACK_SIZE_SERIAL  3072
#define TASK_STACK_SIZE_IR      2048

// =============================
// Data Structure
// =============================
struct IRCommand {
  uint32_t address;
  uint8_t  command;
  uint8_t  repeat;
};

// =============================
// Global Shared Resources
// =============================
IRCommand lastIRCommand = {0x04, 0x04, 1}; // Default + shared
SemaphoreHandle_t xCommandMutex = nullptr;   // Protects lastIRCommand
QueueHandle_t irSendQueue = nullptr;         // Queue to trigger send

// =============================
// Task Declarations
// =============================
void taskIRSender(void *pvParameters);
void taskSerialCommand(void *pvParameters);
void taskButtonPress(void *pvParameters);

// =============================
// Setup
// =============================
void setup() {
  Serial.begin(SERIAL_BAUD);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(STATUS_LED, OUTPUT);
  pinMode(IR_SEND_PIN, OUTPUT);

  IrSender.begin(IR_SEND_PIN);

  // Create mutex and queue
  xCommandMutex = xSemaphoreCreateMutex();
  irSendQueue = xQueueCreate(10, sizeof(bool)); // Just a trigger

  if (xCommandMutex == nullptr || irSendQueue == nullptr) {
    Serial.println("❌ Failed to create RTOS resources!");
    while (1) delay(10);
  }

  // Start tasks
  xTaskCreate(taskButtonPress,   "Button", TASK_STACK_SIZE_BUTTON,   NULL, TASK_PRIORITY_BUTTON,  NULL);
  xTaskCreate(taskSerialCommand, "Serial", TASK_STACK_SIZE_SERIAL,   NULL, TASK_PRIORITY_SERIAL,  NULL);
  xTaskCreate(taskIRSender,      "IRSend", TASK_STACK_SIZE_IR,       NULL, TASK_PRIORITY_IR,      NULL);

  Serial.println("✅ SIMPERA TEST IR TX & RX");
  Serial.println("👤 Indrazno Siradjuddin");
  Serial.println("📅 2025");
  Serial.println("System Ready.");
  Serial.printf("💡 Default IR: Addr=0x%04X, Cmd=0x%02X\n", lastIRCommand.address, lastIRCommand.command);
  Serial.println("👉 Send 'F' or 'F,<addr>,<cmd>' via serial");
  Serial.println("🖐️  Press button to send LAST received command");
}

void loop() {
  delay(1000);
}

// =============================
// Task: IR Sender
// =============================
void taskIRSender(void *pvParameters) {
  bool trigger;
  (void) pvParameters;

  for (;;) {
    // Wait for send trigger
    if (xQueueReceive(irSendQueue, &trigger, portMAX_DELAY) == pdTRUE) {
      // Acquire mutex to read last command
      if (xSemaphoreTake(xCommandMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        IRCommand cmd = lastIRCommand; // Copy
        xSemaphoreGive(xCommandMutex);

        // Feedback
        digitalWrite(STATUS_LED, HIGH);
        IrSender.sendNEC(cmd.address, cmd.command, cmd.repeat);
        vTaskDelay(pdMS_TO_TICKS(150));
        digitalWrite(STATUS_LED, LOW);
        digitalWrite(IR_SEND_PIN, LOW);

        Serial.printf("✅ IR Sent: NEC Addr=0x%04X, Cmd=0x%02X, Repeat=%d\n",
                      cmd.address, cmd.command, cmd.repeat);
      } else {
        Serial.println("❌ Failed to acquire command lock!");
      }
    }
  }
}

// =============================
// Task: Serial Command Parser
// =============================
void taskSerialCommand(void *pvParameters) {
  (void) pvParameters;

  for (;;) {
    if (Serial.available()) {
      String input = Serial.readStringUntil('\n');
      input.trim();

      if (input.startsWith("F") || input.startsWith("f")) {
        IRCommand cmd = lastIRCommand; // Start with current

        int comma1 = input.indexOf(',');
        int comma2 = input.indexOf(',', comma1 + 1);

        if (comma1 != -1 && comma2 != -1) {
          String addrStr = input.substring(comma1 + 1, comma2);
          String cmdStr  = input.substring(comma2 + 1);

          addrStr.trim(); cmdStr.trim();

          cmd.address = strtoul(addrStr.c_str(), NULL, 0);
          cmd.command = strtoul(cmdStr.c_str(),  NULL, 0);
          cmd.repeat = 1;

          // Update shared last command
          if (xSemaphoreTake(xCommandMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            lastIRCommand = cmd;
            xSemaphoreGive(xCommandMutex);

            Serial.printf("📩 Updated last command: F, 0x%04X, 0x%02X\n",
                          cmd.address, cmd.command);
          } else {
            Serial.println("❌ Failed to update command (lock timeout)");
          }

        } else {
          Serial.println("📩 Triggering last stored command via 'F'");
        }

        // Trigger IR send immediately
        bool sendTrigger = true;
        if (xQueueSendToBack(irSendQueue, &sendTrigger, pdMS_TO_TICKS(100)) != pdTRUE) {
          Serial.println("❌ Send queue full!");
        }
      }
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

// =============================
// Task: Button Press
// =============================
void taskButtonPress(void *pvParameters) {
  bool buttonPressed = false;
  bool sendTrigger = true;
  (void) pvParameters;

  for (;;) {
    if (digitalRead(BUTTON_PIN) == LOW && !buttonPressed) {
      buttonPressed = true;

      Serial.println("🖐️  Button Pressed: Sending LAST user command");

      if (xQueueSendToBack(irSendQueue, &sendTrigger, pdMS_TO_TICKS(100)) != pdTRUE) {
        Serial.println("❌ Send queue full! Button ignored.");
      }

      // Cooldown
      vTaskDelay(pdMS_TO_TICKS(500));
    }
    if (digitalRead(BUTTON_PIN) == HIGH) {
      buttonPressed = false;
    }

    vTaskDelay(pdMS_TO_TICKS(20));
  }
}