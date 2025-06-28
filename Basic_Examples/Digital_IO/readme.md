# ESP32 Digital Input/Output - Detailed Explanation

## The Original Code
```cpp
// Connect LED to GPIO18, button to GPIO19 (with pull-up)
const int ledPin = 18;
const int buttonPin = 19;

void setup() {
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP);  // Internal pull-up resistor
}

void loop() {
  int buttonState = digitalRead(buttonPin);
  
  if (buttonState == LOW) {  // Button pressed (pulled to ground)
    digitalWrite(ledPin, HIGH);
    Serial.println("Button pressed - LED ON");
  } else {
    digitalWrite(ledPin, LOW);
    Serial.println("Button released - LED OFF");
  }
  
  delay(100);
}
```

---

## Hardware Setup

### What You'll Need:
- **ESP32 WROOM-32**
- **1 LED** (any color)
- **1 Push button** (momentary switch)
- **1 Resistor** (220Ω for LED)
- **Breadboard**
- **Jumper wires**

### Wiring Diagram:

```
ESP32 Pin    Component           Connection
---------    ---------           ----------
GPIO18   →   LED (+) leg     →   220Ω resistor → GND
GPIO19   →   Button (one leg)
GND      →   Button (other leg)
3.3V     →   Not needed (using internal pull-up)
```

### Step-by-Step Wiring:

1. **LED Connection:**
   - Connect LED's **long leg (anode/+)** to GPIO18
   - Connect LED's **short leg (cathode/-)** to one end of 220Ω resistor
   - Connect other end of resistor to **GND** on ESP32

2. **Button Connection:**
   - Connect one leg of button to **GPIO19**
   - Connect other leg of button to **GND**
   - **No external resistor needed** (we use internal pull-up)

---

## Code Breakdown

### 1. Pin Definitions
```cpp
const int ledPin = 18;
const int buttonPin = 19;
```
- **const int**: Makes these values unchangeable (good practice)
- **GPIO18**: Digital output pin for LED
- **GPIO19**: Digital input pin for button
- **Why these pins?** They're general-purpose GPIO pins, safe to use

### 2. Setup Function
```cpp
void setup() {
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP);
}
```

**Serial.begin(115200):**
- Starts serial communication at 115,200 baud rate
- Allows us to see debug messages in Serial Monitor

**pinMode(ledPin, OUTPUT):**
- Configures GPIO18 as an output pin
- Can now send HIGH (3.3V) or LOW (0V) signals

**pinMode(buttonPin, INPUT_PULLUP):**
- Configures GPIO19 as input with **internal pull-up resistor**
- **Pull-up resistor**: Weak resistor (about 45kΩ) that pulls pin to 3.3V
- **Why needed?** Prevents "floating" input when button isn't pressed

### 3. Understanding Pull-up Resistors

**Without Pull-up (BAD):**
```
Button Released: Pin reads random values (floating)
Button Pressed:  Pin reads LOW (0V)
```

**With Pull-up (GOOD):**
```
Button Released: Pin reads HIGH (3.3V) - pulled up by resistor
Button Pressed:  Pin reads LOW (0V) - connected to ground
```

### 4. Main Loop
```cpp
void loop() {
  int buttonState = digitalRead(buttonPin);
  
  if (buttonState == LOW) {
    digitalWrite(ledPin, HIGH);
    Serial.println("Button pressed - LED ON");
  } else {
    digitalWrite(ledPin, LOW);
    Serial.println("Button released - LED OFF");
  }
  
  delay(100);
}
```

**digitalRead(buttonPin):**
- Reads digital state of GPIO19
- Returns HIGH (1) or LOW (0)
- With pull-up: HIGH = not pressed, LOW = pressed

**if (buttonState == LOW):**
- Button pressed = pin pulled to ground = LOW
- **Counter-intuitive!** With pull-up, pressed = LOW

**digitalWrite(ledPin, HIGH/LOW):**
- HIGH: Sends 3.3V to LED → LED turns ON
- LOW: Sends 0V to LED → LED turns OFF

**delay(100):**
- Wait 100 milliseconds before next reading
- Prevents overwhelming the serial output

---

