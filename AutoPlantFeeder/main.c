/* AutoPlantFeeder.c
 * Created: 10/31/2022 9:21:36 AM
 * Author : agrosch
 */ 

#include <avr/io.h>
#define F_CPU 16000000
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>

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

/* Button Interrupts */

ISR(TIMER3_CAPT_vect) { //button
	statusLevel++;
	statusLevel %= 3;
	PrintStatus(statusLevel);
	delay8m();
}

int main(void)
{
  DDRB |= 0b00111010;
  DDRE |= 0b1010;
  PORTE |= 0x00; //0b00000011
  
	setupLCD();
	ADCInit();
	setupButtons();
	dry_max = air_value + (air_value-water_value)/3; //max dry interval value (~33% humidity)
	sei();
	//checkHumidity();
	//if (intHumidity < 30) { statusLevel = 2; }
	//else if (intHumidity > 70) { statusLevel = 0; }
  
    while (1) //cycle through displays
    {
		defaultLCD();
  		delay8m();
		//checkHumidity();
		//delay8m();
  		checkWaterLevel();
  		delay8m();
		readSoil();
		delay8m();
		triggerPump();
    
    }
}

/* LCD functions */
void setupLCD() {
	DDRD = 0xFF;   //D is output
	DDRE = 0xFF;	//B is output
	_delay_ms(100);
	WriteCommand(0x20);		//Set 8-bit mode
	_delay_ms(5);
	WriteCommand(0x28);		//2 lines - 5x8 font
	_delay_us(100);
	WriteCommand(0x08);		//Display off
	WriteCommand(0x01);		//clear display
	_delay_ms(2);
	WriteCommand(0x06);
	WriteCommand(0x0F);		//display,cursor, and blink on
}
void WriteCommand(uint8_t d){		//This function helps initialize the LCD
	E_LOW();	//E = 0
	RS_LOW();	//RS = 0
	E_HIGH();	//E = 1
	PORTD = ((d & 0xF0) >> 4); //send higher nibble
	_delay_ms(1);
	
	E_LOW();	//E = 0
	RS_LOW();	//RS = 0
	E_HIGH();	//E = 1
	
	PORTD = (d & 0x0F); //send lower nibble
	_delay_ms(1);
	E_LOW();	//E = 0
	_delay_us(39);
}
void WriteData(uint8_t d){		//This function helps write data to the LCD
	E_LOW();	//E = 0
	RS_HIGH();	//RS = 1
	E_HIGH();	//E = 1
	PORTD = ((d & 0xF0) >> 4);
	_delay_ms(1);
	
	E_LOW();	//E = 0
	RS_HIGH();	//RS = 1
	E_HIGH();	//E = 1
	
	PORTD = (d & 0x0F);
	_delay_ms(1);
	E_LOW();	//E = 0
	_delay_us(39);
}
void PrintString(char s[]){
	uint8_t i =0;
	while (s[i]){
		WriteData(s[i++]);		//Outputs data to LCD
	}
}
void GoToXY(uint8_t x, uint8_t y){
		WriteCommand(0x80 | (y*0x40 + x));
}
void delay8m() {
	for (int c = 0; c <= 8; c++)
	{
		_delay_ms(250);
	}
}
void clearLCD() {
	WriteCommand(0x01);
	_delay_ms(20);
}
void defaultLCD() {
	clearLCD();
	GoToXY(0,0);
	PrintString("Happy plant :)  ");
}

/* DHT11 */
void setupDHT() {
	//send start pulse
	DDRC |= (1<<IOBIT);
	IO_LOW(); //PORTC |= 1<<IOBIT;
	_delay_ms(20);
	IO_HIGH(); //PORTC &= ~(1<<IOBIT);

	//receive dht11 response
	DDRC &= ~(1<<IOBIT);
	while(PINC & (1<<IOBIT));
	while((PINC & (1<<IOBIT)) == 0);
	while(PINC & (1<<IOBIT));
}
uint8_t getData() {
	uint8_t data;
	for (int b = 0; b < 8; b++)
	{
		while((PINC & (1<<IOBIT)) == 0);
		_delay_us(30);
		if (PINC & (1<<IOBIT))
		{ 
			data = (data<<1) | (0x01);
		} //if pulse > 30ms, it's logic HIGH
		else { data = (data<<1); }
		while(PINC & (1<<IOBIT));
	}
	return data;
}
void checkHumidity() {
	setupDHT();
	intHumidity = getData();
	floatHumidity = getData();
	intTemp = getData();
	floatTemp = getData();
	checkSum = getData();
	char stringH[16], stringT[16];
	
	if ((intHumidity + floatHumidity + intTemp + floatTemp) != checkSum)
	{
		clearLCD();
		GoToXY(0,0);
		PrintString("DHT11 Error    ");
	}
	else //debugging values
	{
		sprintf(stringH, "H = %d.%d        ", intHumidity, floatHumidity);
		sprintf(stringT, "T = %d.%d        ", intTemp, floatTemp);
		clearLCD();
		GoToXY(0,0);
		PrintString(stringH);
		GoToXY(0,1);
		PrintString(stringT);
	}
}

