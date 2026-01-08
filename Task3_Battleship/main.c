//------------------------------------------- main.c CODE STARTS ---------------------------------------------------------------------------
#include <stdio.h>
#include "NUC100Series.h"
#include "MCU_init.h"
#include "SYS_init.h"
#include "LCD.h"

#define BOUNCING_DLY 200000
#define LED_DLY 100000
#define BUZZER_DLY 50000

#define ROW 8
#define COL 8

volatile uint8_t rxData;
volatile int row = 0, col = 0;
volatile int map[ROW][COL];
volatile int playMap[ROW][COL];
volatile int screenClear = 0;
volatile int rowColSelection = 0;
volatile int shot = 0;
volatile int hit = 0;
volatile int part = 0;
volatile int buzzCount = 0;

enum States {
	WELCOME,
	MAP_LOADED,
	GAME_STARTED,
	GAME_FINISHED
};

volatile enum States state = WELCOME;

volatile int digit = 0;

void System_Config(void);
void UART0_Config(void);
void SPI3_Config(void);
void Timer0_Config(void);

void UART02_IRQHandler(void);
void EINT1_IRQHandler(void);
void TMR0_IRQHandler(void);

void LCD_start(void);

void number_display(int number);

void KeyPadEnable(void);
uint8_t KeyPadScanning(void);

int main(void)
{
	uint8_t pressed_key = 0;
	
	System_Config();
	UART0_Config();
	SPI3_Config();
	Timer0_Config();

	LCD_start();
	clear_LCD();
	
	KeyPadEnable();

	while(1){

		pressed_key=0;
		if (screenClear) {
			clear_LCD();
			screenClear = 0;
		}

		switch(state) {
			case WELCOME:
				buzzCount = 0;
				print_Line(1, "Battleship");
				break;
			
			case MAP_LOADED:
				printS_5x7(6, 24, "Map Loaded Successfully");
				break;
			
			case GAME_STARTED:
				// print map
				for (int i = 0; i < ROW; i++) {
					for (int j = 0; j < COL; j++) {
						if (map[i][j] && playMap[i][j]) {
							printC_5x7(j*17, i*8, 'X');
						} else if (!map[i][j] && playMap[i][j]) {
							printC_5x7(j*17, i*8, '0');
						} else {
							printC_5x7(j*17, i*8, '-');
						}
					}
				}
				
				// scan for key press activity
				while (pressed_key == 0) {
					pressed_key = KeyPadScanning();
					if (screenClear) {
						break;
					}
				}
				CLK_SysTickDelay(BOUNCING_DLY);
				
				switch(pressed_key){
					case 0:
						break;
					
					case 9:
						if(!rowColSelection) {
							rowColSelection = 1;
							PC->DOUT &= ~(1 << 13);
						} else {
							rowColSelection = 0;
							PC->DOUT |= (1 << 13);
						}
						break;
					
					default:
						if (!rowColSelection) {
							col = pressed_key - 1;
						} else {
							row = pressed_key - 1;
						}
						break;
				}
				pressed_key = 0;
				break;
			
			case GAME_FINISHED:
				if (hit >= part) {
					print_Line(1, "You Won");
				} else {
					print_Line(1, "Game Over");
				}
				while (buzzCount < 10) {
					PB->DOUT ^= (1 << 11);
					CLK_SysTickDelay(BUZZER_DLY);
					buzzCount++;
				}
				break;
		}
	}
}


