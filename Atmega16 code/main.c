/*
 * Proyecto - Dispensador de comida.c
 *
 * Created: 09/06/2022 12:08:28 p. m.
 * Author : Alan Samuel Aguirre Salazar
 */ 

// ----------------------- Definiciones -----------------------
// Frecuencia CPU.
	#define F_CPU 4000000
	
// Definiciones del LCD.
	#define DDRLCD DDRA
	#define PORTLCD PORTA
	#define PINLCD PINA
	#define RS 4
	#define RW 5
	#define E 6
	#define BF 3
	#define LCD_Cmd_Clear      0b00000001
	#define LCD_Cmd_Home       0b00000010
	#define LCD_Cmd_ModeDnS	   0b00000110			// Sin shift cursor a la derecha.
	#define LCD_Cmd_ModeInS	   0b00000100			// Sin shift cursor a la izquierda.
	#define LCD_Cmd_ModeIcS	   0b00000111			// Con shift desplazamiento a la izquierda.
	#define LCD_Cmd_ModeDcS	   0b00000101			// Con shift desplazamiento a la derecha.
	#define LCD_Cmd_Off		   0b00001000
	#define LCD_Cmd_OnsCsB	   0b00001100
	#define LCD_Cmd_OncCsB     0b00001110
	#define LCD_Cmd_OncCcB     0b00001111
	#define LCD_Cmd_Func2Lin   0b00101000
	#define LCD_Cmd_Func1LinCh 0b00100000
	#define LCD_Cmd_Func1LinG  0b00100100
	
// Definiciones I2C.
	#define SCL_CLOCK  100000L

// Definiciones de la EEPROM.
    #define ALARM_BITS 8



// ----------------------- Librerías -----------------------
// Librerías.
	#include <avr/io.h>
	#include <util/delay.h>
	#include <stdint.h>
	#include <stdlib.h>
	#include <avr/interrupt.h>
	#include <time.h>
	
	
	
// ----------------------- Variables a utilizar -----------------------
// Hx711.
	float OFFSET = 8615000;
	float SCALE = 1900;
	float currentFoodAmount = 0;
	int maxFoodAmountToGive = 220;

// DS3231.
	uint8_t seconds = 0;
	uint8_t minutes = 0;
	uint8_t hours = 0;
	uint8_t date = 0;
	uint8_t month = 0;
	uint8_t year = 0;
	unsigned long int dateCombination = 0;
	int hourCombination = 0;
	char alreadyGiveFood = 0;
	uint8_t pastAlarmMinutes = 100, pastAlarmHours = 100;
	
	

// ----------------------- Esqueletos de funciones -----------------------
// Esqueletos de funciones útiles.
	uint8_t cero_en_bit(volatile uint8_t *LUGAR, uint8_t BIT);
	uint8_t uno_en_bit(volatile uint8_t *LUGAR, uint8_t BIT);
	void saca_uno(volatile uint8_t *LUGAR, uint8_t BIT);
	void saca_cero(volatile uint8_t *LUGAR, uint8_t BIT);
	
// Esqueletos de funciones de prints.
	void printValues8Bits(uint8_t valor);
	void printValues8BitsTimeFormat(uint8_t valor);
	void printValues(long valor);
	void printValuesWithDecimal(float valor);
	
// Esqueletos de funciones del LCD.
	void LCD_wr_inst_ini(uint8_t instruccion);
	void LCD_wr_char(uint8_t data);
	void LCD_wr_instruction(uint8_t instruccion);
	void LCD_wait_flag(void);
	void LCD_init(void);
	void LCD_wr_string(volatile uint8_t *s);

// Esqueletos del EEPROM
	void EEPROM_write(volatile uint16_t dir, volatile uint8_t data);
	uint8_t EEPROM_read(uint16_t dir);
	
// Esqueletos de Hx711.
	void Hx711_Calibration();
	unsigned long Hx711_ReadCount();
	float read_average(uint8_t times);
	float get_value(uint8_t times);
	float get_units(uint8_t times);
	void tare(uint8_t times);
	
// Esqueletos de I2C.
	void I2C_Init();
	void I2C_Start();
	uint8_t I2C_Read_Acknoledgement();
	uint8_t I2C_Read_Not_Acknoledgement();
	void I2C_Write(uint8_t data);
	void I2C_Stop();
	
