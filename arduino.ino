const int ledYeşil = D1;
const int ledMavi = D2;
const int ledKırmızı = D3;
const int gsrPin = A0;

float skinConductivity = 0.0;
float tolerance = 0.0;

void setup()
{
  Serial.begin(115200);
  pinMode(ledYeşil, OUTPUT);
  pinMode(ledMavi, OUTPUT);
  pinMode(ledKırmızı, OUTPUT);

  digitalWrite(ledYeşil, HIGH);
  digitalWrite(ledMavi, HIGH);
  digitalWrite(ledKırmızı, HIGH);

  long sum = 0;
  const int samples = 250;

  for (int i = 0; i < samples; i++)
  {
    sum += analogRead(gsrPin);
    delay(20);
  }

  skinConductivity = sum / (float)samples;
  tolerance = skinConductivity * 0.10f;

  digitalWrite(ledYeşil, LOW);
  digitalWrite(ledMavi, LOW);
  digitalWrite(ledKırmızı, LOW);
}

void loop()
{
  int value = analogRead(gsrPin);

  if (value == 0 || value > 3800) {
    digitalWrite(ledYeşil, LOW);
    digitalWrite(ledMavi, LOW);
    digitalWrite(ledKırmızı, LOW);
    delay(50);
    return;
  }

  Serial.print(skinConductivity);
  Serial.print('\t');
  Serial.print(skinConductivity + tolerance);
  Serial.print('\t');
  Serial.print(skinConductivity + 2 * tolerance);
  Serial.print('\t');
  Serial.println(value);

  digitalWrite(ledYeşil, LOW);
  digitalWrite(ledMavi, LOW);
  digitalWrite(ledKırmızı, LOW);

  if (value > skinConductivity + 2 * tolerance)
  {
    digitalWrite(ledKırmızı, HIGH);
  }
  else if (value > skinConductivity + tolerance)
  {
    digitalWrite(ledMavi, HIGH);
  }
  else
  {
    digitalWrite(ledYeşil, HIGH);
  }

  delay(20);
}