//------------------------------------------------------------------------------------------------------------------------------------
// Functions definition
//------------------------------------------------------------------------------------------------------------------------------------
void System_Config(void) {
	SYS_UnlockReg(); // Unlock protected registers
	CLK->PWRCON |= (0x01 << 0);
	while(!(CLK->CLKSTATUS & (1 << 0)));
	

	CLK->PWRCON |= (0x01 << 3);
	while(!(CLK->CLKSTATUS & (1 << 3)));
		
	//PLL configuration starts
	CLK->PLLCON &= ~(1<<19); //0: PLL input is HXT
	CLK->PLLCON &= ~(1<<16); //PLL in normal mode
	CLK->PLLCON &= (~(0x01FF << 0));
	CLK->PLLCON |= 48;
	CLK->PLLCON &= ~(1<<18); //0: enable PLLOUT
	while(!(CLK->CLKSTATUS & (0x01ul << 2)));
	//PLL configuration ends
		
	//clock source selection
	CLK->CLKSEL0 &= (~(0x07 << 0));
	CLK->CLKSEL0 |= (0x02 << 0);
	//clock frequency division
	CLK->CLKDIV &= (~0x0F << 0);

	//UART0 Clock selection and configuration
	CLK->CLKSEL1 |= (0b11 << 24); // UART0 clock source is 22.1184 MHz
	CLK->CLKDIV &= ~(0xF << 8); // clock divider is 1
	CLK->APBCLK |= (1 << 16); // enable UART0 clock

	//enable clock of SPI3
	CLK->APBCLK |= 1 << 15;
	
	//BUZZER to indicate game status
	PB->PMD &= (~(0x03 << 22));
	PB->PMD |= (0x01 << 22);

	//GPIO Interrupt configuration. GPIO-B15 is the interrupt source
	PB->PMD &= (~(0x03 << 30));
	PB->IMD &= (~(1 << 15));
	PB->IEN |= (1 << 15);
	
	PB->DBEN |= (1 << 15);
	GPIO->DBNCECON |= (1 << 4);
	GPIO->DBNCECON |= 8;
	
	//NVIC interrupt configuration for GPIO-B15 interrupt source
	NVIC->ISER[0] |= 1 << 3;
	NVIC->IP[0] &= (~(3 << 30));
	
	// 7 segments
	// Set PC4 - PC7 to output mode
	PC->PMD &= ~(0xFF << 8);
	PC->PMD |= (0x55 << 8);

	// Set PE0 - PE7 to output mode
	PE->PMD &= ~(0xFFFF << 0);
	PE->PMD |= (0x5555 << 0);
	
	PC->PMD &= ~(0xFF << 24);
	PC->PMD |= (0x55 << 24);
	
	PC->PMD &= ~(0xFF << 26);
	PC->PMD |= (0x55 << 26);
		
	SYS_LockReg();  // Lock protected registers
}

void UART0_Config(void) {
    // UART0 pin configuration. 
		//PB.1 pin is for UART0 TX
    PB->PMD &= ~(0b11 << 2);
    PB->PMD |= (0b01 << 2); // PB.1 is output pin
    SYS->GPB_MFP |= (1 << 1); // GPB_MFP[1] = 1 -> PB.1 is UART0 TX pin
	
		// PB.0 pin is for UART0 RX
		PB->PMD &= ~(0b11 << 0); // Clear and set PB.0 as input pin
		PB->IMD &= ~(1<<0);
		PB->IEN |= (1<<0);
		SYS->GPB_MFP |= (1 << 0); // GPB_MFP[0] = 1 -> PB.0 is UART0 RX pin

    // UART0 operation configuration
    UART0->LCR |= (0b11 << 0); // 8 data bit
    UART0->LCR &= ~(1 << 2); // one stop bit    
    UART0->LCR &= ~(1 << 3); // no parity bit
    UART0->FCR |= (1 << 1); // clear RX FIFO
    UART0->FCR |= (1 << 2); // clear TX FIFO
    UART0->FCR &= ~(0xF << 16); // FIFO Trigger Level is 1 byte]
    
    //Baud rate config: BRD/A = 1, DIV_X_EN=0
    //--> Mode 0, Baud rate = UART_CLK/[16*(A+2)] = 22.1184 MHz/[16*(10+2)]= 115200 bps

    UART0->BAUD &= ~(0b11 << 28); // mode 0 
    UART0->BAUD &= ~(0xFFFF << 0);
    UART0->BAUD |= 10;  //A = 10
		
	//set up UART interrupt and interrupt priority
	UART0->IER |= 1<<0;
	NVIC->ISER[0] |= 1<<12;
	NVIC->IP[3] &= ~(0b11<<6);
}