// Esqueletos de DS3231.
	uint8_t  BCD_To_DEC(uint8_t BCD_value);
	uint8_t DEC_To_BCD (uint8_t decimal_value);
	void SetTimeDate(uint8_t _minutes, uint8_t _hours, uint8_t _date, uint8_t _month, uint8_t _year);
	void readTimeDate();
	void updateTimeDate();
	
// Esqueletos de funciones de mostrar tipos de pantalla.
	void showMainScreen();
	void showWeightScreen();
	void showAgendOptions();
	void showDeleteAlarms();
	void showAddAlarms();
	uint8_t searchAlarms();
	void showGiveFoodScreen();
	void showGivingFoodScreen(int foodAmount);
	void checkAlarms();
	
	
	






// ----------------------- Funciones implementadas -----------------------
// Funciones útiles.
uint8_t cero_en_bit(volatile uint8_t *LUGAR, uint8_t BIT){
	return (!(*LUGAR&(1<<BIT)));
}

uint8_t uno_en_bit(volatile uint8_t *LUGAR, uint8_t BIT){
	return (*LUGAR&(1<<BIT));
}

void saca_uno(volatile uint8_t *LUGAR, uint8_t BIT){
	*LUGAR=*LUGAR|(1<<BIT);
}

void saca_cero(volatile uint8_t *LUGAR, uint8_t BIT){
	*LUGAR=*LUGAR&~(1<<BIT);
}


// Funciones del LCD.
void LCD_init(void){
	DDRLCD=(15<<0)|(1<<RS)|(1<<RW)|(1<<E); //DDRLCD=DDRLCD|(0B01111111)
	_delay_ms(15);
	LCD_wr_inst_ini(0b00000011);
	_delay_ms(5);
	LCD_wr_inst_ini(0b00000011);
	_delay_us(100);
	LCD_wr_inst_ini(0b00000011);
	_delay_us(100);
	LCD_wr_inst_ini(0b00000010);
	_delay_us(100);
	LCD_wr_instruction(LCD_Cmd_Func2Lin); //4 Bits, número de líneas y tipo de letra
	LCD_wr_instruction(LCD_Cmd_Off); //apaga el display
	LCD_wr_instruction(LCD_Cmd_Clear); //limpia el display
	LCD_wr_instruction(LCD_Cmd_ModeDnS); //Entry mode set ID S
	LCD_wr_instruction(LCD_Cmd_OnsCsB); //Enciende el display
}

void LCD_wr_char(uint8_t data){
	//saco la parte más significativa del dato
	PORTLCD=data>>4; //Saco el dato y le digo que escribiré un dato
	saca_uno(&PORTLCD,RS);
	saca_cero(&PORTLCD,RW);
	saca_uno(&PORTLCD,E);
	_delay_ms(10);
	saca_cero(&PORTLCD,E);
	//saco la parte menos significativa del dato
	PORTLCD=data&0b00001111; //Saco el dato y le digo que escribiré un dato
	saca_uno(&PORTLCD,RS);
	saca_cero(&PORTLCD,RW);
	saca_uno(&PORTLCD,E);
	_delay_ms(10);
	saca_cero(&PORTLCD,E);
	saca_cero(&PORTLCD,RS);
	LCD_wait_flag();
}

void LCD_wr_inst_ini(uint8_t instruccion){
	PORTLCD=instruccion; //Saco el dato y le digo que escribiré un dato
	saca_cero(&PORTLCD,RS);
	saca_cero(&PORTLCD,RW);
	saca_uno(&PORTLCD,E);
	_delay_ms(10);
	saca_cero(&PORTLCD,E);
}

void LCD_wr_instruction(uint8_t instruccion){
	//saco la parte más significativa de la instrucción
	PORTLCD=instruccion>>4; //Saco el dato y le digo que escribiré un dato
	saca_cero(&PORTLCD,RS);
	saca_cero(&PORTLCD,RW);
	saca_uno(&PORTLCD,E);
	_delay_ms(10);
	saca_cero(&PORTLCD,E);
	//saco la parte menos significativa de la instrucción
	PORTLCD=instruccion&0b00001111; //Saco el dato y le digo que escribiré un dato
	saca_cero(&PORTLCD,RS);
	saca_cero(&PORTLCD,RW);
	saca_uno(&PORTLCD,E);
	_delay_ms(10);
	saca_cero(&PORTLCD,E);
	LCD_wait_flag();
}