## Alternative Wiring Methods

### Method 1: External Pull-up Resistor
```cpp
// If you want to use external pull-up instead of internal
pinMode(buttonPin, INPUT);  // No internal pull-up
```

**Wiring:**
```
3.3V → 10kΩ resistor → GPIO19 → Button → GND
```

### Method 2: Pull-down Configuration
```cpp
pinMode(buttonPin, INPUT_PULLDOWN);  // ESP32 also has pull-down
```

**Logic changes:**
```cpp
if (buttonState == HIGH) {  // Now HIGH = pressed
  // Button pressed
}
```

**Wiring:**
```
3.3V → Button → GPIO19
(Internal pull-down pulls to GND when not pressed)
```

---

## Common Issues & Solutions

### Issue 1: LED Doesn't Light Up
**Possible causes:**
- LED connected backwards (swap legs)
- No current-limiting resistor (LED might burn out)
- Wrong GPIO pin selected
- Loose connections

**Debug steps:**
```cpp
// Test LED independently
void setup() {
  pinMode(18, OUTPUT);
}

void loop() {
  digitalWrite(18, HIGH);
  delay(1000);
  digitalWrite(18, LOW);
  delay(1000);
}
```

### Issue 2: Button Reading is Erratic
**Possible causes:**
- No pull-up resistor (floating input)
- Button bouncing (mechanical switches bounce)
- Loose connections

**Solution - Debouncing:**
```cpp
int buttonState = 0;
int lastButtonState = 0;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;

void loop() {
  int reading = digitalRead(buttonPin);
  
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }
  
  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != buttonState) {
      buttonState = reading;
      
      if (buttonState == LOW) {
        digitalWrite(ledPin, HIGH);
      } else {
        digitalWrite(ledPin, LOW);
      }
    }
  }
  
  lastButtonState = reading;
}
```

### Issue 3: Serial Monitor Shows Nothing
**Solutions:**
- Check baud rate is set to 115200 in Serial Monitor
- Make sure correct COM port is selected
- Try unplugging/reconnecting USB cable

---

## Variations to Try

### 1. Toggle LED (Press to Toggle On/Off)
```cpp
bool ledState = false;
bool lastButtonState = HIGH;

void loop() {
  bool currentButtonState = digitalRead(buttonPin);
  
  if (lastButtonState == HIGH && currentButtonState == LOW) {
    // Button just pressed
    ledState = !ledState;  // Toggle state
    digitalWrite(ledPin, ledState);
  }
  
  lastButtonState = currentButtonState;
  delay(50);
}
```

### 2. Multiple Buttons and LEDs
```cpp
const int led1Pin = 18;
const int led2Pin = 19;
const int button1Pin = 21;
const int button2Pin = 22;

void setup() {
  pinMode(led1Pin, OUTPUT);
  pinMode(led2Pin, OUTPUT);
  pinMode(button1Pin, INPUT_PULLUP);
  pinMode(button2Pin, INPUT_PULLUP);
}

void loop() {
  digitalWrite(led1Pin, !digitalRead(button1Pin));
  digitalWrite(led2Pin, !digitalRead(button2Pin));
  delay(50);
}
```

### 3. Button Counter
```cpp
int buttonPresses = 0;
bool lastButtonState = HIGH;

void loop() {
  bool currentButtonState = digitalRead(buttonPin);
  
  if (lastButtonState == HIGH && currentButtonState == LOW) {
    buttonPresses++;
    Serial.print("Button pressed ");
    Serial.print(buttonPresses);
    Serial.println(" times");
  }
  
  lastButtonState = currentButtonState;
  delay(50);
}
```

---

## Key Concepts Learned

1. **Digital I/O**: Reading and writing digital signals (HIGH/LOW)
2. **Pull-up Resistors**: Preventing floating inputs
3. **Active-Low Logic**: Button pressed = LOW signal
4. **pinMode Configuration**: Setting pins as INPUT/OUTPUT
5. **Serial Debugging**: Using Serial.println() for troubleshooting
6. **Basic Control Flow**: if/else statements for decision making

This example is fundamental to most ESP32 projects - you'll use these concepts in sensors, displays, motors, and communication modules!