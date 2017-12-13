// DHT - Version: Latest 
#include <DHT.h>
// LCD5110_Basic - Version: Latest 
#include <LCD5110_Basic.h>




//-----------------------------Pinagem do display LCD-----------------------------
#define CLK 2
#define DIN 3
#define DC  4
#define CE  5
#define RST 6

//-----------------------------Pinagem motor vertical-----------------------------
#define IN3 7
#define IN4 8
#define velocidadeB 9

//-----------------------------Pinagem Motor Horizontal---------------------------
#define IN1  10 
#define IN2  11
#define velocidadeA 12

//-----------------------------Pinagem Sensor de Poeira DSM501A--------------------
#define pinPoeira 13     // Pino digital 
#define AMOSTRAGEM 3000

//-----------------------------Pinagem do infravermelho---------------------------
#define infra 14        // Pino digital (A0) 

//-----------------------------Pinagem sensor MQ-2 (sensor de gases inflamaveis)---
#define pinGasA A1      // Pino sensor de gas no analogico A1

//-----------------------------Constantes para o sensor DHT------------------------
#define DHTPIN 16       // Pino digital (A2) 
#define DHTTYPE DHT22   // Tipo de sensor DHT 22 (AM2302)

//-----------------------------Pinagem para o sensor de CO-------------------------
#define AOUTpin A3      // Pino analgico 

//-----------------------------Ordem dos recipientes------------------------------
#define TINTA 0
#define PO    1
#define AGUA  2
#define PAPEL 3

// -------------------------------Variaveis para o sensor MQ-7 (sensor de CO)-------------------------------------
int limit;             // Variavel booleana que retorna 1 se atingiu o limite e 0 caso contrario
int value;             // Valor armazenado do sensor CO

//--------------------------------Variaveis para o sensor DHT-----------------------------------------------------
DHT dht(DHTPIN, DHTTYPE);
int t;                // Temperatura
int h;                // Humidade

//--------------------------------Variaveis para o sensor de poeira-----------------------------------------------
unsigned long duration;
unsigned long starttime;
unsigned long endtime;
unsigned long sampletime_ms = AMOSTRAGEM;
unsigned long lowpulseoccupancy = 0;
int ratio = 0;                            // Porcentagem de tempo que registrou particula 
int concentration = 0;                    // Valor nominal da quantidade de particulas detectadas


//---------------------------------Configurando Display LCD-------------------------------------------------------
LCD5110 tela(CLK,DIN,DC,RST,CE);
// Obtendo as fontes
extern uint8_t SmallFont[];
extern uint8_t MediumNumbers[];
extern uint8_t BigNumbers[];
// Strings mostradas no LCD
String coValue ;
String tempValue;
String humValue;
String dustValue;
String gasValue;
String stringCor;


//---------------------------------Variaveis para a pintura-------------------------------------------------------
int luz;                    //Se detectou a faixa preta ou nao
volatile int count = 0;     //Conta quantas vezes parou para movimentar o pincel
int humidade;               //Porcentagem da humidade
long int cor;                    //Qual tonalidade do cyanometro
int naAgua;                 //Vezes que o pincel passa na agua
int noPo;                   //Vezes que o pincel passa no po da tinta
int noPapel;                //Vezes que o pincel passa no papel
bool pintou = false;        //Se já pintou ou não
int umidadeTemp;