void LCD_wait_flag(void){
	//	_delay_ms(100);
	DDRLCD&=0b11110000; //Para poner el pin BF como entrada para leer la bandera lo demás salida
	saca_cero(&PORTLCD,RS);// Instrucción
	saca_uno(&PORTLCD,RW); // Leer
	while(1){
		saca_uno(&PORTLCD,E); //pregunto por el primer nibble
		_delay_ms(10);
		saca_cero(&PORTLCD,E);
		if(uno_en_bit(&PINLCD,BF)) {break;} //uno_en_bit para protues, 0 para la vida real
		_delay_us(10);
		saca_uno(&PORTLCD,E); //pregunto por el segundo nibble
		_delay_ms(10);
		saca_cero(&PORTLCD,E);
	}
	saca_uno(&PORTLCD,E); //pregunto por el segundo nibble
	_delay_ms(10);
	saca_cero(&PORTLCD,E);
	//entonces cuando tenga cero puede continuar con esto...
	saca_cero(&PORTLCD,RS);
	saca_cero(&PORTLCD,RW);
	DDRLCD|=(15<<0)|(1<<RS)|(1<<RW)|(1<<E);
}

void LCD_wr_string(volatile uint8_t *s){
	uint8_t c;
	while((c=*s++)){
		LCD_wr_char(c);
	}
}


// Funciones del EEPROM
void EEPROM_write(volatile uint16_t dir, volatile uint8_t data){
	while(uno_en_bit(&EECR, EEWE)){}
	
	EEAR = dir;
	EEDR = data;
	
	cli();
	EECR |= (1<<EEMWE);
	EECR |= (1<<EEWE);
	sei();
}

uint8_t EEPROM_read(uint16_t dir){
	while(uno_en_bit(&EECR, EEWE)){}

	EEAR = dir;
	
	EECR |= (1<<EERE);
	
	return EEDR;
}


// Funciones del Hx711.
unsigned long Hx711_ReadCount(){
	unsigned long count;
	unsigned char i;
	
	saca_uno(&PORTD,0); 
	saca_cero(&PORTD,1);       
	count=0;                              
	while(uno_en_bit(&PIND,0));  

	// Leer ADC de 24 bits.               
	for (i=0;i<24;i++){                     
		saca_uno(&PORTD,1);                     
		count = count<<1;           				
		saca_cero(&PORTD,1);             
		if(uno_en_bit(&PIND,0)) count++;       
	}

	saca_uno(&PORTD,1);                     
	count = count^0x800000;         
	saca_cero(&PORTD,1);              	
	
	return count;                            
}

float read_average(uint8_t times) {
	float sum = 0;
	for (uint8_t i = 0; i < times; i++) {
		sum += Hx711_ReadCount();
	}
	return sum / times;
}

float get_value(uint8_t times) {
	return read_average(times) - OFFSET;
}

float get_units(uint8_t times) {
	return get_value(times) / SCALE;
}

void tare(uint8_t times) {
	double sum = read_average(times);
	OFFSET = sum;
}

void Hx711_Calibration(){
	float knowWeight = 295.0;
	
	float scale = get_value(100) / knowWeight;
	
	printValuesWithDecimal(scale);
}


// Funciones I2C.
void I2C_Init()
{
	// Inicializar reloj de I2C.
	TWSR = 0;                         		// Sin prescaler.
	TWBR = ((F_CPU/SCL_CLOCK)-16)/2;
}

void I2C_Start()
{
	// Borrar el indicador de interrupción TWI, poner la condición de inicio en SDA, habilitar TWI.
	TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN);
	
	while(!(TWCR & (1<<TWINT)));
}

uint8_t I2C_Read_Acknoledgement() {
	TWCR = (1<<TWEN)|(1<<TWINT)|(1<<TWEA);
	while (!(TWCR & (1<<TWINT)));
	
	return TWDR;
}

uint8_t I2C_Read_Not_Acknoledgement() {
	TWCR = (1<<TWINT)|(1<<TWEN);
	while (!(TWCR & (1<<TWINT)));
	
	return TWDR;
}

void I2C_Write(uint8_t data)
{
	TWDR = data;    						// Añadir los datos a TWDR.
	TWCR = (1<<TWINT)|(1<<TWEN);    		// Borrar indicador de interrupción TWI, habilitar TWI.
	while (!(TWCR & (1<<TWINT)));			// Esperar hasta que se transmita el byte TWDR completo
}

