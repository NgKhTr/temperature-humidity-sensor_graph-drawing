/*
vấn đề:
  temPos cũng là NaN_i
  --> Hiển thị các giá trị xung quanh bị NaN_i
  sẽ mất 1 lúc để đo đc 
*/

#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>

#include <stdio.h>
#include <DS1302.h>

#include <DHT.h>

#include <IRremote.h>

#define NaN_i 255

bool isNaN_i(byte num) {
  return num == NaN_i;
}

LiquidCrystal_I2C lcd(0x27, 16, 2);

DS1302 rtc(2, 3, 4);

DHT dht(8, DHT11);

IRrecv irRecv(5);
decode_results irRes;

const byte amount = 60;
byte temperature_hrList[amount],
  humidity_hrList[amount],
  temperature_minList[amount + 1],
  humidity_minList[amount + 1],
  n = 0, preMin;


byte mode = 1,//mặc định hiển thị dữ liệu hiện tại
  denta = 0,//khoảng cách đến vị trí của dữ liệu được hiển thị trên màn hình so với current
  current = 0;//vị trí chứa dữ liệu hiện tại

bool isChangeData = true, isChangeDenta = true, isChangeMode = true;

char line1[17];

String printDayOfWeek(const Time::Day day)
{
  switch (day)
  {
    case Time::kSunday: return "SUN";
    case Time::kMonday: return "MON";
    case Time::kTuesday: return "TUE";
    case Time::kWednesday: return "WED";
    case Time::kThursday: return "THU";
    case Time::kFriday: return "FRI";
    case Time::kSaturday: return "SAT";
  }
  return "ERR";
}
byte getIndexDayOfWeek(const Time::Day day)
{
  switch (day)
  {
    case Time::kSunday: return 0;
    case Time::kMonday: return 1;
    case Time::kTuesday: return 2;
    case Time::kWednesday: return 3;
    case Time::kThursday: return 4;
    case Time::kFriday: return 5;
    case Time::kSaturday: return 6;
  }
  return NaN_i;
}
Time::Day daysOfWeek[7] = { Time::kSunday, Time::kMonday, Time::kTuesday, Time::kWednesday, Time::kThursday, Time::kFriday, Time::kSaturday};

byte position() {
  return ((denta > current)? amount - (denta - current): current - denta);
}
byte increase(byte i) {
  return ((i == amount - 1)? 0: i + 1);
}
byte decrease(byte i) {
  return ((i)? i - 1: amount - 1);
}
byte do_c[8] = {
  B10000,
  B00110,
  B01001,
  B01000,
  B01000,
  B01001,
  B00110
};

byte charsDisplay[8][8] = {
  {0, 0, 0, 0, 0, 0, 0, B11111},
  {0, 0, 0, 0, 0, 0, B11111, 0},
  {0, 0, 0, 0, 0, B11111, 0, 0},
  {0, 0, 0, 0, B11111, 0, 0, 0},
  {0, 0, 0, B11111, 0, 0, 0, 0},
  {0, 0, B11111, 0, 0, 0, 0, 0},
  {0, B11111, 0, 0, 0, 0, 0, 0},
  {B11111, 0, 0, 0, 0, 0, 0, 0},
};

