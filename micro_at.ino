#include <PinChangeInterrupt.h>
#include <PinChangeInterruptBoards.h>
#include <PinChangeInterruptPins.h>
#include <PinChangeInterruptSettings.h>
#include <LiquidCrystal.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <openGLCD.h>
#include <openGLCD_Buildinfo.h>
#include <Wire.h>
#include <openGLCD_Config.h>
#define set_bit(value, bit) ( _SFR_BYTE(value) |= _BV(bit) )
#define clear_bit(value, bit) ( _SFR_BYTE(value) &= ~_BV(bit) )
#define SLAVE 4  // 슬레이브 주소

// LCD-Rader
// byte bird[] = {
//    // 새 그림 데이터 (8x8)
//     B01110000,
//     B01011000,
//     B11110000,
//     B01111100,
//     B01111110,
//     B00111111,
//     B00011000,
//     B00010100
// };
// byte deer[] = {
//    // 노루 그림 데이터 (8x8)
//    B00100100,
//    B01100110,
//    B00100100,
//    B01111110,
//    B11011011,
//    B01011010,
//    B01111110,
//    B00111100
// };

uint16_t bird[16] = {
  0b0000000000000000,
  0b0000000000000000,
  0b0000000000000000,
  0b0000000000000000,
  0b0000111000000000,
  0b0001000100000000,
  0b0001000111110000,
  0b0111000100001000,
  0b0000111001111100,
  0b0000010010000100,
  0b0000010100000100,
  0b0000001100001000,
  0b0000000011110000,
  0b0000000010010000,
  0b0000000110110000,
  0b0000000000000000
};

uint16_t deer[16] = {
  0b0000000000000000,
  0b0000001000000000,
  0b0000110000000000,
  0b0011001000000000,
  0b0100101000000000,
  0b0111001000000000,
  0b0000101000000000,
  0b0000101000000100,
  0b0000100111111000,
  0b0000100000001000,
  0b0000100000001000,
  0b0000010000010000,
  0b0000011111110000,
  0b0000011000110000,
  0b0000011000110000,
  0b0000000000000000
};

// 7-segment
int digit_select_pin[] = { 66, 67, 68, 69 }; // 자릿수 선택 핀, 왼쪽부터 첫 번째, 두 번째
int segment_pin[] = { 58, 59, 60, 61, 62, 63, 64, 65 };   // 7세그먼트 모듈 연결 핀 ‘a, b, c, d, e, f, g, h’ 순서
byte segValue[6][8] = {
   {1,0,0,0,0,0,0,0}, // 위에 뚜껑
   {0,0,0,0,0,1,0,0}, // 왼쪽 위
   {0,0,0,0,1,0,0,0}, // 왼쪽 아래
   {0,0,0,1,0,0,0,0}, // 아래 바닥
   {0,0,1,0,0,0,0,0}, // 아래 C
   {0,1,0,0,0,0,0,0}, // 아래 B
};

// byte segValue[6][8] = {
//    {1,0,0,0,0,0,0,0}, // 위에 뚜껑
//    {0,0,0,0,0,1,0,0}, // 왼쪽 위
//    {0,0,0,0,1,0,0,0}, // 왼쪽 아래
//    {0,0,0,1,0,0,0,0}, // 아래 바닥
//    {0,0,1,0,0,0,0,0}, // 아래 C
//    {0,1,0,0,0,0,0,0}, // 아래 B
// };



// Mini LCD
// 핀 번호 (RS, E, DB4, DB5, DB6, DB7)
LiquidCrystal lcd(44, 45, 46, 47, 48, 49); // LCD 연결
char buffer_deer[255], buffer_bird[255], buffer_total[255];   // 숫자 저장 배열
int vResistor = A0;
int  deer_num;      // 사슴 숫자
int  bird_num;      // 새 숫자
int total_num;      // 총 숫자
float bird_degree[300];
float deer_degree[300];
bool check_bird = false;
bool check_deer = false;


// string parser
char buffer[300];
int buffer_cnt = 0;
int search_cnt = 0;
int col[2] = { 0,0 };
int animal_num = 0;
String animal_data[5][2];

