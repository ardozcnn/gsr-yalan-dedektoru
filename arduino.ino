const int PIN_GSR = A0;
const int PIN_G   = D1;
const int PIN_B   = D2;
const int PIN_R   = D3;

const int SAMPLE_COUNT = 10;
const int SAMPLE_DELAY_MS = 3;

const float FAST_ALPHA = 0.45f;
const float SLOW_ALPHA = 0.02f;

const float PCT_BLUE = 0.04f;
const float PCT_RED  = 0.08f;

const unsigned long CALIB_MS = 4000;
const unsigned long LED_HOLD_MS = 200;
const int NO_FINGER_ADC = 3500;

float threshBlue = 8.0f;
float threshRed  = 16.0f;
float fastV = 0;
float slowV = 0;
bool calibrated = false;

enum LedState { LED_OFF, LED_GREEN, LED_BLUE, LED_RED, LED_WHITE };
LedState currentLed = LED_OFF;
LedState pendingLed = LED_OFF;
unsigned long pendingSince = 0;

void setLed(LedState state) {
  bool r = false, g = false, b = false;
  switch (state) {
    case LED_GREEN: r = false; g = true;  b = false; break;
    case LED_BLUE:  r = false; g = false; b = true;  break;
    case LED_RED:   r = true;  g = false; b = false; break;
    case LED_WHITE: r = true;  g = true;  b = true;  break;
    default: break;
  }
  digitalWrite(PIN_R, r ? HIGH : LOW);
  digitalWrite(PIN_G, g ? HIGH : LOW);
  digitalWrite(PIN_B, b ? HIGH : LOW);
  currentLed = state;
}

int readGsrRaw() {
  long sum = 0;
  for (int i = 0; i < SAMPLE_COUNT; i++) {
    sum += analogRead(PIN_GSR);
    delay(SAMPLE_DELAY_MS);
  }
  return (int)(sum / SAMPLE_COUNT);
}

bool fingersPresent(int adc) {
  return adc > 40 && adc < NO_FINGER_ADC;
}

void applyLedDebounced(LedState wanted) {
  unsigned long now = millis();
  if (wanted != pendingLed) {
    pendingLed = wanted;
    pendingSince = now;
    return;
  }
  if (wanted != currentLed && (now - pendingSince) >= LED_HOLD_MS) {
    setLed(wanted);
  }
}

void computeThresholds(float level, float stdv) {
  float fromPctBlue = level * PCT_BLUE;
  float fromPctRed  = level * PCT_RED;
  float fromStdBlue = stdv * 1.6f + 4.0f;
  float fromStdRed  = stdv * 3.2f + 9.0f;

  threshBlue = max(fromPctBlue, fromStdBlue);
  threshRed  = max(fromPctRed, fromStdRed);

  threshBlue = constrain(threshBlue, 5.0f, 40.0f);
  threshRed  = constrain(threshRed, 10.0f, 80.0f);
  if (threshRed < threshBlue + 5.0f) threshRed = threshBlue + 5.0f;
}

bool calibrate() {
  setLed(LED_WHITE);
  Serial.println(F("=== KALIBRASYON 4sn ==="));

  int raw = readGsrRaw();
  if (!fingersPresent(raw)) {
    setLed(LED_BLUE);
    return false;
  }
  fastV = slowV = (float)raw;

  double sum = 0, sumSq = 0;
  int n = 0;
  unsigned long t0 = millis();

  while (millis() - t0 < CALIB_MS) {
    raw = readGsrRaw();
    if (!fingersPresent(raw)) {
      setLed(LED_BLUE);
      return false;
    }
    fastV = fastV * (1.0f - FAST_ALPHA) + (float)raw * FAST_ALPHA;
    slowV = slowV * 0.8f + fastV * 0.2f;
    sum += fastV;
    sumSq += (double)fastV * fastV;
    n++;
    Serial.print(F("kal "));
    Serial.println(raw);
  }

  if (n < 12) return false;

  float mean = (float)(sum / n);
  float var = (float)(sumSq / n - (double)mean * mean);
  if (var < 0) var = 0;
  float stdv = sqrt(var);

  slowV = mean;
  fastV = mean;
  computeThresholds(mean, stdv);
  calibrated = true;

  Serial.print(F("OK mean="));
  Serial.print(mean, 0);
  Serial.print(F(" std="));
  Serial.print(stdv, 1);
  Serial.print(F(" mavi>="));
  Serial.print(threshBlue, 1);
  Serial.print(F(" kirmizi>="));
  Serial.println(threshRed, 1);

  setLed(LED_GREEN);
  pendingLed = LED_GREEN;
  pendingSince = millis();
  return true;
}

LedState decide(float drop) {
  if (drop >= threshRed) return LED_RED;
  if (drop >= threshBlue) return LED_BLUE;
  return LED_GREEN;
}

void setup() {
  Serial.begin(115200);
  delay(400);
  pinMode(PIN_R, OUTPUT);
  pinMode(PIN_G, OUTPUT);
  pinMode(PIN_B, OUTPUT);
  setLed(LED_OFF);
  analogReadResolution(12);
  analogSetPinAttenuation(PIN_GSR, ADC_11db);
  Serial.println(F("GSR v5 hassas"));
  calibrated = false;
}

void loop() {
  int raw = readGsrRaw();

  if (!fingersPresent(raw)) {
    Serial.println(F("parmak yok"));
    setLed(LED_BLUE);
    calibrated = false;
    delay(80);
    return;
  }

  if (!calibrated) {
    delay(150);
    calibrate();
    return;
  }

  fastV = fastV * (1.0f - FAST_ALPHA) + (float)raw * FAST_ALPHA;
  float drop = slowV - fastV;

  if (drop < threshBlue * 0.5f) {
    slowV = slowV * (1.0f - SLOW_ALPHA) + fastV * SLOW_ALPHA;
    drop = slowV - fastV;
    computeThresholds(slowV, threshBlue * 0.4f);
  }

  applyLedDebounced(decide(drop));

  Serial.print(F("raw="));
  Serial.print(raw);
  Serial.print(F(" drop="));
  Serial.print(drop, 1);
  Serial.print(F(" /"));
  Serial.print(threshBlue, 0);
  Serial.print(F("|"));
  Serial.print(threshRed, 0);
  Serial.print(F(" "));
  if (currentLed == LED_GREEN) Serial.println(F("YESIL"));
  else if (currentLed == LED_BLUE) Serial.println(F("MAVI"));
  else if (currentLed == LED_RED) Serial.println(F("KIRMIZI"));
  else Serial.println();
}
