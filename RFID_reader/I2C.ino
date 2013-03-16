#include <EEPROM.h>
#include <Wire.h>

#define I2C_ADDR_EEPROM          1000

#define I2C_MSG_RESET            1
#define I2C_MSG_OK               2
#define I2C_MSG_ERROR            3
#define I2C_MSG_CHANGE_ADDR      4
#define I2C_MSG_CARD_READ        5
#define I2C_MSG_GRANTED          6
#define I2C_MSG_DENIED           7
#define I2C_MSG_VALIDATE         8
#define I2C_MSG_LOG_DATA         9
#define I2C_MSG_ADD_CARD         10
#define I2C_MSG_DEL_CARD         11
#define I2C_MSG_MEM_FULL         12
#define I2C_MSG_NOT_DOUND         13

volatile uint8_t I2CStatus;

void I2CSetup(){
  uint8_t addr=EEPROM.read(I2C_ADDR_EEPROM);
  if (addr<3 || addr>0x77) addr=0x77;
  Wire.begin(addr);
  Wire.onReceive(I2CReceive);
  Wire.write(I2C_MSG_RESET);
  //Wire.write(I2C_MSG_OK);
}

void I2CReceive(int numBytes){
  while (Wire.available()){
    uint8_t b=Wire.read();
    switch (b){
    case I2C_MSG_CHANGE_ADDR:{
      unsigned long timeOut=millis()+1000;
      uint8_t a;
      do a=Wire.available(); while (millis()<timeOut || a==0);
      if (a==0){
        Wire.write(I2C_MSG_ERROR);
        break;
      }
      uint8_t addr=Wire.read();
      if (addr>=3 || addr<=0x77){
        EEPROM.write(I2C_ADDR_EEPROM,addr);
        Wire.begin(addr);
        Wire.write(I2C_MSG_OK);
      } else Wire.write(I2C_MSG_ERROR);
    } break;
    case I2C_MSG_VALIDATE:{
      unsigned long timeOut=millis()+1000;
      uint8_t a;
      do a=Wire.available(); while (millis()<timeOut || a==0);
      if (a==0){
        I2CStatus=I2C_MSG_ERROR;
        break;
      }
      I2CStatus=Wire.read();
    } break;
    case I2C_MSG_ADD_CARD:{
      uint8_t c[5];
      bool error=false;
      for (uint8_t i=0;i<5;i++){
        unsigned long timeOut=millis()+1000;
        uint8_t a;
        do a=Wire.available(); while (millis()<timeOut || a==0);
        if (a==0){
          error=true;
          break;
        }
        c[i]=Wire.read();
      }
      if (error){
        Wire.write(I2C_MSG_ERROR);
        break;
      }
      int8_t result=addCard(c);
      if (result>=0){
        Wire.write(I2C_MSG_ADD_CARD);
        Wire.write(result);
      } else Wire.write(I2C_MSG_MEM_FULL);
    } break;
    case I2C_MSG_DEL_CARD:{
      uint8_t c[5];
      bool error=false;
      for (uint8_t i=0;i<5;i++){
        unsigned long timeOut=millis()+1000;
        uint8_t a;
        do a=Wire.available(); while (millis()<timeOut || a==0);
        if (a==0){
          error=true;
          break;
        }
        c[i]=Wire.read();
      }
      if (error){
        Wire.write(I2C_MSG_ERROR);
        break;
      }
      int8_t slot=validateCard(c);
      if (slot>=0){
        for (uint8_t i=0;i<5;i++) c[i]=0;
        saveCard(slot,c);
        Wire.write(I2C_MSG_DEL_CARD);
        Wire.write(slot);
      } else Wire.write(I2C_MSG_NOT_DOUND);
    } break;
    default:
      Wire.write(I2C_MSG_ERROR);
      break;
    }
  }
}

bool I2CValidate(uint8_t card[]){
  Wire.write(I2C_MSG_VALIDATE);
  for (uint8_t i=0;i<5;i++)
    Wire.write(card[i]);
  I2CStatus=0;
  unsigned long timeOut=millis()+1000;
  while (millis()<timeOut || I2CStatus==0);
  if (I2CStatus==I2C_MSG_GRANTED) return true; else return false;
}

void I2CLogData(uint8_t card[],bool granted){
  Wire.write(I2C_MSG_LOG_DATA);
  for (uint8_t i=0;i<5;i++)
    Wire.write(card[i]);
  if (granted) Wire.write(I2C_MSG_GRANTED); else Wire.write(I2C_MSG_DENIED);
}
