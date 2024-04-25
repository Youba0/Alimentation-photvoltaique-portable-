#include <Wire.h>
#include <LiquidCrystal_I2C.h>    // Include LiquidCrystal_I2C library

// Configuration de l'écran LCD

  
LiquidCrystal_I2C lcd(0x27, 20, 4);  // Remplacez l'adresse 0x27/0x3F par l'adresse réelle de votre module I2C LCD 
const int PWM_ICON = 0;
const int SOLAR_ICON = 1;
const int BAT_ICON = 2;

float voltage_solar;
float current_solar;


// Broche d'entrée pour la mesure de tension
const int voltagePin = A0;
const int PotoPin = A2;
// Déclaration des broches
const int PWM_PIN = 6;
const int LIN_PIN = 7;
const int Current_Sens = A1;

const float R1= 9750.0;
const float R2= 22880.0;
const float R_T= R1/(R1+R2);
const double R_Shunt = 0.06;
const double R_L = 56000.0;

const float R1_bat = 5000.0;
const float R2_bat = 20000.0;
const float R_T_bat = R1_bat/(R1_bat+R2_bat);

const float PWM_max = 0.95;
const float PWM_min= 0;
const float PWM_init= 0.95;
const float PWM_D = 0.0001;

float V_old = 0;
float P_old = 0;
float PWM_old = PWM_init;
float PWM;
float P = 0;

const float BAT_Max_Voltage = 4.2;
const float BAT_Min_Voltage = 2.75;
int BAT_Voltage_Pourcentage;
const int BatVoltagePin = A3;

float test;



byte batteryIcon[8] = {
  B01110,
  B10001,
  B10001,
  B10001,
  B11111,
  B11111,
  B11111,
  B11111
};

byte solar_icon[8] =
{
  0b11111,
  0b10101,
  0b11111,
  0b10101,
  0b11111,
  0b10101,
  0b11111,
  0b00000
};

byte PWM_icon[8] =
{
  0b11101,
  0b10101,
  0b10101,
  0b10101,
  0b10101,
  0b10101,
  0b10101,
  0b10111,
};



void setup() 
{
  TCCR0B = TCCR0B & B11111000 | B00000001; // Fréquence de découpage 62500.00 Hz
  
  Serial.begin(9600);
  
  icons_init();
  

  // Définir la broche d'entrée comme entrée analogique
  pinMode(voltagePin, INPUT);
  // Définir les broches comme sorties
  pinMode(PWM_PIN, OUTPUT);
  pinMode(LIN_PIN, OUTPUT);

  pinMode(Current_Sens,INPUT);
}

void loop() 
{
  BAT_Voltage_Pourcentage = BatPourcentage(BatVoltagePin);
  voltage_solar = VoltMeter(voltagePin);
  current_solar = AmperMeter(Current_Sens);
  Serial.println(current_solar);
  if (BAT_Voltage_Pourcentage < 80)
  {
    PWM = MPPT(voltage_solar,current_solar);
  }
  else
  {
    PWM = 0.25;
  }
  
  set_PWM();
  LCD_Data();
  mosfet_pwm(PWM);

}

float VoltMeter(int pin)
{
  float rawValue = analogRead(pin);
  float voltage_value = (rawValue / 1023.0) * 5.0;
  float voltage = voltage_value/R_T;
  return voltage; 
}

float BatPourcentage(int pin)
{
  float rawValue = analogRead(pin);
  float voltage_value = (rawValue / 1023.0) * 5.0;
  float voltage = voltage_value/R_T_bat;
  float bat_pourcentage = map(voltage*100,BAT_Min_Voltage*100,BAT_Max_Voltage*100,0,100);
  return bat_pourcentage;
}


float AmperMeter(int pin)
{
  float Voltage_Shunt_Value = analogRead(Current_Sens);
  // Convertir la valeur analogique en tension (assumant une référence de 5V)
  float voltage_Shunt = (Voltage_Shunt_Value / 1023.0) * 5.0;
  float Current = ((voltage_Shunt*5000) / (R_Shunt*R_L));
  return Current;
} 

float MPPT(float V, float I)
{

  P = V*I;
  float dV = V - V_old;
  float dP = P - P_old;
  
  

  if (dP != 0){
    if (dP < 0){
      if (dV < 0){
        PWM = PWM_old + PWM_D;
        //Serial.println("ICI1");
      }
      else{
        PWM = PWM_old - PWM_D;
        //Serial.println("ICI2");
      }
    }
    else{
      if (dV < 0){
        PWM = PWM_old - PWM_D;
        //Serial.println("IC3");
      }
      else{
        PWM = PWM_old + PWM_D;
        //Serial.println("ICI4");
      }
    }
  }

  else{
      PWM = PWM_old;
      //Serial.println("ICI5");
    }

  
  PWM_old = PWM;
  V_old = V;
  P_old = P;
  //Serial.println(V_old);
  return PWM;

}

void set_PWM(void)
{
 if (PWM > PWM_max)
 {
    PWM = PWM_max;
 }
 else if (PWM < PWM_min)
 {
    PWM = PWM_min;
 }
 else
 {
  PWM = PWM;
 }
}

void icons_init(void)
{
  lcd.begin(20, 4);
  lcd.backlight();
  lcd.createChar(SOLAR_ICON, solar_icon);
  lcd.createChar(BAT_ICON, batteryIcon);
  lcd.createChar(PWM_ICON, PWM_icon);
  
  lcd.setCursor(0, 0);
  lcd.print("SOL");
  lcd.setCursor(4, 0);
  lcd.write((byte)SOLAR_ICON);  

  lcd.setCursor(7, 0);
  lcd.print("BAT");
  lcd.setCursor(11,0);
  lcd.write((byte)BAT_ICON);

  lcd.setCursor(15, 0);
  lcd.print("PWM");
  lcd.setCursor(19,0);
  lcd.write((byte)PWM_ICON);
}

void LCD_Data(void)
{
  lcd.setCursor(0, 1);
  lcd.print(voltage_solar, 2);
  lcd.setCursor(5, 1);
  lcd.print("V"); 

  lcd.setCursor(0, 2);
  lcd.print(current_solar, 2);
  lcd.setCursor(5, 2);
  lcd.print("A"); 

  lcd.setCursor(0, 3);
  lcd.print(P, 2);
  lcd.setCursor(5, 3);
  lcd.print("W"); 

  lcd.setCursor(8, 1);
  lcd.print(BAT_Voltage_Pourcentage);
  lcd.setCursor(11, 1);
  lcd.print("%"); 

  if (BAT_Voltage_Pourcentage < 80)
  {
    lcd.setCursor(8, 2);
    lcd.print("CC"); 
    lcd.setCursor(8, 3);
    lcd.print("     ");
  }

  else
  {
    lcd.setCursor(8, 3);
    lcd.print("CV"); 
    lcd.setCursor(8, 2);
    lcd.print("    "); 
  }



  lcd.setCursor(15, 2);
  lcd.print(PWM*100, 1);
  lcd.setCursor(19, 2);
  lcd.print("%"); 

}

float mosfet_pwm(float pwm)
{
  float alpha= pwm * 255;
  analogWrite(PWM_PIN,alpha);
  digitalWrite(LIN_PIN,HIGH);

}