//
volatile uint8_t state = 0;


ISR(PCINT1_vect) {
   state = !state;
}

// 해당 자리에 숫자 하나를 표시하는 함수
void show_digit(int pos, int number) { // (위치, 출력할 숫자)
   for (int i = 0; i < 4; i++) {
      if (i + 1 == pos) // 해당 자릿수의 선택 핀만 LOW로 설정
         digitalWrite(digit_select_pin[i], LOW);
      else // 나머지 자리는 HIGH로 설정
         digitalWrite(digit_select_pin[i], HIGH);
   }
   for (int i = 0; i < 8; i++) {
      boolean on_off = segValue[number][i];
      digitalWrite(segment_pin[i], on_off);
   }
}

// 왼쪽으로 7-segment 이동하는 코드
// 동기
// void shift_left() {
//   for (int i = 4; i >= 1; i--) {
//     show_digit(i, 0);
//     delay(500);
//     if(i==1){
//       show_digit(i, 1);
//       delay(500);
//       show_digit(i, 2);
//       delay(500);
//       break;
//     }
//   }
// }

// 비동기
void shift_left() {
   static unsigned long previousMillis = 0;
   static int i = 4;
   static int step = 0;
   const long interval = 500;  // 500ms 딜레이 넣음

   unsigned long currentMillis = millis();

   if (currentMillis - previousMillis >= interval) {
      previousMillis = currentMillis;

      if (step == 0) {
         for (; i >= 1; i--) {
            show_digit(i, 0);
            break;
         }
         if (i == 1) {
            step = 1;
         }
         else {
            i--;
         }
      }
      else if (step == 1) {
         show_digit(1, 1);
         step = 2;
      }
      else if (step == 2) {
         show_digit(1, 2);
         i = 4;
         step = 0;
      }
   }
}




// 오른쪽으로 7-segment 이동하는 코드
// 동기
// void shift_right(){
//   for(int i = 1; i <= 4; i++){
//     show_digit(i, 3);
//     delay(500);
//     if(i==4){
//       show_digit(i, 4);
//       delay(500);
//       show_digit(i, 5);
//       delay(500);

//     }
//   }
// }
void shift_right() {
   static unsigned long previousMillis = 0;
   static int i = 1;
   static int step = 0;
   const long interval = 500;  // 500ms 딜레이 넣음

   unsigned long currentMillis = millis();

   if (currentMillis - previousMillis >= interval) {
      previousMillis = currentMillis;

      if (step == 0) {
         for (; i <= 4; i++) {
            show_digit(i, 3);
            break;
         }
         if (i == 4) {
            step = 1;
         }
         else {
            i++;
         }
      }
      else if (step == 1) {
         show_digit(4, 4);
         step = 2;
      }
      else if (step == 2) {
         show_digit(4, 5);
         i = 1;
         step = 0;
      }
   }
}



void drawBird(int xStart, int yStart, uint8_t color) {
   int width = 16;
   int height = 16;
   for (int y = 0; y < height; y++) {
      for (int x = 0; x < width; x++) {
         if (bitRead(bird[y], x)) {
            GLCD.SetDot(xStart + x, yStart + y, color);
         }
      }
   }
}

void drawDeer(int xStart, int yStart, uint8_t color) {
   int width = 16;
   int height = 16;
   for (int y = 0; y < height; y++) {
      for (int x = 0; x < width; x++) {
         if (bitRead(deer[y], x)) {
            GLCD.SetDot(xStart + x, yStart + y, color);
         }
      }
   }
}

// 안쪽 작은 원
void drawCircle(int x0, int y0, int radius, uint8_t color) {
   for (int y = -radius; y <= radius; y++) {
      for (int x = -radius; x <= radius; x++) {
         int distanceSquared = x * x + y * y;
         if (distanceSquared <= radius * radius && distanceSquared > (radius - 1) * (radius - 1)) {
            GLCD.SetDot(x + x0, y + y0, color); // 원 테두리의 점 그리기
         }
      }
   }
}

