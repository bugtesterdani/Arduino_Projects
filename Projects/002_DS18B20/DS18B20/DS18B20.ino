/*
 Name:		DS18B20.ino
 Created:	30.11.2021 19:57:52
 Author:	Daniel
*/

#include <OneWire.h>

#define ONE_WIRE_BUS 2

OneWire oneWire(ONE_WIRE_BUS);


OneWire* wire;

bool parasite;
bool waitForConversion;
bool checkForConversion;
bool autoSaveScratchPad;
bool useExternalPullup;
uint8_t devices;
uint8_t ds18Count;
uint8_t bitResolution;
uint8_t pullupPin;

typedef uint8_t ScratchPad[9];
typedef uint8_t DeviceAddress[8];

// the setup function runs once when you press reset or power the board
void setup() {
	Serial.begin(9600);
	setOneWire();
	begin();
}

// the loop function runs over and over again until power down or reset
void loop() {
	Serial.println("Requesting temperatures...");
	requestTemperature();
	Serial.println("DONE");
	float tempC = getTemperatureIndex(0);
	if (tempC != -127)
	{
		Serial.print("Temperature for the device 1 (index 0) is: ");
		Serial.println(tempC);
	}
	else {
		Serial.println("Error: Could not read temperature data");
	}
}

void setOneWire()
{
	wire = &oneWire;
	devices = 0;
	ds18Count = 0;
	parasite = false;
	bitResolution = 9;
	waitForConversion = true;
	checkForConversion = true;
	autoSaveScratchPad = true;
}

void begin()
{
	DeviceAddress deviceAddress;
	wire->reset_search();
	devices = 0;
	ds18Count = 0;

	while (wire->search(deviceAddress))
	{
		if (validAddress(deviceAddress))
		{
			devices++;
			if (validFamility(deviceAddress))
			{
				ds18Count++;
				if (!parasite && readPowerSupply(deviceAddress))
				{
					parasite = true;
				}
				uint8_t b = getResolution(deviceAddress);
				if (b > bitResolution) bitResolution = b;
			}
		}
	}
}

bool validAddress(const uint8_t* dev)
{
	return (wire->crc8(dev, 7) == dev[7]);
}

bool validFamility(const uint8_t* dev)
{
	switch (dev[0])
	{
	case 0x10:
	case 0x28:
	case 0x22:
	case 0x3B:
	case 0x42:
		return true;
	default:
		return false;
	}
}

bool readPowerSupply(const uint8_t* dev)
{
	wire->reset();
	if (dev == nullptr)
		wire->skip();
	else
		wire->select(dev);

	wire->write(0xB4);
	if (wire->read_bit() == 0)
		return reset_return(true);
	else
		return reset_return(false);
}

uint8_t getResolution(const uint8_t* dev)
{
	if (dev[0] == 0x10)
		return 12;
	ScratchPad scratchPad;
	if (isConnected(dev, scratchPad))
	{
		switch (scratchPad[4])
		{
		case 0x7F:
			return 12;
		case 0x5F:
			return 11;
		case 0x3F:
			return 10;
		case 0x1F:
			return 9;
		default:
			break;
		}
	}
	return 0;
}

bool isConnected(const uint8_t* dev, uint8_t* sP)
{
	return readScratchPad(dev, sP) && !isAllZero(sP) && (wire->crc8(sP, 8) == sP[8]);
}

bool readScratchPad(const uint8_t* dev, uint8_t* sP)
{
	if (wire->reset() == 0)
	{
		return false;
	}
	wire->select(dev);
	wire->write(0xBE);

	for (uint8_t i = 0; i < 9; i++)
	{
		sP[i] = wire->read();
	}
	return (wire->reset() == 1);
}

bool isAllZero(const uint8_t* const sP)
{
	for (size_t i = 0; i < 9; i++)
	{
		if (sP[i] != 0)
		{
			return false;
		}
	}
	return true;
}

bool reset_return(bool value)
{
	wire->reset();
	return value;
}

bool requestTemperature()
{
	wire->reset();
	wire->skip();
	wire->write(0x44, parasite);
	if (!waitForConversion)
		return;
	if (checkForConversion && !parasite)
	{
		unsigned long start = millis();
		while ((wire->read_bit() != 1) && (millis() - start < 750))
		{
			yield();
		}
	}
	else
	{
		uint8_t delms;
		switch (bitResolution)
		{
		case 9:
			delms = 94;
			break;
		case 10:
			delms = 188;
			break;
		case 11:
			delms = 375;
			break;
		default:
			delms = 750;
			break;
		}
		if (useExternalPullup)
			digitalWrite(pullupPin, HIGH);
		delay(delms);
		if (useExternalPullup)
			digitalWrite(pullupPin, LOW);
	}
}

float getTemperatureIndex(uint8_t Index)
{
	DeviceAddress Address;
	if (!getAddress(Address, Index))
	{
		return -127;
	}
	ScratchPad sP;
	int16_t fpTemperature;
	if (isConnected(Address, sP))
	{
		fpTemperature = (((int16_t)sP[1]) << 11) | (((int16_t)sP[0]) << 3);
		if ((Address[0] == 0x10) && (sP[7] != 0))
		{
			fpTemperature = ((fpTemperature & 0xfff0) << 3) - 32 + (((sP[7] - sP[6]) << 7) / sP[7]);
		}
	}
	else
	{
		return -7040;
	}
	if (fpTemperature <= -7040)
		return -127;
	return (float)fpTemperature * 0.0078125f;
}

bool getAddress(uint8_t* dev, uint8_t index)
{
	uint8_t depth = 0;
	wire->reset_search();
	while (depth <= index && wire->search(dev))
	{
		if (depth == index && validAddress(dev))
			return true;
		depth++;
	}
	return false;
}

bool requestTemperatureAddress(const uint8_t* dev)
{
	uint8_t bitResolution = getResolution(dev);
	if (bitResolution == 0)
	{
		return false;
	}
	wire->reset();
	wire->select(dev);
	wire->write(0x44, parasite);

	if (!waitForConversion)
		return true;

	if (checkForConversion && !parasite)
	{
		unsigned long start = millis();
		while ((wire->read_bit() != 1) && (millis() - start < 750))
		{
			yield();
		}
	}
	else
	{
		uint8_t delms;
		switch (bitResolution)
		{
		case 9:
			delms = 94;
			break;
		case 10:
			delms = 188;
			break;
		case 11:
			delms = 375;
			break;
		default:
			delms = 750;
			break;
		}
		if (useExternalPullup)
			digitalWrite(pullupPin, HIGH);
		delay(delms);
		if (useExternalPullup)
			digitalWrite(pullupPin, LOW);
	}
	return true;
}
