/**
* "Playstation joystick reader v1.3"
* Microcontontroller reads data from joystick dualshock of playstation first generation.
* GNU GPLv3 © evil_linuxoid 2012
*/

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#define set_true(in, pos) in |= (1 << pos)
#define set_false(in, pos) in &= ~(1 << pos)
#define read_bit(in, pos) ((in & (1 << pos)) ? 1 : 0)

#define RS PA0
#define RW PA1
#define EN PA2

/* Input command in LCD */
void lcd_com(unsigned char p)
{
	set_false(PORTA, RS); //RS = 0 (for command)
	set_true(PORTA, EN); //EN = 1 (start to input command in LCD)
	PORTC = p; //input command 
	_delay_us(100); //duration of the signal
	set_false(PORTA, EN); //EN = 0 (finish to input command in LCD)
	
	if(p == 0x01 || p == 0x02) _delay_us(1800); //pause for execution
	else _delay_us(50);
}

/* Input data in LCD */
void lcd_dat(unsigned char p)
{
	set_true(PORTA, RS); //RS = 1(for data)
	set_true(PORTA, EN); //EN = 1 (start to input data in LCD)
	PORTC = p; //input data
	_delay_us(100); //duration of the signal
	set_false(PORTA, EN); //EN = 0 (finish to input data in LCD)
	_delay_us(50); //pause for execution
}

void lcd_write(char * string)
{
	int i = 0, count = strlen(string);
	for(i = 0; i < count; i ++) lcd_dat(string[i]);
}

/* The initialization function LCD */
void lcd_init(void)
{
	lcd_com(0x08); //display off(1640us)
	lcd_com(0x38); //8 bit, 2 lines(40us)
	lcd_com(0x01); //cleaning the display(1640us)
	lcd_com(0x06); //shift the cursor to the right(40us)
	lcd_com(0x0D); //a blinking cursor(40us)
}

/* Clearing of LCD */
void lcd_clr()
{
	lcd_com(0x01);
}

/* Moving cursor */
void lcd_curs(uint8_t line, uint8_t index)
{
	if((line == 0 || line == 1) && (0 <= index && index <= 15))
	{
		lcd_com((1 << 7) + (line << 6) + index);
	}
}

#define COM PB0
#define SEL PB1
#define ACK PB2
#define DATA PB3
#define CLK PB4

struct _psx
{
  	uint8_t ver;
	uint8_t data;
	uint8_t up, lf, dn, rt;
	uint8_t start, select;
	uint8_t triangle, square, cross, circle;
	uint8_t l1, l2, r1, r2;
	uint8_t lj, rj;
	uint8_t ljx, ljy, rjx, rjy;
};

struct _psx psx;

const int joy_clk_delay = 10;
const int joy_half_clk_delay = 5;

void joy_clk(uint8_t status)
{
	if(status) set_true(PORTB, CLK);
	else set_false(PORTB, CLK);
}

void joy_sel(uint8_t status)
{
	if(status) set_true(PORTB, SEL);
	else set_false(PORTB, SEL);
}

void joy_com(uint8_t status)
{
	if(status) set_true(PORTB, COM);
	else set_false(PORTB, COM);
}

int joy_dat()
{
	return read_bit(PINB, DATA);
}

int joy_ack()
{
	return read_bit(PINB, ACK);
}

int joy_send_byte(uint8_t byte)
{
	uint8_t i = 0;
	int data = 0;
	
	/* Sending every bit */
	for(i = 0; i < 8; i ++)
	{
	      _delay_us(joy_clk_delay);
	      
	      joy_com(byte & (1 << i));
	      joy_clk(0);
	      _delay_us(joy_clk_delay);	
	      
	      data |= (joy_dat() ? (1 << i) : 0);
	      joy_clk(1);
	}
	
	/* Waiting for acknowledgement */
	for(i = 0; (i <= 50) && joy_ack(); i ++)
	{
		_delay_us(joy_half_clk_delay);
	}
	
	if(i != 50) return data;
	else return -1;
}