void I2C_Stop() {
	// Borrar el indicador de interrupción TWI, poner la condición de parada en SDA, habilitar TWI.
	TWCR= (1<<TWINT)|(1<<TWEN)|(1<<TWSTO);
	while(!(TWCR & (1<<TWSTO)));  			// Esperar hasta que se transmita la condición de parada
}


// Funciones DS3231.
uint8_t  BCD_To_DEC(uint8_t BCD_value)
{
	return (BCD_value >> 4) * 10 + (BCD_value & 0x0F);
}

uint8_t DEC_To_BCD (uint8_t decimal_value)
{
	return ((decimal_value / 10) << 4) + (decimal_value % 10);
}

void SetTimeDate(uint8_t _minutes, uint8_t _hours, uint8_t _date, uint8_t _month, uint8_t _year){
	I2C_Start();
	I2C_Write(0xD0);
	I2C_Write(0);
	I2C_Write(DEC_To_BCD(0)); 					// Segundos.
	I2C_Write(DEC_To_BCD(_minutes)); 			// Minutos.
	I2C_Write(DEC_To_BCD(_hours)); 				// Hora.
	I2C_Write(1); 								// Ignorar día de la semana.
	I2C_Write(DEC_To_BCD(_date)); 				// Día.
	I2C_Write(DEC_To_BCD(_month));				// Mes.
	I2C_Write(DEC_To_BCD(_year)); 				// Año.
	I2C_Stop();
}

void readTimeDate(){
	// Comenzar a leer.
	I2C_Start();
	I2C_Write(0xD0);
	I2C_Write(0);
	I2C_Stop();
	
	// Leer información.
	I2C_Start();
	I2C_Write(0xD1);
	seconds = BCD_To_DEC(I2C_Read_Acknoledgement());
	minutes = BCD_To_DEC(I2C_Read_Acknoledgement());
	hours = BCD_To_DEC(I2C_Read_Acknoledgement());
	I2C_Read_Acknoledgement();
	date = BCD_To_DEC(I2C_Read_Acknoledgement());
	month = BCD_To_DEC(I2C_Read_Acknoledgement());
	year = BCD_To_DEC(I2C_Read_Not_Acknoledgement());
	I2C_Stop();
	
	// Guardar valores en variables de comprobación de cambio.
	dateCombination = (year << 12) | (month << 4) | (date << 0);
	hourCombination = (hours << 8) | (minutes << 0);
}

void updateTimeDate(){
	unsigned long int dateCombinationCopy = dateCombination;
	unsigned long int hourCombinationCopy = hourCombination;
	
	readTimeDate();
	
	if(dateCombination != dateCombinationCopy){
		pastAlarmHours = 100;
		pastAlarmMinutes = 100;
		
		LCD_wr_instruction(0b10000000);	
		printValues8BitsTimeFormat(date);
		LCD_wr_string("/");
		printValues8BitsTimeFormat(month);
		LCD_wr_string("/");
		printValues8BitsTimeFormat(year);
	}
	
	if(hourCombination != hourCombinationCopy){
		pastAlarmHours = 100;
		pastAlarmMinutes = 100;
		
		LCD_wr_instruction(0b10001001);	
		printValues8BitsTimeFormat(hours);
		LCD_wr_string(":");
		printValues8BitsTimeFormat(minutes);
	}
}


// Pantallas.
void showMainScreen(){
	checkAlarms();
	
	// Actualizar datos de fechas.
	readTimeDate();
	
	// Fecha.
	LCD_wr_instruction(LCD_Cmd_Clear);			
	LCD_wr_instruction(0b10000000);			
	printValues8BitsTimeFormat(date);
	LCD_wr_string("/");
	printValues8BitsTimeFormat(month);
	LCD_wr_string("/");
	printValues8BitsTimeFormat(year);
		
	// Hora.
	LCD_wr_instruction(0b10001001);			
	printValues8BitsTimeFormat(hours);
	LCD_wr_string(":");
	printValues8BitsTimeFormat(minutes);
		
	// Instrucciones.
	LCD_wr_instruction(0b11000000);			
	LCD_wr_string("Agen  Peso  Dar");
	

    // Checar botones.
    while(1){
		// Actualizar datos de la pantalla main.
		updateTimeDate();
		checkAlarms();
	
        // Botones.
        if(cero_en_bit(&PINB, 0)){
            // Traba.
            _delay_ms(50);
            while(cero_en_bit(&PINB, 0));
            _delay_ms(50);

            showAgendOptions();
        }
		else if(cero_en_bit(&PINB, 1)){
			// Traba.
			_delay_ms(50);
			while(cero_en_bit(&PINB, 1));
			_delay_ms(50);

			showWeightScreen();
		}
        else if(cero_en_bit(&PINB, 2)){
            // Traba.
            _delay_ms(50);
            while(cero_en_bit(&PINB, 2));
            _delay_ms(50);

            showGiveFoodScreen();
        }
    }
}