void SPI3_Config(void) {
	SYS->GPD_MFP |= 1 << 11; //1: PD11 is configured for alternative function
	SYS->GPD_MFP |= 1 << 9; //1: PD9 is configured for alternative function
	SYS->GPD_MFP |= 1 << 8; //1: PD8 is configured for alternative function
	
	SPI3->CNTRL &= ~(1 << 23); //0: disable variable clock feature
	SPI3->CNTRL &= ~(1 << 22); //0: disable two bits transfer mode
	SPI3->CNTRL &= ~(1 << 18); //0: select Master mode
	SPI3->CNTRL &= ~(1 << 17); //0: disable SPI interrupt
	SPI3->CNTRL |= 1 << 11; //1: SPI clock idle high
	SPI3->CNTRL &= ~(1 << 10); //0: MSB is sent first
	SPI3->CNTRL &= ~(3 << 8); //00: one transmit/receive word will be executed in one data transfer
	
	SPI3->CNTRL &= ~(31 << 3); //Transmit/Receive bit length
	SPI3->CNTRL |= 9 << 3;     //9: 9 bits transmitted/received per data transfer
	
	SPI3->CNTRL |= (1 << 2);  //1: Transmit at negative edge of SPI CLK
	SPI3->DIVIDER = 0; // SPI clock divider. SPI clock = HCLK / ((DIVIDER+1)*2). HCLK = 50 MHz
}

void Timer0_Config(void) {
	// Timer 0 from HXT
	CLK->CLKSEL1 &= ~(0b111 << 0);
	CLK->APBCLK |= (1 << 2);
	
	// Reset Timer 0
	TIMER0->TCSR |= (1 << 26);
	TIMER0->TCSR &= ~(0xFF << 0);
	//TIMER0->TCSR |= (11 << 0);
	
	TIMER0->TCSR &= ~(0b11 << 27);
	TIMER0->TCSR |= (0b01 << 27);
	TIMER0->TCSR &= ~(1 << 24);
	TIMER0->TCSR |= (1 << 16);
	TIMER0->TCSR |= (1 << 29);
	
	TIMER0->TCMPR = 49999;
	TIMER0->TCSR |= (1 << 30);
	
	NVIC->ISER[0] |= 1 << 8;
	NVIC->IP[2] &= ~(3 << 6);
}

void EINT1_IRQHandler(void) {
	//CLK_SysTickDelay(BOUNCING_DLY);
	switch(state) {
		case WELCOME:
			break;
		
		case MAP_LOADED:
			state = GAME_STARTED;
			break;
		
		case GAME_STARTED:
			shot++;
			if (map[row][col] && !playMap[row][col]) {
				hit++;
				for (int i = 0; i < 6; i++) {
					PC->DOUT ^= (1 << 12);
					CLK_SysTickDelay(LED_DLY);
				}
			}
			playMap[row][col] = 1;
			if (shot >= 16 || hit >=part) {
				state = GAME_FINISHED;
			}
			break;
			
		case GAME_FINISHED:
			state = WELCOME;
			rowColSelection = 0;
			shot = 0;
			hit = 0;
			break;
	}
	screenClear = 1;
	PB->ISRC |= (1 << 15);
}

void UART02_IRQHandler(void) {
	if (UART0->ISR & 1<<0 && state == WELCOME) {
		rxData = UART0->RBR;
		
		if (rxData == '0') {
			map[row][col] = 0;
			col++;
		} else if (rxData == '1') {
			map[row][col] = 1;
			col++;
			part++;
		}
		
		while(UART0->FSR & (1 << 23));
		UART0->DATA = rxData;
		
		if (col == COL) {
			row++;
			col = 0;
		}
		
		if (row == ROW) {
			state = MAP_LOADED;
			row = 0;
			col = 0;
			screenClear = 1;
		}
	}
}

