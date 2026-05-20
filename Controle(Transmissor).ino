#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

// Struct idêntica e compactada para casar com a ESP32
struct __attribute__((packed)) Pacote {
  int16_t x;
  int16_t y;
  uint8_t botao; // 1=A+, 2=A-, 3=B, 4=X, 5=Y
};

Pacote dados;
RF24 radio(9, 10); // CE no pino 9, CSN no pino 10
const byte endereco[6] = "00001";

// Mapeamento dos botões físicos no Joystick Shield do Uno
#define PIN_BTN_A 2 // Botão de cima
#define PIN_BTN_B 3 // Botão da direita
#define PIN_BTN_C 4 // Botão de baixo
#define PIN_BTN_D 5 // Botão da esquerda

// No Joystick Shield do Uno, os eixos analógicos são A0 e A1
#define PIN_JOY_X A0 
#define PIN_JOY_Y A1 

bool invertido = false; // Se verdadeiro, inverte os controles (Robô de cabeça para baixo)
uint8_t ultimoBotaoEnviado = 0;

// Tradução da sua lógica "obterDirecao" adaptada para a escala do Arduino Uno (0 a 1023)
void processarJoystick(int x, int y, bool inv) {
  // Ponto central padrão do Uno fica próximo de 512.
  // Criamos uma janela de folga (neutro) entre 400 e 600.
  
  if (!inv) {
    if (x > 850 && y > 400 && y < 620)       { dados.y = 1023; dados.x = 512;  } // Frente
    else if (x < 150 && y > 400 && y < 620)  { dados.y = 0;    dados.x = 512;  } // Trás
    else if (y > 850 && x > 400 && x < 620)  { dados.x = 1023; dados.y = 512;  } // Direita
    else if (y < 150 && x > 400 && x < 620)  { dados.x = 0;    dados.y = 512;  } // Esquerda
    else                                     { dados.x = 512;  dados.y = 512;  } // Neutro
  } else {
    // Modo Invertido (Robô capotado)
    if (x > 850 && y > 400 && y < 620)       { dados.y = 0;    dados.x = 512;  } // Frente vira Trás
    else if (x < 150 && y > 400 && y < 620)  { dados.y = 1023; dados.x = 512;  } // Trás vira Frente
    else if (y > 850 && x > 400 && x < 620)  { dados.x = 0;    dados.y = 512;  } // Direita vira Esquerda
    else if (y < 150 && x > 400 && x < 620)  { dados.x = 1023; dados.y = 512;  } // Esquerda vira Direita
    else                                     { dados.x = 512;  dados.y = 512;  } // Neutro
  }
}

void setup() {
  Serial.begin(9600);
  Serial.println("\n--- Transmissor nRF24L01 Uno Pronto ---");

  // Inicializa botões do Shield como INPUT_PULLUP (Chaveiam para GND)
  pinMode(PIN_BTN_A, INPUT_PULLUP);
  pinMode(PIN_BTN_B, INPUT_PULLUP);
  pinMode(PIN_BTN_C, INPUT_PULLUP);
  pinMode(PIN_BTN_D, INPUT_PULLUP);

  if (!radio.begin()) {
    Serial.println("ERRO: Rádio nRF24L01 não respondeu!");
    while (1);
  }

  radio.openWritingPipe(endereco);
  
  // Configurações do seu novo rádio com antena HW-237 para máxima estabilidade
  radio.setPALevel(RF24_PA_MAX);    
  radio.setDataRate(RF24_250KBPS);  
  radio.setRetries(15, 15);         
  
  radio.stopListening();            
}

void loop() {
  // 1. Lê os valores do analógico
  int xValue = analogRead(PIN_JOY_X);
  int yValue = analogRead(PIN_JOY_Y);

  // 2. Processa as direções e aplica inversão caso ativa
  processarJoystick(xValue, yValue, invertido);

  // 3. Lógica dos Botões baseada na sua temporização e ação
  static unsigned long lastBtnB = 0;
  static unsigned long lastBtnD = 0;
  const unsigned long debounceDelay = 300;
  unsigned long now = millis();

  // Botão A (Segurar para manter Arma Ativa - Igual ao seu comando A+ / A-)
  // No Shield, vamos usar o Botão A (Pino 2) para
