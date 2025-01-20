  #include "VOneMqttClient.h"
  #include "DHT.h"

  // Device IDs. Please Replace This with your Device ID
  const char* MQ2sensor = "9af3ef09-608b-4988-a531-f0b9ab6536be";
  const char* DHT11Sensor = "80961513-f19e-4b51-9b6f-7ecb3044560c";
  const char* RainSensor = "a2fcdba0-4379-47b1-adca-5b6436bbd16a";

  // Pins for each sensors and actuators
  const int mq2Pin = A2;          // MQ2 pin
  const int dht11Pin = 42;        // DHT11 pin
  const int rainPin = A4;          // Rain sensor pin
  const int redLedPin = 47;       // Red LED represent Smoke and Fire detector 
  const int yellowLedPin = 48;    // Yellow LED represent Temperature
  const int greenLedPin = A11;     // Green LED represent Raining status
  int rainValue;

  // DHT11 sensor setup
  #define DHTTYPE DHT11
  DHT dht(dht11Pin, DHTTYPE);

  // VOne MQTT client
  VOneMqttClient voneClient;

  // Setup Thresholds
  const int smokeThreshold = 4000;
  const int tempThreshold = 34;

  // Timer
  unsigned long lastRedLedTime = 0;
  unsigned long lastYellowLedTime = 0;
  unsigned long lastMsgTime = 0;

  // LED durations
  const unsigned long redLedDuration = 60000;   // 1 minute
  const unsigned long yellowLedDuration = 60000; // 1 minute

  // LED states
  bool redLedActive = false;
  bool yellowLedActive = false;

  // WiFi setup
  void setup_wifi() {
    delay(10);
    Serial.println();
    Serial.print("Connecting to Wi-Fi...");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
  }

  // Setup
  void setup() {
    setup_wifi();
    voneClient.setup();

    dht.begin();
    pinMode(redLedPin, OUTPUT);
    pinMode(yellowLedPin, OUTPUT);
    pinMode(greenLedPin, OUTPUT);

    digitalWrite(redLedPin, LOW);
    digitalWrite(yellowLedPin, LOW);
    analogWrite(greenLedPin, LOW);
  }

  // Function to round to 2 decimal places
  float roundToTwoDecimalPlaces(float value) {
    return round(value * 100.0) / 100.0;
  }

  // Loop
  void loop() 
  {
    if (!voneClient.connected()) 
    {
      voneClient.reconnect();
    }
    voneClient.loop();

    unsigned long curTime = millis();

    // Read sensors
    float gasValue = analogRead(mq2Pin);
    float humidity = dht.readHumidity();
    int temperature = dht.readTemperature();
    rainValue = analogRead(rainPin);

    // Restrict decimal places to 2
    gasValue = roundToTwoDecimalPlaces(gasValue);   // Gas value rounded
    humidity = roundToTwoDecimalPlaces(humidity);   // Humidity rounded
    temperature = roundToTwoDecimalPlaces(temperature);   // Temperature rounded

    // Publish sensor data

    if (curTime - lastMsgTime > 10000 ) 
    { 
      // Publish every 10 seconds
      lastMsgTime = curTime;

      // Publish Data for MQ2
      voneClient.publishTelemetryData(MQ2sensor, "Gas detector", gasValue);

      // Publish Data for DHT11
      JSONVar dhtPayload;
      dhtPayload["Humidity"] = humidity;
      dhtPayload["Temperature"] = temperature;
      voneClient.publishTelemetryData(DHT11Sensor, dhtPayload);

      // Publish Data forRain Sensor
      voneClient.publishTelemetryData(RainSensor, "Raining", rainValue);
    }

    // Red LED - Lit up if gasValue > smokeThreshold
    if (gasValue > smokeThreshold) 
    {
      if (!redLedActive) 
      {
        redLedActive = true;
        lastRedLedTime = curTime;
        digitalWrite(redLedPin, HIGH);
      } 
      else if (curTime - lastRedLedTime >= redLedDuration) 
      {
        redLedActive = false;
        digitalWrite(redLedPin, LOW);
      }
    }

    // Yellow LED - Light up for 1 minute if temperature > tempThreshold
    if (temperature > tempThreshold) 
    {
      if (!yellowLedActive) 
      {
        yellowLedActive = true;
        lastYellowLedTime = curTime;
        digitalWrite(yellowLedPin, HIGH);
      } 
      else if (curTime - lastYellowLedTime >= yellowLedDuration)
      {
        yellowLedActive = false;
        digitalWrite(yellowLedPin, LOW);
      }
    }

    if (rainValue < 2000) 
    { // LOW means rain detected for most rain sensors
      analogWrite(greenLedPin, 255); // Full brightness for PWM
    
    } 
    else 
    {
      analogWrite(greenLedPin, 0); // Turn off the LED
    
    }
  }

