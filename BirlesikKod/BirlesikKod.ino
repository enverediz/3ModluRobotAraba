#include <SoftwareSerial.h>// bluetooth pinlerini arduinonun rx tx'ine bağlamayıp farklı pinlere bağladığımız için bu kütüphaneyi kullanacağız. 
                           //rx tx pinlerine bağlarsak kod yüklerken hep çıkarmak zorunda kalırdık.
#include <LiquidCrystal_I2C.h> // LCD ekran kütüphanesi
#include <Wire.h> // LCD ekran kütüphanesi
#include <AFMotor.h> //Adafruit Motor Shield Library. First you must download and install AFMotor library
#include <Servo.h>// servo motor kütüphanesi
#include <NewPing.h>

//BLUETOOTH PİNLERİ
#define Rx 3 // bluetooth'un Tx'pini Arduino'da Rx'e bağlanır.
#define Tx 4 // bluetooth'un Rx'pini Arduino'da Tx'e bağlanır.
SoftwareSerial bluetooth(Rx,Tx); // bluetooth'un tanımını yapıyoruz.

String voice;

const int motorA1  = 6;  // L298N'in IN3 Girişi
const int motorA2  = 10;  // L298N'in IN1 Girişi
const int motorB1  = 9; // L298N'in IN2 Girişi
const int motorB2  = 5;  // L298N'in IN4 Girişi

int durum; //Bluetooth cihazından gelecek sinyalin değişkeni
int hiz = 255;   // Standart Hız, 0-255 arası bir değer alabilir
// SERVO MOTOR PİNİ
Servo servo; //define servo name

//FAR VE NİŞAN ALMA PİNLERİ
int LED1 = A0; //define LED 1 pin // bu sol far olsun.
int LED2 = A1; //define LED 2 pin // bu da sağ far olsun.
//int buzzerPin = 8; //define buzzer pin // BUZZER VE NEWPING KÜTÜPHANELERİNDE PORT ÇAKIŞMASI OLDUĞUNDAN BUZZERI KULLANAMADIK.
int lazerPin = 8;

// MOD SEÇİM PİNLERİ
int kaydirmaliAnahtar1 = 2; // BU ANAHTAR ENGEL MODU VE BLUETOOTH MODU ARASINDA SEÇİM YAPMAK İÇİN. SOLDAYSA ENGEL MODU, SAĞDAYSA BLUETOOTH MODU.
int kaydirmaliAnahtar2 = 7; //BU MOD DA EĞER YUKARIDAKİ MOD BLUETOOTH İSE KUMANDA MODU MU YOKSA SES KONTROL MODU MU ONU BELİRLER. SOLDAYSA KUMANDA SAĞDAYSA SES MODU.
                            // HER İKİ ANAHTAR İÇİN DE SOL BACAK VCC'YE YANİ +'YA, SAĞ BACAK GND'YE YANİ -'YE BAĞLI OLACAK. TERSİ DE OLABİLİR SADECE KODU BUNA GÖRE YAZMALIYIZ.

//ses sensörü pinleri
#define USTrigger 6
#define USEcho 7
#define maksimumUzaklik 100

NewPing sonar(USTrigger, USEcho, maksimumUzaklik);//ultrasonik sensör tanımlama
//kullanılacak eleman tanımı
unsigned int uzaklik;
unsigned int on_uzaklik;
unsigned int sol_uzaklik;
unsigned int sag_uzaklik;
unsigned int zaman;

LiquidCrystal_I2C ekran(0x27,16,2); // ekranı tanımlıyoruz.