void showWeightScreen(){
	checkAlarms();
	
	LCD_wr_instruction(LCD_Cmd_Clear);		
	LCD_wr_instruction(0b10000000);		
	LCD_wr_string("   Peso:");
	float pastFoodAmount  = get_units(10);
	LCD_wr_instruction(0b10001001);
	printValues(pastFoodAmount);
	
	// Instrucciones.
	LCD_wr_instruction(0b11000000);
	LCD_wr_string("Agen  Hora  Dar");
	

	// Checar botones.
	while(1){
		// Actualizar datos del peso.
		currentFoodAmount = get_units(5);
		checkAlarms();
		
		if((int)currentFoodAmount != (int)pastFoodAmount){
			pastFoodAmount = currentFoodAmount;
			LCD_wr_instruction(0b10001001);
			LCD_wr_string("            ");
			LCD_wr_instruction(0b10001001);
			printValues(pastFoodAmount);
		}
		
		// Botones.
		if(cero_en_bit(&PINB, 0)){
			// Traba.
			_delay_ms(50);
			while(cero_en_bit(&PINB, 0));
			_delay_ms(50);

			showAgendOptions();
		}
		else if(cero_en_bit(&PINB, 1)){
			// Traba.
			_delay_ms(50);
			while(cero_en_bit(&PINB, 1));
			_delay_ms(50);

			showMainScreen();
		}
		else if(cero_en_bit(&PINB, 2)){
			// Traba.
			_delay_ms(50);
			while(cero_en_bit(&PINB, 2));
			_delay_ms(50);

			showGiveFoodScreen();
		}
	}
}

void showAgendOptions(){
	checkAlarms();
	
	// Fecha.
	LCD_wr_instruction(LCD_Cmd_Clear);		
	LCD_wr_instruction(0b10000000);		
	LCD_wr_string("  --Opciones--  ");

    LCD_wr_instruction(0b11000000);		
	LCD_wr_string("Ret   Del   Add");

    // Checar botones.
    while(1){
		checkAlarms();
		
        if(cero_en_bit(&PINB, 0)){
            // Traba.
            _delay_ms(50);
            while(cero_en_bit(&PINB, 0));
            _delay_ms(50);

            showMainScreen();
        }
        else if(cero_en_bit(&PINB, 1)){
            // Traba.
            _delay_ms(50);
            while(cero_en_bit(&PINB, 1));
            _delay_ms(50);

            showDeleteAlarms();
        }
        else if(cero_en_bit(&PINB, 2)){
            // Traba.
            _delay_ms(50);
            while(cero_en_bit(&PINB, 2));
            _delay_ms(50);

            showAddAlarms();
        }
    }
}