void TMR0_IRQHandler(void) {
	PC->DOUT &= ~(0xF << 4);
	if (state == GAME_STARTED) {
		if (digit == 0) {
			PC->DOUT |= (1 << 7);
			if (!rowColSelection) {
				number_display(col + 1);
			} else {
				number_display(row + 1);
			}
			digit = 1;
		} else if (digit == 1) {
			PC->DOUT |= (1 << 5);
			number_display(shot / 10);
			digit = 2;
		} else if (digit == 2) {
			PC->DOUT |= (1 << 4);
			number_display(shot % 10);
			digit = 0;
		}
	}
		
	TIMER0->TISR |= (1 << 0);
}

void LCD_start(void) {
	lcdWriteCommand(0xE2); // Set system reset
	lcdWriteCommand(0xA1); // Set Frame rate 100 fps
	lcdWriteCommand(0xEB); // Set LCD bias ratio E8~EB for 6~9 (min~max)
	lcdWriteCommand(0x81); // Set V BIAS potentiometer
	lcdWriteCommand(0xA0); // Set V BIAS potentiometer: A0 ()
	lcdWriteCommand(0xC0);
	lcdWriteCommand(0xAF); // Set Display Enable
}

void number_display(int number) {
	switch(number) {
		case 0:
			PE->DOUT &= ~(0xFF << 0);
			PE->DOUT |= (0x82 << 0);
			break;
		case 1:
			PE->DOUT &= ~(0xFF << 0);
			PE->DOUT |= (0xEE << 0);
			break;
		case 2:
			PE->DOUT &= ~(0xFF << 0);
			PE->DOUT |= (0x07 << 0);
			break;
		case 3:
			PE->DOUT &= ~(0xFF << 0);
			PE->DOUT |= (0x46 << 0);
			break;
		case 4:
			PE->DOUT &= ~(0xFF << 0);
			PE->DOUT |= (0x6A << 0);
			break;
		case 5:
			PE->DOUT &= ~(0xFF << 0);
			PE->DOUT |= (0x52 << 0);
			break;
		case 6:
			PE->DOUT &= ~(0xFF << 0);
			PE->DOUT |= (0x12 << 0);
			break;
		case 7:
			PE->DOUT &= ~(0xFF << 0);
			PE->DOUT |= (0xE6 << 0);
			break;
		case 8:
			PE->DOUT &= ~(0xFF << 0);
			PE->DOUT |= (0x02 << 0);
			break;
		case 9:
			PE->DOUT &= ~(0xFF << 0);
			PE->DOUT |= (0x42 << 0);
			break;
		default:
			break;
	}
}

void KeyPadEnable(void) {
	GPIO_SetMode(PA, BIT0, GPIO_MODE_QUASI);
	GPIO_SetMode(PA, BIT1, GPIO_MODE_QUASI);
	GPIO_SetMode(PA, BIT2, GPIO_MODE_QUASI);
	GPIO_SetMode(PA, BIT3, GPIO_MODE_QUASI);
	GPIO_SetMode(PA, BIT4, GPIO_MODE_QUASI);
	GPIO_SetMode(PA, BIT5, GPIO_MODE_QUASI);
}

uint8_t KeyPadScanning(void) {
	PA0=1; PA1=1; PA2=0; PA3=1; PA4=1; PA5=1;
	if (PA3==0) return 1;
	if (PA4==0) return 4;
	if (PA5==0) return 7;
	PA0=1; PA1=0; PA2=1; PA3=1; PA4=1; PA5=1;
	if (PA3==0) return 2;
	if (PA4==0) return 5;
	if (PA5==0) return 8;
	PA0=0; PA1=1; PA2=1; PA3=1; PA4=1; PA5=1;
	if (PA3==0) return 3;
	if (PA4==0) return 6;
	if (PA5==0) return 9;
	return 0;
}
//------------------------------------------- main.c CODE ENDS ---------------------------------------------------------------------------