#include "mp3_functions.h"

void mp3_decoder_initialize(){

    /* Configuring Required Pins */
    gpio_s chip_select = gpio__construct_as_output(0, 26);
    gpio_s data_select = gpio__construct_as_output(1, 31);
    gpio_s dreq = gpio__construct_as_input(1, 20);

    ssp2_pin_initialize();

    ssp2__initialize(1000);
    
    
    gpio_set(chip_select);
    gpio_set(data_select);

    


}

void cs(bool flag){
    if(flag)
        LPC_GPIO0->CLR = (1 << 26);
    
    else
        LPC_GPIO0->SET = (1 << 26);
}

void sj2_write_to_decoder(uint8_t address, uint16_t data){
    cs(true);
    ssp2__exchange_byte(0x2);
    ssp2__exchange_byte(address);
    ssp2__exchange_byte((data >> 8) & 0xFF);
    ssp2__exchange_byte((data >> 0) & 0xFF);
    cs(false);
}

