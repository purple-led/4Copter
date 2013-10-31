#include <LiquidCrystal.h>

//LiquidCrystal(rs, rw, enable, d4, d5, d6, d7)

void setup() {
  //Выводим текст
  LiquidCrystal lcd = LiquidCrystal(7, 6, 13, 5, 4, 3, 2);
  lcd.begin(16, 2);
  lcd.print("Hello!");

  while(1)
  {
    lcd.setCursor(0, 1);
    //Выводим число секунд со старта
    lcd.print(millis()/1000);
  }
}

void loop() {
  //Выставляем курсор во 2-ю строку,
  //1й столбец (счет идет с 0, поэтому
  //строка номер 1, стобец номер 0)
}
