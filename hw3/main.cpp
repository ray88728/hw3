#include "mbed.h"

#include "fsl_port.h"

#include "fsl_gpio.h"

#define UINT14_MAX        16383

// FXOS8700CQ I2C address

#define FXOS8700CQ_SLAVE_ADDR0 (0x1E<<1) // with pins SA0=0, SA1=0

#define FXOS8700CQ_SLAVE_ADDR1 (0x1D<<1) // with pins SA0=1, SA1=0

#define FXOS8700CQ_SLAVE_ADDR2 (0x1C<<1) // with pins SA0=0, SA1=1

#define FXOS8700CQ_SLAVE_ADDR3 (0x1F<<1) // with pins SA0=1, SA1=1

// FXOS8700CQ internal register addresses

#define FXOS8700Q_STATUS 0x00

#define FXOS8700Q_OUT_X_MSB 0x01

#define FXOS8700Q_OUT_Y_MSB 0x03

#define FXOS8700Q_OUT_Z_MSB 0x05

#define FXOS8700Q_M_OUT_X_MSB 0x33

#define FXOS8700Q_M_OUT_Y_MSB 0x35

#define FXOS8700Q_M_OUT_Z_MSB 0x37

#define FXOS8700Q_WHOAMI 0x0D

#define FXOS8700Q_XYZ_DATA_CFG 0x0E

#define FXOS8700Q_CTRL_REG1 0x2A

#define FXOS8700Q_M_CTRL_REG1 0x5B

#define FXOS8700Q_M_CTRL_REG2 0x5C

#define FXOS8700Q_WHOAMI_VAL 0xC7


I2C i2c( PTD9,PTD8);

Serial pc(USBTX, USBRX);

DigitalOut led1(LED1);

InterruptIn btn(SW2);

Timer for_debounce;

Ticker ledTicker;

Ticker vectorTicker;

EventQueue eventQueue;

EventQueue readQueue;

Thread Thread1;

Thread Thread2; 

int m_addr = FXOS8700CQ_SLAVE_ADDR1;

uint8_t who_am_i, data[2], res[6];

int16_t acc16;

float t[3];

float X[100];

float Y[100];

float Z[100];

int i,j;

int tilt[101];

float stationary_x;

float stationary_y;

float stationary_z;

void FXOS8700CQ_readRegs(int addr, uint8_t * data, int len);

void FXOS8700CQ_writeRegs(uint8_t * data, int len);

void read_vector(void);

void log_vector(void);

void blink_led1(void);

void logger(void);

int main() {
   EventQueue queue;

   Thread1.start(callback(&eventQueue, &EventQueue::dispatch_forever));

   Thread2.start(callback(&readQueue, &EventQueue::dispatch_forever));

   led1=1;

   btn.rise(&logger);

   pc.baud(115200);

   // Enable the FXOS8700Q

   FXOS8700CQ_readRegs( FXOS8700Q_CTRL_REG1, &data[1], 1);

   data[1] |= 0x01;

   data[0] = FXOS8700Q_CTRL_REG1;

   FXOS8700CQ_writeRegs(data, 2);

   // Get the slave address

   FXOS8700CQ_readRegs(FXOS8700Q_WHOAMI, &who_am_i, 1);

    read_vector();
    stationary_x=t[0];
    stationary_y=t[1];
    stationary_z=t[2];
    while(true){
        ;
    }
}


void FXOS8700CQ_readRegs(int addr, uint8_t * data, int len) {

   char t = addr;

   i2c.write(m_addr, &t, 1, true);

   i2c.read(m_addr, (char *)data, len);

}


void FXOS8700CQ_writeRegs(uint8_t * data, int len) {

   i2c.write(m_addr, (char *)data, len);

}

void read_vector(){
        
        FXOS8700CQ_readRegs(FXOS8700Q_OUT_X_MSB, res, 6);

        acc16 = (res[0] << 6) | (res[1] >> 2);

        if (acc16 > UINT14_MAX/2)

            acc16 -= UINT14_MAX;

        t[0] = ((float)acc16) / 4096.0f;


        acc16 = (res[2] << 6) | (res[3] >> 2);

        if (acc16 > UINT14_MAX/2)

            acc16 -= UINT14_MAX;

        t[1] = ((float)acc16) / 4096.0f;


        acc16 = (res[4] << 6) | (res[5] >> 2);

        if (acc16 > UINT14_MAX/2)

            acc16 -= UINT14_MAX;

        t[2] = ((float)acc16) / 4096.0f;
}

void log_vector(void){

    while(i<=99){

        read_vector();

        X[i]=t[0];

        Y[i]=t[1];
        
        Z[i]=t[2];
        
        if( Z[i] < stationary_z/sqrt(2) )
            tilt[i] = 1;
        else
        {
            tilt[i] = 0;
        }

        i++;


        wait(0.1);
    }
            
    for (j = 0; j < i; j++){

        pc.printf("%1.4f\r\n", X[j]);
        wait(0.1);
    }

    for (j = 0; j < i; j++){

        pc.printf("%1.4f\r\n", Y[j]);
        wait(0.1);
    }
    for (j = 0; j < i; j++){

        pc.printf("%1.4f\r\n", Z[j]);
        wait(0.1);
    }

    for(j = 0; j < i; j++){
           pc.printf("%d\r\n",tilt[j]);
           wait(0.1);
    }
}

void logger(void){

    for_debounce.reset();

    for_debounce.start();

    i = 0;

    readQueue.call(log_vector);   

//    vectorTicker.attach(readQueue.event(read_vector), 0.1); 

    ledTicker.attach(eventQueue.event(&blink_led1), 0.2); 
}


void blink_led1() {

  // this runs in the normal priority thread
    if(for_debounce.read()>10){
        led1=1;
    }
    else{
        led1 = !led1;
    }

}