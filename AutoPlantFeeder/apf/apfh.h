/* 	apfh.h 
 * 	Header file for Auto Plant Feeder project.
 */ 


#ifndef APFH_H_
#define APFH_H_
// ------------- INLINES ------------
//LCD bit connections
#define EBIT 1		//PE1
#define RSBIT 0		//PE0
//DHT11 sensor bit connections
#define IOBIT 3		//PC3 (ADC3/PTCY)
#define U3BIT 5		//PB5

//Water Level sensor bit connection
#define H2OBIT 4	//PC4 (ADC4/PTCY/SDA0)

//LCD
inline void E_HIGH(){ PORTE |= (1<<EBIT); }
inline void E_LOW(){ PORTE &= ~(1<<EBIT); }
inline void RS_HIGH(){ PORTE |= (1<<RSBIT); }
inline void RS_LOW(){ PORTE &= ~(1<<RSBIT); }
inline void Delay_250ns() {
	asm volatile(
	"swap R16\t\n"
	"swap R16\t\n"
	"swap R16\t\n"
	"swap R16\t\n"
	);
}

//DHT11
inline void IO_HIGH() { PORTC |= 1<<IOBIT; }
inline void IO_LOW() { PORTC &= ~(1<<IOBIT); }

//Water Pump
inline void PUMP_ON() { PORTB |= 1<<U3BIT; }
inline void PUMP_OFF() { PORTB &= ~(1<<U3BIT); }

// ---------- DECLARATIONS -------------
//define LCD functions
void setupLCD();
void WriteCommand(uint8_t d);
void WriteData(uint8_t d);
void PrintString(char s[]);
void GoToXY(uint8_t x, uint8_t y);
void clearLCD();
void defaultLCD();
void delay8m();

//define DHT functions
void setupDHT();
uint8_t getData();
//void checkHumidity();
void printDHTData(uint8_t intH, uint8_t floatH, uint8_t intT, uint8_t floatT);
uint8_t intHumidity, floatHumidity, intTemp, floatTemp, checkSum;

//define ADC functions
void ADCInit();
uint16_t AnalogRead (uint8_t channel);

//define water level sensor functions
void checkWaterLevel();
void PrintWater();
const uint8_t low_limit = 60; //min good water level
const uint8_t high_limit = 180; //max good water level
uint8_t water_level_val; //value to save readings to

//define soil moisture sensor functions
void readSoil();
void PrintSoil(double moisture);
const uint8_t air_value = 0; // 0% humidity
const uint8_t water_value = 0x7F; // 100% humidity
uint8_t dry_max;
uint8_t water_min;
uint16_t soil_moisture_val;

//define button functions
void setupButtons();
void PrintStatus(uint8_t s);
uint8_t statusLevel = 1; //start at medium (0=minimal, 1=medium, 2=max)
const char status[3] = "LMH";
	
//define water pump functions
void triggerPump();
#endif /* APFH_H_ */