void joy_scan()
{
	int data[9];
	
	/* Preparing */
	joy_com(1);
	joy_clk(1);
	joy_sel(0);
	_delay_us(joy_clk_delay);

	/* Scanning */
	data[0] = joy_send_byte(0x01); //start byte
	data[1] = joy_send_byte(0x42); //byte with version of joystick
	data[2] = joy_send_byte(0xff); //check byte
	
	//if(data[1] != 0x41 && data[1] != 0x73) continue; //need to get 0x41(digital mode) or 0x73(analog mode)
	//if(data[2] != 0x5a) continue; //need to get 0x5a
	
	data[3] = joy_send_byte(0xff); //data1
	data[4] = joy_send_byte(0xff); //data2
	data[5] = joy_send_byte(0xff); //analog2_x
	data[6] = joy_send_byte(0xff); //analog2_y
	data[7] = joy_send_byte(0xff); //analog1_x
	data[8] = joy_send_byte(0xff); //analog1_y
	
	/* Ending */
	_delay_us(joy_clk_delay);
	
	joy_sel(1);
	joy_com(1);
	joy_clk(1);
	
	/* Data handling */
	psx.ver = data[1];
	
	psx.up = (1 << 4) & data[3] ? 0 : 1;
	psx.lf = (1 << 7) & data[3] ? 0 : 1;
	psx.dn = (1 << 6) & data[3] ? 0 : 1;
	psx.rt = (1 << 5) & data[3] ? 0 : 1;
	
	psx.start = (1 << 3) & data[3] ? 0 : 1;
	psx.select = (1 << 0) & data[3] ? 0 : 1;
	
	psx.triangle = (1 << 4) & data[4] ? 0 : 1;
	psx.square = (1 << 7) & data[4] ? 0 : 1;
	psx.cross = (1 << 6) & data[4] ? 0 : 1;
	psx.circle = (1 << 5) & data[4] ? 0 : 1;

	psx.l1 = (1 << 2) & data[4] ? 0 : 1;
	psx.l2 = (1 << 0) & data[4] ? 0 : 1;
	psx.r1 = (1 << 3) & data[4] ? 0 : 1;
	psx.r2 = (1 << 1) & data[4] ? 0 : 1;

	/* Only in analog mode next values can be non zero */
	psx.lj = (1 << 1) & data[3] ? 0 : 1;
	psx.rj = (1 << 2) & data[3] ? 0 : 1;

	psx.ljx = data[7];
      	psx.ljy = data[8];
	psx.rjx = data[5];
	psx.rjy = data[6];
}

int main()
{
	/* For joystick pins */
	DDRB = (1 << COM) | (1 << SEL) | (1 << CLK);
	set_true(PORTB, DATA); //pin gets extra resistance of 100kOhm
	set_true(PORTB, ACK); //pin gets extra resistance of 100kOhm

	/* For LCD */
	set_true(DDRA, RS); //RC is output
	set_true(DDRA, RW); //RW is output
	set_true(DDRA, EN); //EN is output
	PORTA = 0x00; //zero in output
	DDRC = 0xff; //PD0-7 are output
	PORTC = 0x00; //zero in output
	
	/* LCD initialization */
	lcd_init();
	
	/* Greeting */
	char * greet_1 = "PSJoystickReader";
	char * greet_2 = "      v1.2      ";

	lcd_com(0x0C);
	
	lcd_curs(0, 0);
	lcd_write(greet_1);
	lcd_curs(1, 0);
	lcd_write(greet_2);
	_delay_ms(1500);

	/* Main code */
	char buf[2][16];
	
	lcd_clr();
	
	while(1)
	{
		joy_scan();

		sprintf(buf[0], "[%1d%1d%1d%1d][%1d%1d][%1d%1d%1d%1d]", psx.l1, psx.l2, psx.r1, psx.r2, psx.lj, psx.rj, psx.triangle, psx.square, psx.cross, psx.circle);
		
		if(psx.ver == 0x41) sprintf(buf[1], "%1d %1d %1d %1d         ", psx.up, psx.lf, psx.dn, psx.rt);
		else sprintf(buf[1], "%3d %3d %3d %3d ", psx.ljx, psx.ljy, psx.rjx, psx.rjy);

		lcd_curs(0, 0);
		lcd_write(buf[0]);
		lcd_curs(1, 0);
		lcd_write(buf[1]);
		
		_delay_ms(50);
	}

	return 0;
}