void setup()
{
  bluetooth.begin(9600);
  Serial.begin(9600); //seri bağlantı başlat
  servo.attach(10); //servo motorun bağlandığı pin
  servo.write(90);  //servo motor açılışta 90 derecede çalışsın
  pinMode(LED1, OUTPUT); //led1 pini A0
  pinMode(LED2, OUTPUT); //led2 pini A1
  //pinMode(buzzerPin, OUTPUT); //buzzer pini A2
  pinMode(lazerPin,OUTPUT); // lazer nokta pini, pin3

  //Blueooth Pinleri 
  pinMode(Rx,INPUT);
  pinMode(Tx, OUTPUT);

  // Motor pinleri
  pinMode(motorA1, OUTPUT);
  pinMode(motorA2, OUTPUT);
  pinMode(motorB1, OUTPUT);
  pinMode(motorB2, OUTPUT);

  pinMode(kaydirmaliAnahtar1, INPUT); // ANAHTARDAN GELEN DEĞERİ OKUYACAĞIMIZ İÇİN INPUT OLUYOR. EĞER MOTOR ÇALIŞTIRMA VEYA LED YAKMA OLSAYDI OUTPUT OLURDU.
  pinMode(kaydirmaliAnahtar2, INPUT);

  ekran.init(); // ekranı çalıştır.
  ekran.backlight(); // ekran arka ışığı aç.  

}

void loop()
{   
  if(kaydirmaliAnahtar1 == HIGH){       //BU KODUN TAMAMI ŞU DEMEK. EĞER 1. KAYDIRMALI ANAHTAR SOLDAYSA YANI + ISE ENGELDEN KAÇMA MODU ÇALIŞACAK. DEĞİLSE YANI - İSE BLUETOTTH MODLARINDAN BİRİ ÇALIŞACAK.
      engeldenKacmaModu();              // BURAYA DA BİR EĞER KOYDUK. EĞER 2. KAYDIRMALI ANAHTAR SOLDAYSA YANI + ISE KUMANDA MODU ÇALIŞACAK DEĞİLSE YANI - İSE SES KONTROL MODU ÇALIŞACAK.
  }                                     // BURADA BİZ SOL BACAĞA + BAĞLADIĞIMIZ İÇİN SOL + İSE YAZDIK, -'Yİ SOLA BAĞLASAYDIK - İSE YAZARDIK.    
  else{
    if(kaydirmaliAnahtar2 == HIGH){
      bluetoothKumandaModu();  
    }
    else {
      bluetoothSesleKontrolModu();  
    }
  }  
}

void engeldenKacmaModu() {
  ekranTemizleme(); // ilk önce ekran temizleme kodu çalıştırıyoruz çünkü modlar arası geçiş yapınca diğer modun ekran yazısı ekranda kalmış olacak.
  delay(10);
  girisYazisi();
  delay(3000);
  ekranTemizleme();
  delay(10);
  engelModuEkranYazisi();
  delay(2000);
  ekranTemizleme();
  delay(10);
  isimListesiYazisi();

  ///////////////////////////////////////////////////
  delay(500); // çalışır çalışmaz hareket etmesin önce yarım saniye beklesin diye bunu yazdık.
    
  sensor_olcum();
  on_uzaklik = uzaklik;
  if (on_uzaklik > 35 || on_uzaklik == 0)
  {
    ileriGit();
  }
  else
  {
    durdur();
    servo.write(180);
    delay(500);
    sensor_olcum();
    sol_uzaklik = uzaklik;
    servo.write(0);
    delay(500);
    sensor_olcum();
    sag_uzaklik = uzaklik;
    servo.write(90);
    delay(300);
    if (sag_uzaklik > sol_uzaklik)
    {
      sagYap();
      delay(200);
      ileriGit();
    }
    else if (sol_uzaklik > sag_uzaklik)
    {
      solYap();
      delay(200);
      ileriGit();
    }
    else
    {
      geriGit();
    }
  }
}

