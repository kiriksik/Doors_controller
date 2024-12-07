import processing.serial.*;
Serial myPort;
int sec_towait = -1;
int port = 2; // Настройка номера порта среди доступных COM
char letter;
String words = "";
String receivedData = "";
String receivedCode = "";
int shift = 15;  
boolean alarm = false;
int alarm_counter = 0;


String encode(String message, int shift) {
  StringBuilder encoded = new StringBuilder();
  for (int i = 0; i < message.length(); i++) {
    char c = message.charAt(i);
    c = (char) (c + shift);
    encoded.append(c);
  }
  shift = (shift - 9) % 20 + 10;
  return encoded.toString();
}


String decodeMessage(String message, int shift) {
  StringBuilder encoded = new StringBuilder();
  for (int i = 0; i < message.length()-1; i++) {
    char c = message.charAt(i);
    println((c - shift) % 128);
    if (c - shift < 32)
      c = (char) (128 + (c - 32 - shift));
    else
      c = (char) (c - shift);
    encoded.append(c);
  }
  return encoded.toString();
}


void setup() {
  size(800, 500);
  println("Available ports:");
  println(Serial.list());
  myPort = new Serial(this, Serial.list()[port], 9600);
  myPort.clear();
  myPort.bufferUntil('\0');
  println(myPort);
}


void draw() {
  background(0);
  if (sec_towait != -1)
    if (((second() - sec_towait + 60) % 60) > 15) {
      sec_towait = -1;
      alarm = true;
      receivedCode = "Ответ не пришёл. Тревога";
    }
  int s = second(); 
  int m = minute(); 
  int h = hour(); 
  textSize(30);
  text(h+":"+m+":"+s, 20, 200);
  text(words, 20, 120, 540, 300);
  text(" " + receivedCode, 20, 50);
}


void serialEvent(Serial p) {
  if (alarm) {
    println("Alarm");
  }
  sec_towait = -1;
  receivedData = p.readString();
  if (receivedData != null) {
    println("Received: " + receivedData);
    receivedData = decodeMessage(receivedData, shift);
    println("Decoded: " + receivedData);
    int first_letter = int(receivedData.charAt(receivedData.length()-2)) - 48;
    int second_letter = int(receivedData.charAt(receivedData.length()-1)) - 48;
    receivedData = receivedData.substring(0, receivedData.length()-2);
    //println(first_letter + " " + second_letter + " "+ shift);

    if (shift == (first_letter * 10 + second_letter)) {
      shift = (shift - 9) % 20 + 10;
      println(receivedData);
      if (!alarm) {
          if (receivedData.equals("aprd") == true) {
            receivedCode = "Состояние двери изменено";
            println("Aproved");
            return;
          }
          if (receivedData.equals("deny") == true) {
            receivedCode = "Код не принят. Тревога";
            alarm_counter++;
            println("Not aproved");
            if (alarm_counter > 0)
              alarm = true;
              
            return;
          }
      }
      else if (receivedData.equals("aprd") == true) {
        receivedCode = "Тревога отключена";
        println("Alarm Deactiveted");
        alarm = false;
        return;
      }
    }
    else
      alarm = true;
    
  }
  else
    println("Received:" + null);
}


void keyTyped() {
  if (alarm) {
    println("Alarm");
    return;
  }
  if (key == 127 || key == 8) {
      if (words.length() > 0) {
        words = words.substring(0, words.length()-1);
      }
  }
  if (words.length() < 4) { 
    if ((key >= '0' && key <= '9')) {
      letter = key;
      words = words + key;
      println(key);
    }
  }
  else
  {
    if (key == '\n') {
        sec_towait = second();
      myPort.write(encode((words + shift), shift) + '\0');
      words = "";
    }
  }
}
