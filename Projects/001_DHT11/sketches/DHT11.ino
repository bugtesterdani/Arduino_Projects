
#define DHT11_DataPIN 2

void setup()
{
	Serial.begin(115200);
	DHTbegin(DHT11_DataPIN);
}

void loop()
{
	delay(500);
	
	float h = readHumidity();
	float t = ReadTemperature();
	if (isnan(h) || isnan(t))
	{
		Serial.print("Error Reading from DHT11");
		return;
	}
	
	Serial.print("Humidity: ");
	Serial.print(h, 2);
	Serial.println("%");
	
	Serial.print("Temperature: ");
	Serial.print(t, 2);
	Serial.println("C");
}


uint8_t pullTime;
uint8_t _pin;
uint8_t data[5];
uint32_t _lastreadtime, _maxcycles;
bool _lastresult;

#define MIN_INTERVAL 2000
#define TIMEOUT UINT32_MAX

void DHTbegin(uint8_t pin)
{
	(void)6;
	_pin = pin;
	_maxcycles = microsecondsToClockCycles(1000);
	
	pinMode(_pin, INPUT_PULLUP);
	_lastreadtime = millis() - MIN_INTERVAL;
	pullTime = 55;
}

float ReadTemperature()
{
	float f = NAN;
	if (read())
	{
		f = data[2];
		if (data[3] & 0x80)
		{
			f = -1 - f;
		}
		f += (data[3] & 0x0f) * 0.1;
	}
	return f;
}

float readHumidity()
{
	float f = NAN;
	if (read())
	{
		f = data[0] + data[1] * 0.1;
	}
	return f;
}

bool read()
{
	uint32_t currentTime = millis();
	if ((currentTime - _lastreadtime) < MIN_INTERVAL)
		return _lastresult;
	_lastreadtime = currentTime;
	data[0] = data[1] = data[2] = data[3] = data[4] = 0;
	pinMode(_pin, INPUT_PULLUP);
	delay(1);
	pinMode(_pin, OUTPUT);
	digitalWrite(_pin, LOW);
	delay(20);
	uint32_t cycles[80];
	{
		pinMode(_pin, INPUT_PULLUP);
		delayMicroseconds(pullTime);
		if (expectPulse(LOW) == TIMEOUT)
		{
			_lastresult = false;
			return _lastresult;
		}
		if (expectPulse(HIGH) == TIMEOUT)
		{
			_lastresult = false;
			return _lastresult;
		}
		
		for (int i = 0; i < 80; i += 2)
		{
			cycles[i] = expectPulse(LOW);
			cycles[i + 1] = expectPulse(HIGH);
		}
	}
	for (int i = 0; i < 40; ++i)
	{
		uint32_t lowCycles = cycles[2 * i];
		uint32_t highCycles = cycles[2 * i + 1];
		if ((lowCycles == TIMEOUT) || (highCycles == TIMEOUT))
		{
			_lastresult = false;
			return _lastresult;
		}
		data[i / 8] <<= 1;
		if (highCycles > lowCycles)
			data[i / 8] |= 1;
	}
	if (data[4] == ((data[0] + data[1] + data[2] + data[3]) & 0xFF))
	{
		_lastresult = true;
		return _lastresult;
	}
	else
	{
		_lastresult = false;
		return _lastresult;
	}
}

uint32_t expectPulse(bool level)
{
	uint32_t count = 0;
	while (digitalRead(_pin) == level)
	{
		if (count++ >= _maxcycles)
		{
			return TIMEOUT;
		}
	}
	return count;
}