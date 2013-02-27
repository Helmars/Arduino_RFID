#define RFID_PIN        PIND
#define RFID_PIN_NUM    4  // Arduino PIN4
#define RFID_PIN_MASK   (1<<RFID_PIN_NUM)
#define GEN_PIN         PINB
#define GEN_PIN_NUM     3  // Arduino PIN11
#define GEN_PIN_MASK    (1<<GEN_PIN_NUM)
#define LED             13

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

void readRawData(){
  loop_until_bit_is_clear(RFID_PIN, RFID_PIN_NUM);
  loop_until_bit_is_set(RFID_PIN, RFID_PIN_NUM);
  uint8_t counter=0,last=0,next;
  do {
    next=GEN_PIN&GEN_PIN_MASK;
    if (next!=last) counter++;
    last=next;
  } while (bit_is_set(RFID_PIN, RFID_PIN_NUM) && counter<0xFF);
  if (counter==0xFF && bit_is_set(RFID_PIN, RFID_PIN_NUM)) return;
  uint8_t halfbit=counter,offset=counter>>1;
  //uint8_t halfbit=64,offset=32;
  const int len=32;
  uint8_t data[len];
  uint8_t nextBit,nextByte,i,j,lastBit;
  nextBit=readBit(offset,halfbit);
  if (nextBit==2) return;
  for (uint8_t i=7;i>0;i--)
    if (readBit(halfbit+offset,halfbit)!=1) return;
  for (i=0;i<len;i++){
    nextByte=0;
    for (j=8;j>0;j--){
      nextBit=readBit(halfbit+offset,halfbit);
      if (nextBit==2) break;
      nextByte<<=1;
      nextByte|=nextBit;
    }
    if (nextBit==2){
      lastBit=j;
      for (;j>0;j--) nextByte<<=1;
      data[i]=nextByte;
      break;
    }
    data[i]=nextByte;
  }
  if (i==len) i--;
  Serial.println(halfbit,DEC);
  for (uint8_t i1=0;i1<=i;i1++){
    nextByte=data[i1];
    for (j=8;j>0;j--){
      if (i1==i && j==lastBit) break;
      if ((nextByte&128)==0) Serial.print('0'); else Serial.print('1');
      nextByte<<=1;
    }
    Serial.print(' ');
  }
  Serial.println();
}

void loop(){
  readRawData();
}