/* Read Sensors */
/*	ADEN = ADC Enable: 1 to enable
	ADSC = start conversion: 1 */
void ADCInit() {
	DDRC |= 0x20; //0b00100000
	ADMUX = (1<<REFS0); //0b01000000
			//REFS 01 = AVcc w/ external cap @ AREF pin
			//MUX 0000 = ADC0
	ADCSRA = (1<<ADEN) | (1<<ADPS2) | (1<<ADPS1) | (1<<ADPS0);
			// ADCSRA = 0b10000111
	ADCSRB = (0b000<<0) | (0b0<<6);  //Free Running Mode
}

uint16_t AnalogRead (uint8_t channel) {
	ADMUX = (ADMUX & 0xF0) | (channel & 0xF0); 
	ADCSRA |= (1<<ADSC);				//start conversion
	while ((ADCSRA & (1<<ADSC)) == 0); //wait for conversion end
	return ADC;
}

/* Water Level Sensor */
void checkWaterLevel() {
	PORTC |= (1<<5);		// turn on sensor
	_delay_ms(20);
	water_level_val = AnalogRead(H2OBIT); //connected to ADC4 (PC4)
	_delay_ms(20);
	PORTC &= ~(1<<5);		// turn off sensor
	PrintWater();
}

void PrintWater() {
	clearLCD();
	GoToXY(0,0);
	if (water_level_val < low_limit)
	{
		PrintString("Add Water       ");
	}
	else if (water_level_val > high_limit)
	{
		PrintString("Too Much Water  ");
	}
	else
	{
		PrintString("Water Level Good");
	}
}

/* Soil Moisture Sensor */
void readSoil() {
	char stringM[16];
	soil_moisture_val = AnalogRead(2); //connected to ADC2 (PC2)
	double moisture = 100-(soil_moisture_val*100.00)/1023.00;
  PrintSoil(moisture);
  
	if (soil_moisture_val <= dry_max) { triggerPump(); }
}

void PrintSoil(double moisture) {
  char stringM[16];
  sprintf(stringM, "%f ", moisture);
  clearLCD();
  GoToXY(0,0);
  PrintString("Moisture:       ");
  GoToXY(0,1);
  PrintString(stringM);
}

/* Buttons */
void setupButtons() {
	//Button 1
	TCCR3B |= (0<<ICES3);	//trigger on falling edge
	TIFR3 = (1<<ICF3);		//set input capture flag
	TIMSK3 = (1<<ICIE3);	//enable input capture mode
}

void PrintStatus(uint8_t s) {
	clearLCD();
	GoToXY(0,0);
	PrintString("Water Freq:");
	GoToXY(0,1);
	switch(s) {
		case 0:
			PrintString("Low Watering    ");
			break;
		case 1:
			PrintString("Med Watering    ");
			break;
		case 2:
			PrintString("High Watering   ");
			break;
		default:
			PrintString("Error           ");
	}	
}

/* Water Pump */
void triggerPump() {
	PUMP_ON();
	//need to figure out how long to run pump
	switch(statusLevel) {
		case 0: //min watering
			delay8m();
			break;
		case 1: //mid watering
			delay8m();
			delay8m();
			break;
		case 2: //max watering
			delay8m();
			delay8m();
			delay8m();
			break;
		default:
			clearLCD();
			GoToXY(0,0);
			PrintString("Err: statusLevel");
			GoToXY(0,1);
			PrintString("not defined     ");
	}
	PUMP_OFF();
}