void printCellTemOfChart(byte tem, byte temPos, byte curX) {
  if (temPos == NaN_i || tem == NaN_i) {
    lcd.setCursor(curX, 0);
    lcd.write(0);
    lcd.setCursor(curX, 1);
    lcd.write(7);
  }
  else if (tem > temPos && tem - temPos < 9) {
    lcd.setCursor(curX, 0);
    lcd.write(tem - temPos - 1);
  }
  else if (tem < temPos && temPos - tem < 9) {
    lcd.setCursor(curX, 1);
    lcd.write(8 + tem - temPos);
  }
}
void printCellHumOfChart(byte tem, byte temPos, byte curX) {
  printCellTemOfChart(tem, temPos, curX); //co the co thay doi
}
void setup()
{
  lcd.init();
  
  dht.begin();

  irRecv.enableIRIn();

  for (byte i = 0; i < amount; i++) {
    temperature_hrList[i] = NaN_i;
    humidity_hrList[i] = NaN_i;
  }

  Serial.begin(9600);
  //hiển thị các giá trị đầu tiên
  lcd.createChar( 0, do_c);
  lcd.backlight();
  Time t = rtc.time();
  String day = printDayOfWeek(t.day);
  snprintf(line1, sizeof(line1), "%02d:%02d  %03s,%02d/%02d",
    t.hr, t.min, day.c_str(), t.date, t.mon);
  Serial.println(line1);

  lcd.setCursor(0,0);
  lcd.print(line1);

  //line 2
  float hum = dht.readHumidity(),
    tem = dht.readTemperature();
  Serial.print("Hum: ");
  Serial.print(hum);
  Serial.print(" Tem: ");
  Serial.println(tem);
  lcd.setCursor(0,1);
  if (isnan(tem) || isnan(hum) || tem < 0 || tem >= 100 || hum < 0 || hum >= 100) {
    lcd.print("TEM:ERR HUM:ERR");
  }
  else {
    humidity_hrList[current] = humidity_minList[n] = round(hum);
    temperature_hrList[current] = temperature_minList[n] = round(tem);
    n++;

    lcd.print("TEM:");
    lcd.print(temperature_hrList[current]);
    lcd.write(0);
    lcd.setCursor(8,1);
    lcd.print("HUM:"); 
    lcd.print(humidity_hrList[current]);
    lcd.print("%");
  }
  preMin = rtc.time().min;
}
void loop()
{
  float
    hum = dht.readHumidity(),
    tem = dht.readTemperature();
  byte min = rtc.time().min;
  int sum;
  Serial.print("Hum: ");
  Serial.print(hum);
  Serial.print(" Tem: ");
  Serial.println(tem);

  isChangeData = true;
  if (min < preMin) {
    //đã qua 1 giờ
    if (n) {
      sum = humidity_minList[0];
      for (byte i = 1; i < n; i++) {
        sum += int(humidity_minList[i]);
      }
      humidity_hrList[current] = round(double(sum) / n);

      sum = temperature_minList[0];
      for (byte i = 1; i < n; i++) {
        sum += int(temperature_minList[i]);
      }
      temperature_hrList[current] = round(double(sum) / n);
    }
    else {
      humidity_hrList[current] = temperature_hrList[current] = NaN_i;
    }

    current++;
    n = 0;
    preMin = min;

    //cũng phải đo nữa chứ !!!!
    if ( isnan(hum) || isnan(tem) || tem < 0 || tem >= 100 || hum < 0 || hum >= 100) {
      humidity_hrList[current] = temperature_hrList[current] = NaN_i;
    }
    else {
      humidity_hrList[current] = humidity_minList[n] = round(hum);
      temperature_hrList[current] = temperature_minList[n] = round(tem);
      n++;
    }
  }
  else if (min - preMin >= 1) {
    if ( ! isnan(hum) && ! isnan(tem) && tem >= 0 && tem < 100 && hum >=0 && hum < 100) {
      humidity_hrList[current] = humidity_minList[n] = round(hum);
      temperature_hrList[current] = temperature_minList[n] = round(tem);
      n++;
    }

    preMin = min;
  }
  else {
    isChangeData = false;
  }

  isChangeMode = false;
  isChangeDenta = false;
  if (irRecv.decode( &irRes)) {
    if (irRes.value == 16738455 && mode != 0) {
      isChangeMode = true;
      mode = 0;
      Serial.println("Mode 0");
    }
    else if (irRes.value == 16724175 && mode != 1) {
      isChangeMode = true;
      mode = 1;
      Serial.println("Mode 1");
    }
    else if (irRes.value == 16718055 && mode != 2) {
      isChangeMode = true;
      mode = 2;
      Serial.println("Mode 2");
    }
    else if (irRes.value == 16743045 && mode != 3) {
      isChangeMode = true;
      mode = 3;
      Serial.println("Mode 3");
    }
    else if (irRes.value == 16712445) {
      isChangeDenta = true;
      denta = increase(denta);
      Serial.print("Denta: ");
      Serial.println(denta);
    }
    else if (irRes.value == 16720605) {
      isChangeDenta = true;
      denta = decrease(denta);
      Serial.print("Denta: ");
      Serial.println(denta);
    }
    else if (irRes.value == 16736925) {
      isChangeDenta = true;
      denta = 0;
      Serial.print("Denta: 0");
    }
    delay(20);
    irRecv.resume();
  }
  /*
  Data: tim/hum/thời gian
  mode != 0 -> thay đổi
  isChangeMode == true

  isChangeMode == false
    isChangeData == true
      isChangeMode == true
        -> change
      isChangeMode == false và 
        mode 0
        mode 1 và denta != 0
        mode 2, 3 và denta >= 7
        -> Ko thay đổi
    isChangeData == false
  mode == 0 -> ko thay đổi
  */
  if (isChangeData || isChangeDenta || isChangeMode) {
    Serial.print(isChangeData);
    Serial.print(isChangeDenta);
    Serial.print(isChangeMode);
    Serial.println();

    if (mode) {
      lcd.backlight();
      lcd.display();
      lcd.clear();
      if (mode == 1) {
        lcd.createChar( 0, do_c);

        Time t = rtc.time();
        //tru time
        if (denta) {
          t.min = 30;
          byte dentaGio = denta,
            dentaNgay = dentaGio / 24; //denta ngay
          
          dentaGio %= 24; //gio can tru
          if (dentaGio > t.hr) {
            dentaNgay++;
            t.hr = 24 - (dentaGio - t.hr);
          }
          else {
            t.hr -= dentaGio;
          }

          if (t.date > dentaNgay) {
            t.date -= dentaNgay;
          }
          else {
            t.mon = ((t.mon == 1) ? 12: t.mon - 1);
            //ko toi nuoc phai tru 2 mon dau :)))
            switch (t.mon)
            {
              case 1:
              case 3:
              case 5:
              case 7:
              case 8:
              case 10:
              case 12:
                t.date = 31 - (dentaNgay - t.date);
                break;
              case 4:
              case 6:
              case 11:
                t.date = 30 - (dentaNgay - t.date);
                break;
              default:
                if ((t.yr % 4 == 0 && t.yr % 100 != 0) || t.yr % 400 == 0) {
                  t.date = 29 - (dentaNgay - t.date);
                }
                else {
                  t.date = 28 - (dentaNgay - t.date);
                }
                break;
            }

          }
          byte day = getIndexDayOfWeek(t.day);
          day = (dentaNgay > day) ? 7 - (dentaNgay - day): day - dentaNgay;
          t.day = daysOfWeek[day];
        }
        snprintf(line1, sizeof(line1), "%02d:%02d  %03s,%02d/%02d",
          t.hr, t.min, printDayOfWeek(t.day).c_str(), t.date, t.mon);

        lcd.setCursor(0,0);
        lcd.print(line1);

        //line 2
        byte _position = position();
        lcd.setCursor(0,1);
        if (temperature_hrList[_position] == NaN_i) {
          lcd.print("TEM:ERR HUM:ERR");
        }
        else {
          lcd.print("TEM:");
          lcd.print(temperature_hrList[_position]);
          lcd.write(0);
          lcd.setCursor(8,1);
          lcd.print("HUM:"); 
          lcd.print(humidity_hrList[_position]);
          lcd.print("%");
        }
      }
      else if (mode == 2) {
        //hiện biểu đồ nhiệt độ
        for (byte i = 0; i < 8; i++) {
          lcd.createChar( i, charsDisplay[i]);
        }
        byte _position = position();

        byte x;
        if (denta < 7) {
          x = 0;
          for (byte i = current; i != _position; i = decrease(i), x++) {
            printCellTemOfChart( temperature_hrList[i], temperature_hrList[_position], x);
          }

          lcd.setCursor(x, 0);
          lcd.write(7);
          lcd.setCursor(x, 1);
          lcd.write(0);
          x = x + 1;
          lcd.setCursor(x, 0);
          lcd.write(7);
          lcd.setCursor(x, 1);
          lcd.write(0);
          x = x + 1;

          for (byte i = decrease(_position); x < 16; i = decrease(i),  x++) {
            printCellTemOfChart( temperature_hrList[i], temperature_hrList[_position], x);
          }
        }
        else if (amount - denta <= 7) {
          x = 15;
          for (byte i = increase(current); i != _position; i = increase(i), x--) {
            printCellTemOfChart( temperature_hrList[i], temperature_hrList[_position], x);
          }

          lcd.setCursor(x, 0);
          lcd.write(7);
          lcd.setCursor(x, 1);
          lcd.write(0);
          x = x - 1;
          lcd.setCursor(x, 0);
          lcd.write(7);
          lcd.setCursor(x, 1);
          lcd.write(0);
          x = x - 1;

          for (byte i = increase(_position); x != 255; i = increase(i), x--) {
            printCellTemOfChart( temperature_hrList[i], temperature_hrList[_position], x);
          }
        }
        else {
          lcd.setCursor(7, 0);
          lcd.write(7);
          lcd.setCursor(7, 1);
          lcd.write(0);
          lcd.setCursor(8, 0);
          lcd.write(7);
          lcd.setCursor(8, 1);
          lcd.write(0);

          x = 6;
          for (byte i = increase(_position); x != 255; i = increase(i), x--) {
            printCellTemOfChart( temperature_hrList[i], temperature_hrList[_position], x);
          }

          x = 9;
          for (byte i = decrease(_position); x < 16; i = decrease(i), x++) {
            printCellTemOfChart( temperature_hrList[i], temperature_hrList[_position], x);
          }
        }
      }
      else {
        //hiện biểu đồ độ ẩm
        for (byte i = 0; i < 8; i++) {
          lcd.createChar( i, charsDisplay[i]);
        }
        byte _position = position();

        byte x;
        if (denta < 7) {
          x = 0;
          for (byte i = current; i != _position; i = decrease(i), x++) {
            printCellHumOfChart( humidity_hrList[i], humidity_hrList[_position], x);
          }

          lcd.setCursor(x, 0);
          lcd.write(7);
          lcd.setCursor(x, 1);
          lcd.write(0);
          x = x + 1;
          lcd.setCursor(x, 0);
          lcd.write(7);
          lcd.setCursor(x, 1);
          lcd.write(0);
          x = x + 1;

          for (byte i = decrease(_position); x < 16; i = decrease(i),  x++) {
            printCellHumOfChart( humidity_hrList[i], humidity_hrList[_position], x);
          }
        }
        else if (amount - denta <= 7) {
          x = 15;
          for (byte i = increase(current); i != _position; i = increase(i), x--) {
            printCellHumOfChart( humidity_hrList[i], humidity_hrList[_position], x);
          }

          lcd.setCursor(x, 0);
          lcd.write(7);
          lcd.setCursor(x, 1);
          lcd.write(0);
          x = x - 1;
          lcd.setCursor(x, 0);
          lcd.write(7);
          lcd.setCursor(x, 1);
          lcd.write(0);
          x = x - 1;

          for (byte i = increase(_position); x != 255; i = increase(i), x--) {
            printCellHumOfChart( humidity_hrList[i], humidity_hrList[_position], x);
          }
        }
        else {
          lcd.setCursor(7, 0);
          lcd.write(7);
          lcd.setCursor(7, 1);
          lcd.write(0);
          lcd.setCursor(8, 0);
          lcd.write(7);
          lcd.setCursor(8, 1);
          lcd.write(0);

          x = 6;
          for (byte i = increase(_position); x != 255; i = increase(i), x--) {
            printCellHumOfChart( humidity_hrList[i], humidity_hrList[_position], x);
          }

          x = 9;
          for (byte i = decrease(_position); x < 16; i = decrease(i), x++) {
            printCellHumOfChart( humidity_hrList[i], humidity_hrList[_position], x);
          }
        }
      }
    }
    else {
      lcd.noDisplay();
      lcd.noBacklight();
    }
  }
  Serial.println("1 loop");
  delay(500);
}