//                           ------------------------------------------------------------------------------
//                          |            Diluir                  |          Escurecer                      |
//                          |------------------------------------------------------------------------------|
// A  | T  | P     D  | E   |    Agua      |      Po             |      Tinta    |     Papel               |
//13  | 1  | 1  =  0  | 27  | 13-(x/2)     | (x/2)+1      (PAR)  |  ((x-27)/2)+1 | ((x-27)/2)+1    (IMPAR) | 
//13  | 1  | 2  =  1  | 28  | 13-((x-1)/2) | ((x+1)/2)+1  (IMPAR)|  ((x-27+1)/2) | ((x-27+1)/2)+1  (PAR)   |
//12  | 2  | 2  =  2  | 29   ------------------------------------------------------------------------------
//12  | 2  | 3  =  3  | 30
//11  | 3  | 3  =  4  | 31  
//11  | 3  | 4  =  5  | 32
//10  | 4  | 4  =  6  | 33
//10  | 4  | 5  =  7  | 34
// 9  | 5  | 5  =  8  | 35
// 9  | 5  | 6  =  9  | 36
// 8  | 6  | 6  =  10 | 37
// 8  | 6  | 7  =  11 | 38
// 7  | 7  | 7  =  12 | 39
// 7  | 7  | 8  =  13 | 40
// 6  | 8  | 8  =  14 | 41
// 6  | 8  | 9  =  15 | 42
// 5  | 9  | 9  =  16 | 43
// 5  | 9  | 10 =  17 | 44
// 4  | 10 | 10 =  18 | 45
// 4  | 10 | 11 =  19 | 46
// 3  | 11 | 11 =  20 | 47
// 3  | 11 | 12 =  21 | 48
// 2  | 12 | 12 =  22 | 49
// 2  | 12 | 13 =  23 | 50
// 1  | 13 | 13 =  24 | 51
// 1  | 13 | 14 =  25 | 52
//
// A - passadas na agua
// T - passadas na tinta em po
// P - passadas no papel
// D - cor da tinta diluida
// E - cor da tinta engrossada



void setup(){
  Serial.begin(9600);
  //Inicializa Pinos
  //Motor horizontal
  pinMode(IN1,OUTPUT);
  pinMode(IN2,OUTPUT);
  pinMode(velocidadeA,OUTPUT);
  
  //Motor vertical
  pinMode(IN3,OUTPUT);
  pinMode(IN4,OUTPUT);  
  pinMode(velocidadeB,OUTPUT);
  
  pinMode(infra,INPUT_PULLUP);        // Configura Sensor de luz
  
  pinMode(AOUTpin, INPUT);            // Configura pino do sensor CO
  pinMode(pinPoeira,INPUT);           // Configura pino Digital de sensor do poeira
  pinMode(pinGasA, INPUT);            // Configura pino Analogico do sensor de gas 
  dht.begin();                        // Inicializa o sensor DHT
  starttime = millis();               // Inicia o timer para o sensor de poeira
  tela.InitLCD();                     // Inicializando o display

}

void loop(){ 
  
  //TODO Determinar cor stringCor
  
  coValue = medeCO();  
  tempValue = medeTemperatura();
  humValue = medeHumidade();
  dustValue = medePoeira();
  gasValue = medeGas();  
  printaTela(coValue,tempValue,humValue,dustValue,gasValue,stringCor);

  umidadeTemp = humValue.toInt();
  cor = map(umidadeTemp,0,100,0,52);
  //cor = 25;
  
  //Zerando variaveis auxiliares 
  naAgua = 0;
  noPapel = 0;
  noPo = 0;
  
  if(pintou == false){                //Verifica se já fez a pintura 
    if(cor == 26){                    //Tonalidade media da tinta
      controlaMotores();
      pintou = true;
    }
    else if(cor < 26 && cor >= 0){    //Mais claro que a tinta
      controlaMotoresDiluir(cor);
      pintou = true;
    }
    else if(cor > 26 && cor <= 52){   //Mais escuro que a tinta
      controlaMotoresEscurecer(cor);
      pintou = true;
    }
    else{
      Serial.println("Deu erro na leitura das cores!");
      delay(500);
    }
  }


}