void bluetoothKumandaModu() {

  ekranTemizleme(); // ilk önce ekran temizleme kodu çalıştırıyoruz çünkü modlar arası geçiş yapınca diğer modun ekran yazısı ekranda kalmış olacak.
  delay(10);
  girisYazisi();
  delay(3000);
  ekranTemizleme();
  delay(10);
  kumandaModuEkranYazisi();
  delay(2000);
  ekranTemizleme();
  delay(10);
  isimListesiYazisi();

  //Gelen veriyi 'durum' değişkenine kaydet
  if (bluetooth.available() > 0) { //bluetooth'tan gelen veri var mı?
    durum = bluetooth.read();      // veri varsa bunu durum'a yaz.
    Serial.println(durum);         // durum'u de seri ekrana bağla.
  }

  /* Uygulamadan ayarlanabilen 4 hız seviyesi.(Değerler 0-255 arasında olmalı)*/
  if (durum == '0') {
    hiz = 0;
  }
  else if (durum == '1') {
    hiz = 100;
  }
  else if (durum == '2') {
    hiz = 180;
  }
  else if (durum == '3') {
    hiz = 200;
  }
  else if (durum == '4') {
    hiz = 255;
  }

  /***********************İleri****************************/
  //Gelen veri 'F' ise araba ileri gider.
  if (durum == 'F') {
    ileriGit();
  }

  /***********************Geri****************************/
  //Gelen veri 'B' ise araba geri gider.
  else if (durum == 'B') {
    geriGit();
  }
  /***************************Sol*****************************/
  //Gelen veri 'L' ise araba sola gider.
  else if (durum == 'L') {
    solYap();
  }
  /***************************Sağ*****************************/
  //Gelen veri 'R' ise araba sağa gider
  else if (durum == 'R') {
    sagYap();
  }

  /************************Stop*****************************/
  //Gelen veri 'S' ise arabayı durdur.
  else if (durum == 'S') {
    durdur();
  }

}

void bluetoothSesleKontrolModu() {

  ekranTemizleme(); // ilk önce ekran temizleme kodu çalıştırıyoruz çünkü modlar arası geçiş yapınca diğer modun ekran yazısı ekranda kalmış olacak.
  delay(10);
  girisYazisi();
  delay(3000);
  ekranTemizleme();
  delay(10);
  sesModuEkranYazisi();
  delay(2000);
  ekranTemizleme();
  delay(10);
  isimListesiYazisi();
  
  while (bluetooth.available()) { //seri bağlantı var mı yok mu
    delay(10); //10 milisaniye sürsün
    char c = bluetooth.read(); //seri ekranı oku ve c'ye yaz. Burada c bizim ağzımızdan çıkacak her bir harfi simgeler. c harfi de Char'ın ilk harfidir.
    if (c == '#') {
      break; //Exit the loop when the # is detected after the word
    }
    voice += c; //voice ise kelimeleri temsil eder. Her bir harfi yan yana yazınca kelime yani voice oluşur. voice = voice+c şeklinde de yazılabilir.
  }
  if (voice.length() > 0) {
    if (voice == "*ileri git") {
      ileriGit();
    }
    else if (voice == "*geri gel") {
      geriGit();
    }
    else if (voice == "*sol yap") {
      sagYap();
    }
    else if (voice == "*sağ yap") {
      solYap();
    }
    else if (voice == "*far aç") {
      ledAcik();
    }
    else if (voice == "*far kapat") {
      ledKapali();
    }
//    else if (voice == "*korna çal") {
//      uyariSesi();
//    }
    else if (voice == "*dur") {
      durdur();
    }
    else if (voice == "*nişan al"){
      digitalWrite(lazerPin, HIGH); // sonra lazeri kapatmak için ateşleme komutundan sonra lazeri kapatacak komutu yazmayı unutma.
    }
    else if(voice == "*bismillah ateş"){
      digitalWrite(lazerPin, LOW);
    }
    
    voice = ""; //her komut sonrası ses verisini sıfırlamak için bunu yapıyoruz, yoksa sürekli ilk verdiğimiz komutu yapar.
  }
}
void ileriGit() //
{
  analogWrite(motorA1, hiz);
  analogWrite(motorA2, 0);
  analogWrite(motorB1, hiz);
  analogWrite(motorB2, 0);
}

void geriGit() // geri komutları
{
  analogWrite(motorA1, 0);
  analogWrite(motorA2, hiz);
  analogWrite(motorB1, 0);
  analogWrite(motorB2, hiz);
}

void sagYap()
{
  sagSinyal();
  
  servo.write(180);
  delay(1000);
  servo.write(90);
  delay(1000);
  analogWrite(motorA1, 0);
  analogWrite(motorA2, 0);
  analogWrite(motorB1, hiz);
  analogWrite(motorB2, 150);
}

void solYap()
{
  solSinyal();
  
  servo.write(0);
  delay(1000);
  servo.write(90);
  delay(1000);
  analogWrite(motorA1, hiz);
  analogWrite(motorA2, 150);
  analogWrite(motorB1, 0);
  analogWrite(motorB2, 0);
}