// 바깥쪽 큰 원
void drawCircle_animal(int x0, int y0, int radius, uint8_t color) {
   for (int y = -radius; y <= radius; y++) {
      for (int x = -radius; x <= radius; x++) {
         int distanceSquared = x * x + y * y;
         if (distanceSquared <= radius * radius && distanceSquared > (radius - 1) * (radius - 1)) {
            GLCD.SetDot(x + x0, y + y0, color); // 원 테두리의 점 그리기
         }
      }
   }
}

// 새 그림 그리기
void drawBirdBetweenCircles(int x0, int y0, int innerRadius, int outerRadius, float degree, uint8_t color) {
   float radian = radians(degree); // 각도를 라디안으로 변환
   int xOffset = round((innerRadius + outerRadius) / 2.0 * sin(radian)); // 이동한 x 거리 계산
   int yOffset = round((innerRadius + outerRadius) / 2.0 * cos(radian)); // 이동한 y 거리 계산

   int xStart = x0 + xOffset - 4; // 시작점 x 좌표
   int yStart = y0 + yOffset - 4; // 시작점 y 좌표

   drawBird(xStart, yStart, color); // 그림 그리기
}

// 노루 그림 그리기
void drawDeerBetweenCircles(int x0, int y0, int innerRadius, int outerRadius, float degree, uint8_t color) {
   float radian = radians(degree); // 각도를 라디안으로 변환
   int xOffset = round((innerRadius + outerRadius) / 2.0 * sin(radian)); // 이동한 x 거리 계산
   int yOffset = round((innerRadius + outerRadius) / 2.0 * cos(radian)); // 이동한 y 거리 계산

   int xStart = x0 + xOffset - 4; // 시작점 x 좌표
   int yStart = y0 + yOffset - 4; // 시작점 y 좌표

   drawDeer(xStart, yStart, color); // 그림 그리기
}


void PCINT1_init(void) { //인터럽트 입력했을때
   PCICR |= (1 << PCIE1);      // 핀 변화 인터럽트 1 허용
   PCMSK1 |= (1 << PCINT9);    // PJ0 핀의 핀 변화 인터럽트 허용
   sei();              // 전역적으로 인터럽트 허용
}


void setup() {
   pinMode(19, INPUT_PULLUP);
   //attachInterrupt(4, my_reset, FALLING);

   pinMode(vResistor, INPUT);
   lcd.begin(0, 2);
   lcd.setCursor(0, 0);
   lcd.write(byte(0));

   // 7-segment
   for (int i = 0; i < 4; i++) { // 자릿수 선택 핀을 출력으로 설정
      pinMode(digit_select_pin[i], OUTPUT);
   }
   for (int i = 0; i < 8; i++) { // 세그먼트 제어 핀을 출력으로 설정
      pinMode(segment_pin[i], OUTPUT);
   }

   // LCD-Rader
   GLCD.Init(); // GLCD 초기화
   GLCD.ClearScreen(); // 화면 지우기

   Wire.begin(SLAVE);  // 통신
   Wire.onReceive(receiveFromMaster);
   Serial.begin(9600);
}