//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------Display LCD------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void printaTela(String coValue, String tempValue, String humValue,String dustValue,String gasValue,String stringCor)
{  
  tela.setFont(SmallFont); //Definindo a fonte
  tela.print("Cor: ", LEFT, 0); 
  tela.print(stringCor, RIGHT, 0);
  tela.print("Umidade: ", LEFT, 10); 
  tela.print(humValue+"%", RIGHT, 10); 
  tela.print("Temp: ", LEFT, 20); 
  tela.print(tempValue+"*C", RIGHT, 20); 
  tela.print("Poeira:",LEFT,30);
  tela.print(dustValue+"ptc.", RIGHT, 30); 
  tela.print("Gas: ", LEFT, 35); 
  tela.print(gasValue+"ppm", RIGHT,35); 
  tela.print("CO: ", LEFT, 40); 
  tela.print(coValue+"ppm", RIGHT, 40); 
  delay(100);
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------Sensor CO--------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
String medeCO()
{
  value= analogRead(AOUTpin);    //reads the analaog value from the CO sensor's AOUT pin
  Serial.print("Valor de CO: ");
  Serial.println(value);         //prints do valor de CO
  return String(map(value,0,1024,10,10000)); //Retorna o valor convertido para ppm
}


//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------Sensor DHT-------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
String medeTemperatura()
{
  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)  
  // Read temperature as Celsius
  t = (int) dht.readTemperature();  // Verifica Temperatura  
  // Check if any reads failed and exit early (to try again).
  Serial.print("Temperatura: "); 
  Serial.print(t);
  Serial.print(" *C temp:  ");
  return String(t);
}

String medeHumidade(){
  h = dht.readHumidity();          // Verifica humidade
  Serial.print("Humidade: "); 
  Serial.println(h);
  return String(h);
}


//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------Sensor de Poeira-------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//Quantitative particle (> 1 micron) measurement 
String medePoeira(){
  duration = pulseIn(pinPoeira, LOW);                
  lowpulseoccupancy += duration;
  endtime = millis();  
  if ((endtime-starttime) > sampletime_ms)
  {
    ratio = (lowpulseoccupancy-endtime+starttime + sampletime_ms)/(sampletime_ms*10.0);  // Integer percentage 0=>100
    concentration = (int) 1.1*pow(ratio,3)-3.8*pow(ratio,2)+520*ratio+0.62;              // using spec sheet curve
    Serial.print("Proporcao de poeira:");
    Serial.print(ratio);
    Serial.print("%    Concentracao de poeira:");
    Serial.println(concentration);
    lowpulseoccupancy = 0;
    starttime = millis();
  }
  return String(concentration);
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------Sensor de Gases inflamveis---------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
String medeGas(){
  // Le os dados do pino analogico do sensor
  int valor_analogico = analogRead(pinGasA);
  return String(map(valor_analogico,0,1024,300,10000)); // Retorna o valor convertido para ppm
}




//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------Tinta tonalidade media ----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void controlaMotores(){
  if(count == TINTA){
    desce();                             //Primeira parada para pegar a tinta
    sobe(); 
    count++;
  }  
  luz = digitalRead(infra);
  abre(); 
  if(luz == HIGH && count <= PAPEL){           
    paraH();    
    if(count == PAPEL){                  //Parada para pintar no papel
      paraH();
      desce();
      fecha();
      abrePouco();
      sobe();
    }
    count++;                             //Acrescenta no contador porque parou mais uma vez   
    luz = digitalRead(infra);
  } 

  if(count > PAPEL) {                    //Verifica se o motor horizontal parou mais de 4 vezes para poder retornar a posicao inicial
    volta();
    count = 0;
  }

}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------Diluir a tinta ------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void controlaMotoresDiluir(int cor){

  if((cor % 2) == 0){                    //Se o valor da cor for par
    naAgua = 13-(cor/2) ;
    noPapel = (cor/2)+1;
  }
  else{                                  //Se o valor da cor for impar
    naAgua =  13-((cor-1)/2) ;
    noPapel = ((cor+1)/2)+1;
  } 

  if(count == TINTA){
    desce();                             //Primeira parada para pegar a tinta
    sobe(); 
    count++;
  }

  luz = digitalRead(infra);
  abre();

  if(luz == HIGH && count <= PAPEL){           
    paraH();    
    if(count == AGUA){                   //Parada para pegar agua
      paraH();
      desce();
      for(int i = 0;i< naAgua;i++){      //Meche o pincel na agua para diluir  
        if((i%2)==0) fecha();
        else abrePouco();        
      }
      sobe();
      if(!((naAgua%2)==0)) abrePouco();  //Caso seja impar os movimentos, deve reajustar a posicao
    }
    if(count == PAPEL){                  //Parada para pintar o papel papel
      paraH();
      desce();
      for(int i = 0;i< noPapel;i++){  
        if((i%2)==0) fecha();
        else abrePouco();        
      }
      sobe();
    }
    count++;                             //Acrescenta no contador porque parou mais uma vez   
    luz = digitalRead(infra);
  } 

  if(count > PAPEL) {                    //Verifica se o motor horizontal parou mais de 4 vezes para poder retornar a posicao inicial
    volta();
    count = 0;
  }
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------Escurecer a tinta -----------------------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void controlaMotoresEscurecer(int cor){ 

  if((cor % 2) == 0){                 //Se o valor da cor for par
    noPo =  ((cor-27)/2)+1;
    noPapel = ((cor-27+1)/2)+1;
  }
  else{                               //Se o valor da cor for impar
    noPo =   ((cor-27+1)/2);
    noPapel = ((cor-27+1)/2)+1;
  } 

  if(count == 0){
    desce();                         //Primeira parada para pegar a tinta
    sobe(); 
    count++;
  }  
  luz = digitalRead(infra);
  abre(); 
  if(luz == HIGH && count <= PAPEL){                 
    paraH();
    if(count == PO){                 //Parada para pegar po da tinta   
      desce();
      for(int i =0;i<noPo;i++){
        if((i%2)==0) fecha();
        else abrePouco();
      }
      sobe();
      if(!((noPo%2)==0)) abrePouco(); //Caso seja impar, deve reajustar a posicao
    }    
    if(count == PAPEL){               //Parada para pintar no papel 
     desce();
      for(int i = 0;i<noPapel;i++){ 
        if((i%2)==0) fecha();
        else abrePouco();
      }
      sobe();
    }
    count++;                          //Acrescenta no contador porque parou mais uma vez   
    luz = digitalRead(infra);
  } 

  if(count > PAPEL) {                 //Verifica se o motor horizontal parou mais de 4 vezes para poder retornar a posicao inicial
    volta();
    count = 0;
  }



}


//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------Funcoes auxiliares para o controle dos motores-------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void sobe(){  // Motor faz o pincel subir até a altura maxima
  //Sobe
  digitalWrite(IN3,LOW);
  digitalWrite(IN4,HIGH); 
  analogWrite(velocidadeB,140); 
  delay(270); 
  //Para motor vertical
  analogWrite(velocidadeB,0);
  delay(500);
}

