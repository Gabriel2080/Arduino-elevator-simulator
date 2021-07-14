#include <Adafruit_NeoPixel.h>

#define sobe 13  
#define elevador 12
#define destino 11
#define desce 10

#define NUMPIXELS      10 

Adafruit_NeoPixel led_sobe = Adafruit_NeoPixel(NUMPIXELS, sobe, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel led_elevador = Adafruit_NeoPixel(NUMPIXELS, elevador, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel led_destino = Adafruit_NeoPixel(NUMPIXELS, destino, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel led_desce = Adafruit_NeoPixel(NUMPIXELS, desce, NEO_GRB + NEO_KHZ800);

int redColor = 255;
int greenColor = 0;
int blueColor = 0;

const int operante = 7;
const int porta_abeta  = 6;
const int emergencia = 5;

const int botaes_elevador = 2;
const int botaes_de_fora = 3;

bool tecla_ON_apertada;
bool tecla_OFF_apertada;
bool tecla_emergencia = false;

int andar_corrente = 0;

bool subindo[10] = {false,false,false,false,false,false,false,false,false,false};
bool decendo[10] = {false,false,false,false,false,false,false,false,false,false};
bool destino_elevador[10] = {false,false,false,false,false,false,false,false,false,false};

enum { INOPERANTE,
       OCIOSO,
       ALINHADO_S,
       MOVENDO_S,
       ESTACIONADO_S,
       FIM_S,
       ALINHADO_D,
       MOVENDO_D,
       ESTACIONADO_D,
       FIM_D} estado = INOPERANTE;

void setup() {
  Serial.begin(9600);
  
  led_sobe.begin();
  led_elevador.begin();
  led_destino.begin();
  led_desce.begin();
  
  pinMode(operante, OUTPUT);
  pinMode(porta_abeta, OUTPUT);
  pinMode(emergencia, OUTPUT);
  
  pinMode(botaes_elevador, INPUT_PULLUP);
  attachInterrupt(botaes_elevador - 2, botao_elevador, FALLING);
  pinMode(botaes_de_fora, INPUT_PULLUP);
  attachInterrupt(botaes_de_fora - 2, botao_de_fora, FALLING);
}

void loop() {
  
  Serial.print("Entrando no loop ");
  Serial.print("[");
  Serial.print(estado);
  Serial.print("] ");
  
  switch (estado)
  {
    case INOPERANTE: estado_INOPERANTE(); break;
    case OCIOSO: estado_OCIOSO(); break;
    case ALINHADO_S: estado_ALINHADO_S(); break;
    case MOVENDO_S: estado_MOVENDO_S(); break;
    case ESTACIONADO_S: estado_ESTACIONADO_S(); break;
    case FIM_S: estado_FIM_S(); break;
    case ALINHADO_D: estado_ALINHADO_D(); break;
    case MOVENDO_D: estado_MOVENDO_D(); break;
    case ESTACIONADO_D: estado_ESTACIONADO_D(); break;
    case FIM_D: estado_FIM_D(); break;
    default: Serial.println("ESTADO INVÁLIDO");
  }
  
  delay(1000);
}

void estado_INOPERANTE()
{
  Serial.println("==> INOPERANTE");
  
  mostrar_elevador(andar_corrente,255,0,0); // red
  desmarcar_demandas();
  
  tecla_ON_apertada = false;
  
  while (!tecla_ON_apertada)	
  {	
    delay(500);	//	não é necessário,	mas	evita testar demais
  }
  tecla_OFF_apertada = false;
  digitalWrite(operante, HIGH);
  estado = OCIOSO;
}

void estado_OCIOSO()
{
  Serial.println("==> OCIOSO");
  
  mostrar_elevador(andar_corrente,0,128,0); // verde
  
  if(ha_emergencia()){
    digitalWrite(porta_abeta, HIGH);
    desmarcar_emergencia();
    delay(3000);
  	porta_fecha();
  }
  
  if(tecla_OFF_apertada==true){
    //Serial.println("off");
    digitalWrite(operante, LOW);
    estado = INOPERANTE;
  }
  
  if(ha_demanda_abaixo(andar_corrente) || ha_chamada_D(andar_corrente)){estado = ALINHADO_D;}
  if(ha_demanda_acima(andar_corrente) || ha_chamada_S(andar_corrente) || ha_destino(andar_corrente)){estado = ALINHADO_S;}
  
  
}

void estado_ALINHADO_S()
{
  Serial.println("==> ALINHADO_S");
  mostrar_elevador(andar_corrente,0,128,0); // verde
  //Serial.println(ha_demanda_acima(andar_corrente));
  if(!ha_chamada_S(andar_corrente) && !ha_destino(andar_corrente) && !ha_emergencia() && !ha_demanda_acima(andar_corrente)){
    estado = FIM_S;
  }
  else if(!ha_chamada_S(andar_corrente) && !ha_destino(andar_corrente) && !ha_emergencia() && ha_demanda_acima(andar_corrente)){
    estado = MOVENDO_S;
  }
  else if(ha_chamada_S(andar_corrente) || ha_emergencia() || ha_destino(andar_corrente)){
    estado = ESTACIONADO_S;
  }
  
}

void estado_MOVENDO_S()
{
  Serial.println("==> MOVENDO_S");
  andar_corrente++;
  estado = ALINHADO_S;
}

void estado_ESTACIONADO_S()
{ 
  Serial.println("==> ESTACIONADO_S");
  digitalWrite(porta_abeta, HIGH);
  desmarcar_S(andar_corrente);
  desmarcar_destino(andar_corrente);
  desmarcar_emergencia();
  delay(3000);
  porta_fecha();
  estado = ALINHADO_S; 
}

void estado_FIM_S()
{
  Serial.println("==> FIM_S");
  //Serial.println(ha_demanda_abaixo(andar_corrente));
  if(!ha_demanda_abaixo(andar_corrente) && !ha_chamada_D(andar_corrente)){
    estado = OCIOSO;
  }
  else if(ha_demanda_abaixo(andar_corrente) || ha_chamada_D(andar_corrente)){
    estado = ALINHADO_D;
  }
}

void estado_ALINHADO_D()
{
  Serial.println("==> ALINHADO_D");
  mostrar_elevador(andar_corrente,0,128,0);
  //Serial.println(ha_demanda_acima(andar_corrente));
  if(!ha_chamada_D(andar_corrente) && !ha_destino(andar_corrente) && !ha_emergencia() && !ha_demanda_abaixo(andar_corrente)){
    estado = FIM_D;
  }
  else if(!ha_chamada_D(andar_corrente) && !ha_destino(andar_corrente) && !ha_emergencia() && ha_demanda_abaixo(andar_corrente)){
    estado = MOVENDO_D;
  }
  else if(ha_chamada_D(andar_corrente) || ha_emergencia() || ha_destino(andar_corrente)){
    estado = ESTACIONADO_D;
  }
}

void estado_MOVENDO_D()
{
  Serial.println("==> MOVENDO_D");
  andar_corrente--;
  estado = ALINHADO_D;
}

void estado_ESTACIONADO_D()
{
  Serial.println("==> ESTACIONADO_D");
  digitalWrite(porta_abeta, HIGH);
  desmarcar_D(andar_corrente);
  desmarcar_destino(andar_corrente);
  desmarcar_emergencia();
  delay(3000);
  porta_fecha();
  estado = ALINHADO_D;
}

void estado_FIM_D()
{
  Serial.println("==> FIM_D");
  if(!ha_demanda_acima(andar_corrente) && !ha_chamada_S(andar_corrente)){
    estado = OCIOSO;
  }
  else if(ha_demanda_acima(andar_corrente) || ha_chamada_S(andar_corrente)){
    estado = ALINHADO_S;
  }
}

void mostrar_elevador(int andar, int r, int g, int b){
  led_elevador.setPixelColor(andar, led_elevador.Color(r, g, b));
  led_elevador.setPixelColor((andar-1), 0,0,0); // apaga
  led_elevador.setPixelColor((andar+1), 0,0,0);
  led_elevador.show();
}

void marcar_destino(int x){
  led_destino.setPixelColor(x, led_destino.Color(255,255,0));
  destino_elevador[x]= true;
  led_destino.show();
}

void marcar_S(int x){
  led_sobe.setPixelColor(x, led_sobe.Color(255,255,0));
  subindo[x]= true;
  led_sobe.show();
}

void marcar_D(int x){
  led_desce.setPixelColor(x, led_desce.Color(255,255,0));
  decendo[x]= true;
  led_desce.show();
}

void marcar_emergencia(){
  tecla_emergencia = true;
  digitalWrite(emergencia, HIGH);
}

void desmarcar_destino(int x){
  led_destino.setPixelColor(x, 0,0,0);
  destino_elevador[x]= false;
  led_destino.show();
}

void desmarcar_S(int x){
  led_sobe.setPixelColor(x, 0,0,0);
  subindo[x]= false;
  led_sobe.show();
}

void desmarcar_D(int x){
  led_desce.setPixelColor(x, 0,0,0);
  decendo[x]= false;
  led_desce.show();
}

void desmarcar_emergencia(){
  tecla_emergencia = false;
  digitalWrite(emergencia, LOW);
}

void porta_fecha(){
  digitalWrite(porta_abeta, LOW);
}

void desmarcar_demandas(){
  for(int i=0;i<NUMPIXELS;i++){
    desmarcar_destino(i);
    desmarcar_S(i);
    desmarcar_D(i);
  }
}

bool ha_emergencia(){
  return (tecla_emergencia);
}

bool ha_destino(int x){
  return destino_elevador[x];
}

bool ha_chamada_S(int x){
  return subindo[x];
}

bool ha_chamada_D(int  x) {
  return decendo[x];
}

bool ha_demanda_acima(int x) {
  for(int i=x+1;i<NUMPIXELS;i++){
    if (ha_destino(i)== true){return true;}
    else if (ha_chamada_S(i)== true){return true;}
    else if (ha_chamada_D(i)== true){return true;}
  }
  return false;
}

bool ha_demanda_abaixo(int x){
  for(int i=0;i<x;i++){
    if (ha_destino(i)== true){return true;}
    else if (ha_chamada_S(i)== true){return true;}
    else if (ha_chamada_D(i)== true){return true;}
  }
  return false;
}

void botao_de_fora()
{
  Serial.print("Botao apertado: ");
  int codigo = analogRead(A5);
  Serial.println(codigo);
  
  switch (codigo)
  {
    case 40: marcar_S(0); break; // T
    case 77: marcar_S(1); break; // 1
    case 112: marcar_S(2); break; // 2
    case 144: marcar_S(3); break; // 3
    case 173: marcar_S(4); break; // 4
    case 201: marcar_S(5); break; // 5
    case 227: marcar_S(6); break; // 6
    case 252: marcar_S(7); break; // 7
    case 275: marcar_S(8); break; // 8
    
    case 296: marcar_D(1); break; // 1
    case 317: marcar_D(2); break; // 2
    case 336: marcar_D(3); break; // 3
    case 355: marcar_D(4); break; // 4
    case 372: marcar_D(5); break; // 5
    case 388: marcar_D(6); break; // 6
    case 404: marcar_D(7); break; // 7
    case 419: marcar_D(8); break; // 8
    case 433: marcar_D(9); break; // 9
  }
}

void botao_elevador()
{
  Serial.print("Botao apertado: ");
  int codigo = analogRead(A4);
  Serial.println(codigo);
  
  switch (codigo)
  {
    case 40: marcar_destino(0); break; // T
    case 77: marcar_destino(1); break; // 1
    case 112: marcar_destino(2); break; // 2
    case 144: marcar_destino(3); break; // 3
    case 173: marcar_destino(4); break; // 4
    case 201: marcar_destino(5); break; // 5
    case 227: marcar_destino(6); break; // 6
    case 252: marcar_destino(7); break; // 7
    case 275: marcar_destino(8); break; // 8
    case 296: marcar_destino(9); break; // 9
    case 317: porta_fecha(); break; // FECHAR PORTA
    case 336: marcar_emergencia(); break; // EMERGENCIA
    case 355: tecla_ON_apertada = true; break;
    case 372: tecla_OFF_apertada = true; break;
  }
}