void showDeleteAlarms(){
	checkAlarms();
	
    if(searchAlarms() == 0){
        // Error.
        LCD_wr_instruction(LCD_Cmd_Clear);		
        LCD_wr_instruction(0b10000000);			
        LCD_wr_string("  ==Error==  ");
        LCD_wr_instruction(0b11000000);				
        LCD_wr_string("No hay alarmas");

        _delay_ms(1000);

        showAgendOptions();
    }

    // Mostrar mensajes estáticos.
    LCD_wr_instruction(LCD_Cmd_Clear);			
    LCD_wr_instruction(0b10000000);				
    LCD_wr_string("Alarma: ");
    LCD_wr_instruction(0b11000000);
    LCD_wr_string("Ret  Del  Next");

    // Variables.
    uint8_t hoursRegister = 0, minutesRegister = 0;

    // Para cada alarma.
    for(int i=0;i<ALARM_BITS;i+=2){
        hoursRegister = EEPROM_read(i);
        if(hoursRegister != 255){
            minutesRegister = EEPROM_read(i+1);

            LCD_wr_instruction(0b10001000);				
            printValues8BitsTimeFormat(hoursRegister);
            LCD_wr_string(":");
            printValues8BitsTimeFormat(minutesRegister);
            

            // Checar botones.
            while(1){
				checkAlarms();
				
                // Return.
                if(cero_en_bit(&PINB, 0)){
                    // Traba.
                    _delay_ms(50);
                    while(cero_en_bit(&PINB, 0));
                    _delay_ms(50);

                    showAgendOptions();
                }
                // Delete.
                else if(cero_en_bit(&PINB, 1)){
                    // Traba.
                    _delay_ms(50);
                    while(cero_en_bit(&PINB, 1));
                    _delay_ms(50);

                    EEPROM_write(i, 255);
                    EEPROM_write(i+1, 255);

                    // Error.
                    LCD_wr_instruction(LCD_Cmd_Clear);		
                    LCD_wr_instruction(0b10000000);			
                    LCD_wr_string("   ==Exito==   ");
                    LCD_wr_instruction(0b11000000);			
                    LCD_wr_string("   Bye alarma  ");

                    showDeleteAlarms();
                }
                // Next.
                else if(cero_en_bit(&PINB, 2)){
                    // Traba.
                    _delay_ms(50);
                    while(cero_en_bit(&PINB, 2));
                    _delay_ms(50);

                    if(i+2 >= ALARM_BITS){
                        i = -2;
                    }
					
					break;
                }
            }
        }
    }
	
	showDeleteAlarms();
}

void showAddAlarms(){
	checkAlarms();
	
    if(searchAlarms() == ALARM_BITS/2){
        // Error.
        LCD_wr_instruction(LCD_Cmd_Clear);			
        LCD_wr_instruction(0b10000000);		
        LCD_wr_string("  ==Error==  ");
        LCD_wr_instruction(0b11000000);			
        LCD_wr_string(" Ya hay alarmas ");
		_delay_ms(1000);

        LCD_wr_instruction(0b11000000);			
        LCD_wr_string(" Borre algunas ");
		_delay_ms(1000);

        showAgendOptions();
    }

    // Mostrar mensajes estáticos.
    LCD_wr_instruction(LCD_Cmd_Clear);			
    LCD_wr_instruction(0b10000000);				
    LCD_wr_string("Alarma: ");
    LCD_wr_instruction(0b11000000);
    LCD_wr_string("Ret  Add  Next");

    // Variables.
    volatile uint8_t hoursRegister = 0, hoursCont = 0, minutesCont = 0;

    // Para cada alarma.
    for(volatile int i=0, j=0;i<ALARM_BITS;i+=2){
        hoursRegister = EEPROM_read(i);
        if(hoursRegister == 255){
            LCD_wr_instruction(0b10001000);				
            printValues8BitsTimeFormat(hoursCont);
            LCD_wr_string(":");
            printValues8BitsTimeFormat(minutesCont);
            

            // Checar botones.
            while(1){
				checkAlarms();
				
                // Return.
                if(cero_en_bit(&PINB, 0)){
                    // Traba.
                    _delay_ms(50);
                    while(cero_en_bit(&PINB, 0));
                    _delay_ms(50);

                    showAgendOptions();
                }
                // Add.
                else if(cero_en_bit(&PINB, 1)){
                    // Traba.
                    _delay_ms(50);
                    while(cero_en_bit(&PINB, 1));
                    _delay_ms(50);

					if(j==0){
						j++;
						i-=2;
						break;
					}
					else{
						EEPROM_write(i, hoursCont);
						EEPROM_write(i+1, minutesCont);

						// Error.
						LCD_wr_instruction(LCD_Cmd_Clear);	
						LCD_wr_instruction(0b10000000);				
						LCD_wr_string("   ==Exito==   ");
						LCD_wr_instruction(0b11000000);			
						LCD_wr_string("Nueva alarma :)");
						
						_delay_ms(1000);

						showAddAlarms();
					}
                }
                // Next.
                else if(cero_en_bit(&PINB, 2)){
                    // Traba.
                    _delay_ms(50);
                    while(cero_en_bit(&PINB, 2));
                    _delay_ms(50);

                    if(j==0){
						i = -2;
						if(hoursCont+1 > 23){
							hoursCont = 0;
						}
						else{
							hoursCont++;
						}
					}
					else{
						i = -2;
						if(minutesCont+1 > 59){
							minutesCont = 0;
						}
						else{
							minutesCont++;
						}
					}
					break;
                }
            }
        }
    }
}