void desce(){ // Motor faz o pincel descer até a altura minima
  //Desce
  digitalWrite(IN3,HIGH);
  digitalWrite(IN4,LOW); 
  analogWrite(velocidadeB,80); 
  delay(140); 
  //Para motor vertical
  analogWrite(velocidadeB,0);
  delay(500);
}


void abre(){  // Motor faz a bandeija abrir 
  //Abre
  digitalWrite(IN1,LOW);
  digitalWrite(IN2,HIGH);
  analogWrite(velocidadeA,180);
  delay(10);
}

void abrePouco(){ // Motor faz a bandeija abrir um pouco 
  //Abre
  digitalWrite(IN1,LOW);
  digitalWrite(IN2,HIGH);
  analogWrite(velocidadeA,180);
  delay(180);
  //Para motor horizontal
  analogWrite(velocidadeA,0);
  delay(500);
}

void fecha(){ // Motor faz a bandeija fechar um pouco 
  //Fecha
  digitalWrite(IN1,HIGH);
  digitalWrite(IN2,LOW);
  analogWrite(velocidadeA,180);
  delay(180);  
  //Para motor horizontal
  analogWrite(velocidadeA,0);
  delay(500);  
}

void volta(){ // Motor faz a bandeija voltar a posicao inicial
  //Fecha
  digitalWrite(IN1,HIGH);
  digitalWrite(IN2,LOW);
  analogWrite(velocidadeA,200);
  delay(700);  
  //Para motor horizontal
  analogWrite(velocidadeA,0);
  delay(1000);  
  count = 0;
}

void paraH(){ // Faz o motor parar a bandeija
  //Para motor Horizontal
  analogWrite(velocidadeA,0);
  delay(500);
}