void loop() {
   PCINT1_init();
   clear_bit(DDRJ, 0);
   set_bit(DDRE, 5);

start:
   // 각 인자 입력 받기
   sprintf(buffer_deer, "%02d", deer_num);
   sprintf(buffer_bird, "%02d", bird_num);
   sprintf(buffer_total, "%02d", total_num);

   // 첫번째 줄에 출력
   lcd.setCursor(0, 0);
   lcd.print("Total Animal: ");
   lcd.print(buffer_total);

   // 두번째 줄에 출력
   lcd.setCursor(0, 1);
   lcd.print("Bird:");
   lcd.print(buffer_bird);

   lcd.setCursor(9, 1);
   lcd.print("Deer:");
   lcd.print(buffer_deer);


   if (!state) {
      set_bit(PORTE, 5);
   }

   else {
      clear_bit(PORTE, 5);
   }


   // 7-segment
   shift_right();
   shift_left();


  //  int bird_num = 4; // 새의 개수 설정
  //  float bird_degree[bird_num] = {-45, -90, -135, -150}; // 각각의 새에 대한 각도 값들
  //  float bird_degree[bird_num] = 
  //  bird_degree = -135.0;
  //  deer_degree = -90.0;
  //  int deer_num = 2; // 노루 마리수 설정
  //  float deer_degree[deer_num] = {-135, -110}; // 각각의 노루에 대한 각도 값들

   int x0 = 64;
   int y0 = 64;
   int innerRadius = 30;
   int outerRadius = 64;
   drawCircle(x0, y0, innerRadius, BLACK);
   drawCircle_animal(x0, y0, outerRadius, BLACK);

   for(int i = 0; i < bird_num; i++) { // bird_num 개의 새들 그리기
     Serial.print("루프 안에있는 bird_degree : ");
     Serial.print(bird_degree[i]);
     Serial.print('\n');
     drawBirdBetweenCircles(x0, y0, innerRadius, outerRadius, (-bird_degree[i]-180), BLACK);
   }

   for(int i = 0; i < deer_num; i++) { // deer_num 개의 노루들 그리기
     Serial.print("루프 안에있는 deer_degree : ");
     Serial.print(deer_degree[i]);
     Serial.print('\n');
     drawDeerBetweenCircles(x0, y0, innerRadius, outerRadius, (-deer_degree[i]-180), BLACK);
   }
   
  //  float deer_degree = 110.0;
  //  float bird_degree = 140.0;
  //  drawBirdBetweenCircles(x0, y0, innerRadius, outerRadius, bird_degree, BLACK);
  //  drawDeerBetweenCircles(x0, y0, innerRadius, outerRadius, deer_degree, BLACK);
    delay(1000);
    GLCD.ClearScreen(); // 화면 지우기
    //my_reset();

  //  GLCD.Init(); // GLCD 초기화
}

// 초기화 함수, 여기 부분 블루투스로 입력 받으면 될듯
void my_reset() {
   total_num = 0;
   deer_num = 0;
   bird_num = 0;

   for(int i=0; i < 100; i++){
     bird_degree[i]=0;
   }
    for(int i=0; i < 100; i++){
     deer_degree[i]=0;
   }

  //  bird_degree = 0.0;
  //  deer_degree = 0.0;
}

void receiveFromMaster(int nByteNum){
  for(int i=0; i<nByteNum; i++){
    char test = Wire.read();
    // Serial.print("초기 받아오는 값 : ");
    // Serial.print(test);
    // Serial.print("\n");

    buffer[buffer_cnt++] = test;

    if(test == ';'){
      buffer_cnt = 0;
      search_cnt = 0;
      my_reset();
      parsing();
    }
  }
}

void parsing(){
  for(int i=0; i<sizeof(buffer); i++){
    Serial.print(buffer[i]);
    while (buffer[search_cnt] < 48 || buffer[search_cnt] > 57) {  //숫자면 인덱스 멈추기
      search_cnt++;
    }
    total_num = buffer[search_cnt] - '0';
    Serial.print("total_num = ");
    Serial.print(search_cnt);

    if(buffer[i] != ',' && i>1){
      if(buffer[i] == 'D' && buffer[i+1] == 'e' && buffer[i+2] == 'e' && buffer[i+3] == 'r'){
        deer_num++;
        int keep = i+5;
        String degree = "";
        while(buffer[keep] != ','){
          degree += buffer[keep];
          keep++;
        }
        deer_degree[deer_num-1] = degree.toInt();
        Serial.print("deer_degree : ");
        Serial.print(deer_degree[deer_num-1]);
        Serial.print("\n");

      }
    }
    if(buffer[i] != ',' && i>1){
      if(buffer[i] == 'B' && buffer[i+1] == 'i' && buffer[i+2] == 'r' && buffer[i+3] == 'd'){
        bird_num++;
        Serial.print(bird_num);
        int keep = i+5;
        String degree = "";
        while(buffer[keep] != ','){
          degree += buffer[keep];
          keep++;
        }
        bird_degree[bird_num-1] = degree.toInt();
        Serial.print("bird_degree : ");      
        Serial.print(bird_degree[bird_num-1]);
        Serial.print("\n");
      }
    }
  }
}