uint8_t searchAlarms(){
    uint8_t validRegisterCounter = 0, hoursRegister = 0;

    for(uint8_t i=0;i<ALARM_BITS;i+=2){
        hoursRegister = EEPROM_read(i);
        if(hoursRegister != 255){
            validRegisterCounter++;
        }
    }

    return validRegisterCounter;
}

void showGiveFoodScreen(){
	checkAlarms();
	
	// Mostrar mensajes estáticos.
	LCD_wr_instruction(LCD_Cmd_Clear);		
	LCD_wr_instruction(0b11000000);			
	LCD_wr_string("Ret    Dar    +");
	LCD_wr_instruction(0b10001001);
	LCD_wr_string("g");
	
	
	for(int i=10;i<maxFoodAmountToGive;i+=10){
		if(i < 100){
			LCD_wr_instruction(0b10000110);
			LCD_wr_string(" ");
		}
		else if(i >= 100){
			LCD_wr_instruction(0b10000110);
		}
		printValues((long)i);
		
			
		// Checar botones.
		while(1){
			checkAlarms();
			
			// Return.
			if(cero_en_bit(&PINB, 0)){
				// Traba.
				_delay_ms(50);
				while(cero_en_bit(&PINB, 0));
				_delay_ms(50);

				showMainScreen();
			}
			// Dar comida.
			else if(cero_en_bit(&PINB, 1)){
				// Traba.
				_delay_ms(50);
				while(cero_en_bit(&PINB, 1));
				_delay_ms(50);
					
				showGivingFoodScreen(i);
			}
			// Más comida.
			else if(cero_en_bit(&PINB, 2)){
				// Traba.
				_delay_ms(50);
				while(cero_en_bit(&PINB, 2));
				_delay_ms(50);
					
				break;
			}
			
			if(i+10 >= maxFoodAmountToGive){
				i = 0;
			}
		}
	}
	
	showGiveFoodScreen();
}

void showGivingFoodScreen(int foodAmount){
	LCD_wr_instruction(LCD_Cmd_Clear);		
	LCD_wr_instruction(0b10000000);				
	LCD_wr_string("=====Espere=====");
	
	float pastAmountFood = get_units(50);
	
	if(pastAmountFood < 10 || pastAmountFood < foodAmount){
		LCD_wr_instruction(LCD_Cmd_Clear);		
		LCD_wr_instruction(0b10000000);			
		LCD_wr_string("=====Error=====");
		
		LCD_wr_instruction(0b11000000);				
		LCD_wr_string("Hay poca comida");
		_delay_ms(1000);
		
		LCD_wr_instruction(0b11000000);				
		LCD_wr_string("                ");
		LCD_wr_instruction(0b11000000);				
		LCD_wr_string("Agregue + comida");
		_delay_ms(1000);
		
		showWeightScreen();
	}
	
	currentFoodAmount = get_units(100);

	// Mostrar mensajes estáticos.
	LCD_wr_instruction(0b11000000);
	LCD_wr_string("Dando comida...");
	
	// Dar comida.
	while(currentFoodAmount > pastAmountFood-foodAmount){
		OCR0 = 14;
		_delay_ms(400);
		OCR0 = 10;
		_delay_ms(1000);
		currentFoodAmount = get_units(1);
	}
	OCR0 = 10;
	
	
	
	// Mostrar mensajes estáticos.
	LCD_wr_instruction(LCD_Cmd_Clear);			
	LCD_wr_instruction(0b10000000);			
	LCD_wr_string("=====Exito=====");
	LCD_wr_instruction(0b11000000);
	LCD_wr_string("Tortuguita feli");
	_delay_ms(1000);
	
	showMainScreen();
}

