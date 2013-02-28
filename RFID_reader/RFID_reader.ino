#include <EEPROM.h>

uint8_t cparity;
uint8_t numbers[5];

#define RFID_PIN        PIND
#define RFID_PIN_NUM    4  // Arduino PIN4
#define RFID_PIN_MASK   (1<<RFID_PIN_NUM)
#define GEN_PIN         PINB
#define GEN_PIN_NUM     3  // Arduino PIN11
#define GEN_PIN_MASK    (1<<GEN_PIN_NUM)
#define LED             13
#define ADM_BUTTON      2
#define SAVE_SLOTS      100

void setup() {
  TCCR2A=0b01000010;  // Toggle OC2A on Compare Match, Clear Timer on Compare Match
  TCCR2B=0b10000001;  // Force Output Compare A, No prescaling
  OCR2A=63;           // 16000000/125000/2-1
  DDRB|=GEN_PIN_MASK; // PB3 is output
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }
  // prints title with ending line break
  Serial.println("Hello"); 
  pinMode(4,INPUT);  // RFID input
  pinMode(LED,OUTPUT);
  pinMode(ADM_BUTTON,INPUT_PULLUP);  // Administrative button
  if (digitalRead(ADM_BUTTON)==LOW){
    uint8_t card[]={0,0,0,0,0};
    for (uint8_t i=0;i<SAVE_SLOTS;i++)
      saveCard(i,card);
  }
}

void wait(uint8_t t){
  uint8_t counter=0,last=0,next;
  do {
    next=GEN_PIN&GEN_PIN_MASK;
    if (next!=last) counter++;
    last=next;
  } while (counter<t);
}

uint8_t readBit(uint8_t time1,uint8_t time2){
  wait(time1);
  uint8_t counter=0,last=0,state1=RFID_PIN&RFID_PIN_MASK,state2;
  do {
    uint8_t next=GEN_PIN&GEN_PIN_MASK;
    if (next!=last) counter++;
    last=next;
    state2=RFID_PIN&RFID_PIN_MASK;
  } while (counter<=time2 && state1==state2);
  if (state1==state2) return 2;
  if (state2==0) return 0; else return 1;
}

int8_t read4Bits(uint8_t time1,uint8_t time2){
  uint8_t number=0,parity=0;
  for (uint8_t i=4;i>0;i--){
    uint8_t bit=readBit(time1,time2);
    if (bit==2) return -1;
    number=(number<<1)+bit;
    parity+=bit;
  }
  uint8_t bit=readBit(time1,time2);
  if (bit==2 || (parity&1)!=bit) return -1;
  cparity^=number;
  return number;
}

uint8_t readCard(){
  loop_until_bit_is_clear(RFID_PIN, RFID_PIN_NUM);
  loop_until_bit_is_set(RFID_PIN, RFID_PIN_NUM);
  uint8_t counter=0,last=0,next;
  do {
    next=GEN_PIN&GEN_PIN_MASK;
    if (next!=last) counter++;
    last=next;
  } while (bit_is_set(RFID_PIN, RFID_PIN_NUM) && counter<0xFF);
  if (counter==0xFF && bit_is_set(RFID_PIN, RFID_PIN_NUM)) return 1;
  uint8_t halfbit=counter,offset=counter>>1;
  if (readBit(offset,halfbit)!=1) return 1;
  for (uint8_t i=7;i>0;i--)
    if (readBit(halfbit+offset,halfbit)!=1) return 1;
  cparity=0;
  for (uint8_t i=0;i<5;i++){
    int8_t n1=read4Bits(halfbit+offset,halfbit),
           n2=read4Bits(halfbit+offset,halfbit);
    if (n1<0 || n2<0) return 1;
    numbers[i]=(n1<<4)+n2;
  }
  uint8_t cp=0;
  for (uint8_t i=4;i>0;i--){
    uint8_t bit=readBit(halfbit+offset,halfbit);
    if (bit==2) return 1;
    cp=(cp<<1)+bit;
  }
  if (cparity!=cp) return 1;
  if (readBit(halfbit+offset,halfbit)!=0) return 1;
  return 0;
}

bool loadCard(uint8_t s,uint8_t card[]){
  bool notEmpty=false;
  uint16_t addr=s*5;
  for (uint8_t i=0;i<5;i++){
    uint8_t b=EEPROM.read(addr+i);
    notEmpty=notEmpty||b;
    card[i]=b;
  }
  return notEmpty;
}

void saveCard(uint8_t s,uint8_t card[]){
  uint16_t addr=s*5;
  for (uint8_t i=0;i<5;i++)
    EEPROM.write(addr+i,card[i]);
}

int8_t addCard(uint8_t card[]){
  for (uint8_t i=0;i<SAVE_SLOTS;i++){
    uint8_t c[5];
    if (!loadCard(i,c)){
      saveCard(i,card);
      return i;
    }
  }
  return -1;
}

int8_t validateCard(uint8_t card[]){
  uint8_t c[5];
  for (uint8_t i=0;i<SAVE_SLOTS;i++){
    if (!loadCard(i,c)) continue;
    bool found=true;
    for (uint8_t j=0;j<5;j++) found=found&&(c[j]==card[j]);
    if (found) return i;
  }
  return -1;
}

void loop() {
  digitalWrite(LED,LOW);
  uint8_t result;
  do {
    result=readCard();
  } while (result!=0);
  digitalWrite(LED,HIGH);
  if (digitalRead(ADM_BUTTON)==LOW){
    if (validateCard(numbers)<0) addCard(numbers);
  }
  Serial.print("ACCESS ");
  if (validateCard(numbers)>=0) Serial.println("GRANTED"); else Serial.println("DENIED");
  Serial.print("Raw data: ");
  for (uint8_t i=0;i<5;i++) Serial.print(numbers[i],HEX);
  Serial.println();
  Serial.print("Customer ID: ");
  Serial.print(numbers[0],HEX);
  Serial.println();
  Serial.print("Card number: ");
  uint8_t numbersr[4];
  numbersr[0]=numbers[4];
  numbersr[1]=numbers[3];
  numbersr[2]=numbers[2];
  numbersr[3]=numbers[1];
  Serial.print(*(uint32_t*)(&numbersr),DEC);
  Serial.println();
}

/*
void loop(){
  uint8_t card[]={0,0,0,0,0};
  for (uint8_t i=0;i<SAVE_SLOTS;i++)
    saveCard(i,card);
  card[0]=0x11;
  card[1]=0x22;
  card[2]=0x33;
  card[3]=0x44;
  card[4]=0x55;
  Serial.println(addCard(card));
  card[0]=0x11;
  card[1]=0x22;
  card[2]=0x77;
  card[3]=0x44;
  card[4]=0x55;
  Serial.println(addCard(card));
  Serial.println(validateCard(card));
  card[2]=0x33;
  Serial.println(validateCard(card));
  card[2]=0x99;
  Serial.println(validateCard(card));
  while (1);
}
*/