void ledAcik ()
{
  digitalWrite(LED1, HIGH);
  digitalWrite(LED2, HIGH);
}

void ledKapali ()
{
  digitalWrite(LED1, LOW);
  digitalWrite(LED2, LOW);
}

//void uyariSesi ()
//{
//  tone(buzzerPin, 100);
//  delay(800);
//  noTone(buzzerPin);
//}

void durdur ()
{
  analogWrite(motorA1, 0);
  analogWrite(motorA2, 0);
  analogWrite(motorB1, 0);
  analogWrite(motorB2, 0);
}

// sensörün mesafe ölçümü
void sensor_olcum()
{
delay(50);
zaman = sonar.ping();
uzaklik = zaman / US_ROUNDTRIP_CM;
}

void girisYazisi(){  
  ekran.setCursor(5,0);
  ekran.print("#Milli");
  ekran.setCursor(0,1);
  ekran.print("TeknolojiHamlesi");  
}

void isimListesiYazisi(){
  
  String dizi[] = {"YAGIZ SELIM", "YAHYA", "SEMIH", "CIYAGER", "ZANA", "YEKTA", "YAGMUR", "JINDA","RONI","ROHAT","EFE","EREN_1","AZAT","HEBUN","AZIZ", "BERAN", "ROBIN", "RONIN", "YUSUF",
  "ARDA", "OMUR_1","OMUR_2", "OMUR_3", "ABDULLAH", "EVIN", "SILAN", "NARIN", "SEMA", "AYSE_1", "HIRA_1", "ADA", "MIRAN_1", "ISIL", "MIRAN_2", "MERTCAN", "IBRAHIM", "HUSEYIN", "POLAT", "LAVIN",
  "SUMEYYE", "AYSE_2", "HIRA_2", "EMIR", "SIYABEND", "MUHAMMET_1", "MUHAMMET_2", "YAREN", "YUSUF", "ADEM", "ASIM", "OMER_1", "EREN_2", "EYMEN_1", "HASAN", "ELA", "ASIYE", "RAHE", "ECE"};
  ekran.setCursor(1,0);
  ekran.print("Takim Uyeleri");
  for(int i = 0; i<8; i++){    
    ekran.setCursor(0,1);
    ekran.print(dizi[i]);
    delay(1000);
    ekran.setCursor(0,1);
    ekran.print("                "); // ekrandaki yazıyı temizlemek için.    
  }  
}

void ekranTemizleme(){ // ekrandaki yazıları temizleme kodu. Çünkü son yazılan yazı sürekli ekranda kalıyor.
  
    ekran.setCursor(0,0);
    ekran.print("                ");
    ekran.setCursor(0,1);
    ekran.print("                ");   
}

void engelModuEkranYazisi(){
    ekran.setCursor(1,0);
    ekran.print("ENGELDEN KACMA");
    ekran.setCursor(3,1); // YAZIYI ORTALAMAK İÇİN İLK 3 KUTUYU BOŞ BIRAKTIK. TOPLAMDA 16 KUTU VAR BU SATIRDA.
    ekran.print("MODU AKTIF");
}

void kumandaModuEkranYazisi(){
    ekran.setCursor(0,0);
    ekran.print("UZAKTAN KUMANDA");
    ekran.setCursor(3,1);
    ekran.print("MODU AKTIF");
}

void sesModuEkranYazisi(){
    ekran.setCursor(1,0);
    ekran.print("SESLE KONTROL");
    ekran.setCursor(3,1);
    ekran.print("MODU AKTIF");
}

void solSinyal(){
    digitalWrite(LED1, LOW);
    delay(10);
    digitalWrite(LED1, HIGH);
    delay(150);
    digitalWrite(LED1, LOW);
    delay(150);
    digitalWrite(LED1, HIGH);
    delay(150);
    digitalWrite(LED1, LOW);
    delay(150);
}

void sagSinyal(){
    digitalWrite(LED2, LOW);
    delay(10);
    digitalWrite(LED2, HIGH);
    delay(150);
    digitalWrite(LED2, LOW);
    delay(150);
    digitalWrite(LED2, HIGH);
    delay(150);
    digitalWrite(LED2, LOW);
    delay(150);
}