void checkAlarms(){
	if(searchAlarms() <= 0){
		return;
	}	
	
	readTimeDate();
	if(pastAlarmHours == hours && pastAlarmMinutes == minutes){
		return;
	}
	
	uint8_t hoursRegister1 = 0, minutesRegister1 = 0;

	for(uint8_t i=0;i<ALARM_BITS;i+=2){
		hoursRegister1 = EEPROM_read(i);
		if(hoursRegister1 != 255){
			minutesRegister1 = EEPROM_read(i+1);
			
			if(hoursRegister1 == hours && minutesRegister1 == minutes){
				pastAlarmHours = hours;
				pastAlarmMinutes = minutes;
				
				showGivingFoodScreen(30);
			}
		}
	}
	
	return;
}
	

// Prints.
void printValues8BitsTimeFormat(uint8_t valor){
	uint8_t num[8] = {0};
	uint8_t i = 0;
	
	if(valor == 0){
		LCD_wr_string("00");
		return;
	}
	
	while(valor > 0){
		num[i] = valor%10;
		valor /= 10;
		i++;
	}
	
	int validacion = 0;
	for(int i=7;i>=0;i--){
		if(validacion == 1){
			LCD_wr_char(num[i] + 48);
		}
		if(num[i] != 0 && validacion == 0){
			if(i==0){
				i+=2;
			}
			else if(i==1){
				i++;
			}
			validacion = 1;
		}
		
	}
}

void printValues8Bits(uint8_t valor){
	uint8_t num[8] = {0};
	uint8_t i = 0;
	
	while(valor > 0){
		num[i] = valor%10;
		valor /= 10;
		i++;
	}
	
	int validacion = 0;
	for(int i=7;i>=0;i--){
		if(num[i] != 0){
			validacion = 1;
		}
		else if(validacion == 1){
			LCD_wr_char(num[i] + 48);
		}
	}
}

void printValues(long valor){
	if(valor <= 0){
		LCD_wr_string("0");
		return;
	}
	
	uint8_t num[24] = {0};
	uint8_t i = 0;
	
	while(valor > 0){
		num[i] = valor%10;
		valor /= 10;
		i++;
	}
	
	int validacion = 0;
	for(int i=23;i>=0;i--){
		if(num[i] != 0){
			validacion = 1;
		}
		if(validacion == 1){
			LCD_wr_char(num[i] + 48);
		}
	}
}

void printValuesWithDecimal(float valor){
	// Valor entero.
	long int valorInt = valor;
	
	// Valor de los decimales en entero.
	long int decimalValorInt = (valor-valorInt)*100;
	
	if(valor <= 0){
		LCD_wr_string("0");
		return;
	}
	
	LCD_wr_instruction(LCD_Cmd_Clear);			// Limpiar el display
	LCD_wr_instruction(0b10000000);				// Posición cero.
	
	uint8_t num[10] = {0};
	uint8_t decimals[10] = {0};
	uint8_t i = 0;
	
	// Obtener valores de la parte entera.
	while(valorInt > 0){
		num[i] = valorInt%10;
		valorInt /= 10;
		i++;
	}
	
	i=0;
	
	// Obtener valores de la parte decimal.
	while(decimalValorInt > 0){
		decimals[i] = decimalValorInt%10;
		decimalValorInt /= 10;
		i++;
	}
	
	// Imprimir valores de la parte entera.
	int validacion = 0;
	for(int i=9;i>=0;i--){
		if(num[i] != 0){
			validacion = 1;
		}
		if(validacion == 1){
			LCD_wr_char(num[i] + 48);
		}
	}
	
	// Imprimir punto.
	LCD_wr_char(46);
	
	// Imprimir valores de la parte decimal.
	validacion = 0;
	for(int i=9;i>=0;i--){
		if(decimals[i] != 0){
			validacion = 1;
		}
		if(validacion == 1){
			LCD_wr_char(decimals[i] + 48);
		}
	}
	
	_delay_ms(300);
}



// ----------------------- Código principal -----------------------

int main(void)
{		
	// Inicializacion del LCD
	   LCD_init();
	   LCD_wr_instruction(0b10000000);

    // Configuracion del puerto B
		DDRB = 0b00001000;
		PORTB = 0b00000111;
	   
	// Configuracion del puerto D
		DDRD = 0b00110010;
		PORTD = 0b00000000;
	
	// Inicializar I2C.
		I2C_Init();
		
	// Obtener tiempo.
		readTimeDate();
		
	// Iniciar motor.
		TCNT0 = 0;
		OCR0 = 10;
		TCCR0 = 0b01101100;
		
	// Mostrar pantalla principal.
		showMainScreen();
}
