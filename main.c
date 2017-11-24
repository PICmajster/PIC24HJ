/*****************************************************************************
  FileName:        main.c
  Processor:       PIC24HJ128GP502
  Compiler:        XC16 ver 1.30
  Created on:      14 listopada 2017, 09:37
  Description:     SPI MASTER
 ******************************************************************************/

#include "xc.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h> /*dyrektywy uint8_t itp*/
#include <string.h>
#include "ustaw_zegar.h" /*tutaj m.in ustawione FCY*/
#include <libpic30.h> /*dostep do delay-i,musi byc po zaincludowaniu ustaw_zegar.h*/
#include "dogm204.h"

void spi_config_master(void);
uint8_t Transfer_SPI(uint8_t bajt);

volatile uint8_t SPI_receive_data ;

#define LED1_TOG PORTA ^= (1<<_PORTA_RA1_POSITION) /*zmienia stan bitu na przeciwny*/

int main(void) {
    ustaw_zegar(); /*odpalamy zegar wewnetrzny na ok 40MHz*/
    __delay_ms(50); /*stabilizacja napiec*/
    /*
     * wylaczamy ADC , wszystkie piny chcemy miec cyfrowe
     * pojedynczo piny analogowe wylaczamy w rejestrze AD1PCFGL 
     * Po resecie procka piny oznaczone ANx sa w trybie analogowych wejsc.
     */
    PMD1bits.AD1MD = 1; /*wylaczamy ADC*/
    /* 
     * ustawiamy wszystkie piny analogowe (oznacznone ANx) jako cyfrowe
     * do zmiany mamy piny AN0-AN5 i AN9-AN12 co daje hex na 16 bitach = 0x1E3F
     */
    AD1PCFGL = 0x1E3F;
    TRISAbits.TRISA1 = 0 ; // RA1 jako wyjscie tu mamy LED
/*remaping pinow na potrzeby SPI
 SDO --> pin 11
 SDI --> pin 14
 SCK --> pin 15
 */
   
    RPOR2bits.RP4R = 7;     /*inaczej _RP4R = 7*/
    RPINR20bits.SDI1R = 5;  /*inaczej _SDI1R = 5*/
    RPOR3bits.RP6R = 8;     /*inaczej _RP6R = 8*/
    
    WlaczLCD(); /*LCD Init*/
    UstawKursorLCD(1,6);
    WyswietlLCD("TEST SPI");
    UstawKursorLCD(2,1);
    WyswietlLCD("Send data :");
    UstawKursorLCD(3,1);
    WyswietlLCD("Receive data :"); 
        
    spi_config_master();
    SPI_receive_data = 0;
    
    while (1) {
        
        if(Transfer_SPI(49) == 50) LED1_TOG  ; /*Send and Receive data to be transmitted*/
        
        WyswietlLCD("  ");
        
        if(SPI_receive_data){ /*jesli cos odebrane*/
            UstawKursorLCD(3,16);
            lcd_int(SPI_receive_data); /*daj na ekran odebrany bajt*/
            SPI_receive_data = 0;
        }            
        __delay_ms(1000) ; /*bee*/
                      
    }

}
/*konfiguracja SPI dla Mastera*/
void spi_config_master(void) {
     
IFS0bits.SPI1IF = 0;                  /*Clear the Interrupt Flag*/
IEC0bits.SPI1IE = 0;                  /*Disable the Interrupt*/
/*Set clock SPI on SCK, 40 MHz / (4*2) = 5 MHz*/
SPI1CON1bits.PPRE = 0b10;             /*Set Primary Prescaler 1:4*/
SPI1CON1bits.SPRE = 0b110;            /*Set Secondary Prescaler 2:1*/

SPI1CON1bits.MODE16 = 0;              /*Communication is word-wide (8 bits)*/
SPI1CON1bits.MSTEN = 1;               /*Master Mode Enabled*/
SPI1STATbits.SPIEN = 1;               /*Enable SPI Module*/
SPI1BUF = 0x0000;
IFS0bits.SPI1IF = 0;                  /*Clear the Interrupt Flag*/
IEC0bits.SPI1IE = 1;                  /*Enable the Interrupt*/
}

uint8_t Transfer_SPI(uint8_t bajt){
    SPI1BUF = bajt; /*wysylamy bajt*/
    UstawKursorLCD(2,16);
    lcd_int(bajt); /*daj na ekran wysylany bajt*/
    while(SPI1STATbits.SPITBF); /*czekamy na zakonczenie transferu*/
    return SPI1BUF ; /*odczytujemy dane*/
}

/*Obsluga wektora przerwania dla SPI1 , przerwanie zglaszane po zakonczeniu transferu*/
void __attribute__((interrupt, no_auto_psv))_SPI1Interrupt(void)
{
    SPI_receive_data = SPI1BUF; /*pobieramy dane przychodzace*/

    IFS0bits.SPI1IF = 0 ; /*Clear SPI1 Interrupt Flag*/
}

