#include "mcp3221.h"
#include "I2C.h"
#include <logdef.h>

//Create instance  MCP3221(gpio_num_t sda, gpio_num_t scl);
MCP3221::MCP3221()
{
	errorcount=0;
	exponential_average=0;
	_noDevice = false;
}

bool MCP3221::begin(gpio_num_t sda, gpio_num_t scl)
{
	errorcount=0;
	// init( sda, scl );
	exponential_average = 0;
	return true;
}

//destroy instance
MCP3221::~MCP3221()
{
}

// scan bus for I2C address
esp_err_t MCP3221::selfTest(){
	uint8_t data[2];
	esp_err_t err = bus->readBytes(MCP3221_CONVERSE, 0, 2, data );
	if( err != ESP_OK ){
		ESP_LOGI(FNAME,"MCP3221 selftest, scan for I2C address %02x FAILED",MCP3221_CONVERSE );
		return ESP_FAIL;
	}
	ESP_LOGI(FNAME,"MCP3221 selftest, scan for I2C address %02x PASSED",MCP3221_CONVERSE );
	return ESP_OK;
}

float MCP3221::readAVG( float alpha ) {

	uint16_t newval;
	esp_err_t ret = readRaw(newval);
	// ESP_LOGI(FNAME,"Airspeed AD1: %d", newval );
	if( ret == ESP_OK ){
		// ESP_LOGI(FNAME, "%d", newval );
		if ( exponential_average == 0 ){
			exponential_average = newval;
		}
		exponential_average = exponential_average + alpha*(newval - exponential_average);
		return exponential_average;
	}
	else
		return 0.0;
}
//You cannot write to an MCP3221, it has no writable registers.
//MCP3221 also requires an ACKnowledge between each byte sent, before it will send the next byte. So we need to be a bit manual with how we talk to it.
//It also needs an (NOT) ACKnowledge after the second byte or it will keep sending bytes (continuous sampling)
//
//From the datasheet.
//
//I2C.START
//Send 8 bit device/ part address to open conversation.   (See .h file for part explanation)
//read a byte (with ACK)
//read a byte (with NAK)
//I2C.STOP

uint16_t as_last= 1000;

int  MCP3221::readVal(){
	int retval = 0;
	int samples = 0;
	for( int i=0; i<4; i++ ){
		uint16_t as;
		if( readRaw( as ) == ESP_OK ) {
			if( abs( as - as_last) < 500 ) {
				retval += as;
				samples++;
			}
			else{
				 ESP_LOGE(FNAME,"Airspeed delta out of bounds, cur:%d  last:%d", as, as_last );
			}
			as_last = as;
		}
		else
			return -1;
	}
	retval = retval / samples;
	// ESP_LOGI(FNAME,"Airspeed AD1 readVal: %d", retval );
	return retval;
}


esp_err_t MCP3221::readRaw(uint16_t &val)
{
	// esp_err_t ret=read16bit( MCP3221_CONVERSE, &val );
	uint8_t data[2];
	esp_err_t err = bus->readBytes(MCP3221_CONVERSE, 0, 2, data );
	if( err != ESP_OK ){
		val = 0;
	}else{
		val = data[1] | (data[0] << 8 );
	}

	return( err );
}


