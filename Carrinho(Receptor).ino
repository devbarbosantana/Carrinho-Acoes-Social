#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

// Struct idêntica à do Uno
struct __attribute__((packed)) Pacote {
  int16_t x;
  int16_t y;
  uint8_t botao;
};
Pacote dados;

// CE no GPIO 2, CSN no GPIO 15
RF24 radio(2, 15);
const byte endereco[6] = "00001";

// Matriz de motores original do seu código
int motoresPins[4][2] = {
  {13, 14}, // motor esquerdo
  {4, 27},  // motor direito
  {26, 25}, // motor da arma
  {32, 33}  // motor extra
};

const int quantidadeMotores = sizeof(motoresPins) / sizeof(motoresPins[0]);

bool controlesInvertidos = false;
String direcao = "";
bool motorAAtivo = false;
unsigned long ultimaRecepcao = 0; 

void frente() {
  for (int i = 0; i < 2; i++) {
    digitalWrite(motoresPins[i][0], HIGH);
    digitalWrite(motoresPins[i][1], LOW);
  }
}

void tras() {
  for (int i = 0; i < 2; i++) {
    digitalWrite(motoresPins[i][0], LOW);
    digitalWrite(motoresPins[i][1], HIGH);
  }
}

void direita() {
  digitalWrite(motoresPins[0][0], LOW);  digitalWrite(motoresPins[0][1], HIGH);
  digitalWrite(motoresPins[1][0], HIGH); digitalWrite(motoresPins[1][1], LOW);
}

void esquerda() {
  digitalWrite(motoresPins[0][0], HIGH); digitalWrite(motoresPins[0][1], LOW);
  digitalWrite(motoresPins[1][0], LOW);  digitalWrite(motoresPins[1][1], HIGH);
}

void parado() {
  for (int i = 0; i < 2; i++) {
    digitalWrite(motoresPins[i][0], LOW);
    digitalWrite(motoresPins[i][1], LOW);
  }
}

void aplicarMovimento() {
  if (direcao == "F") frente();
  else if (direcao == "T") tras();
  else if (direcao == "E") esquerda();
  else if (direcao == "D") direita();
  else parado();
}

void botaoA(bool ligar) {
  motorAAtivo = ligar;
  if (ligar) {
    if (controlesInvertidos) {
      digitalWrite(motoresPins[2][0], HIGH); digitalWrite(motoresPins[2][1], LOW);
    } else {
      digitalWrite(motoresPins[2][0], LOW);  digitalWrite(motoresPins[2][1], HIGH);
    }
  } else {
    digitalWrite(motoresPins[2][0], LOW);   digitalWrite(motoresPins[2][1], LOW);
  }
}

void botaoB() {
  parado();
  digitalWrite(motoresPins[2][0], LOW);
  digitalWrite(motoresPins[2][1], LOW);
  direcao = "N";
  motorAAtivo = false;
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n--- Receptor nRF24L01 (HW-237) Iniciado ---");

  for (int i = 0; i < quantidadeMotores; i++) {
    pinMode(motoresPins[i][0], OUTPUT);
    pinMode(motoresPins[i][1], OUTPUT);
  }
  botaoB(); 

  SPI.begin(18, 19, 23, 15); 

  if (!radio.begin()) {
    Serial.println("Erro Crítico: Rádio HW-237 não responde!");
    while (1);
  }

  radio.openReadingPipe(1, endereco);
  
  // --- AJUSTES ESPECÍFICOS PARA O RÁDIO COM ANTENA ---
  radio.setPALevel(RF24_PA_MAX);    // Configura potência máxima (Ideal para a arena)
  radio.setDataRate(RF24_250KBPS);  // Velocidade baixa = Máxima distância e barreira contra ruídos
  radio.setRetries(15, 15);         // Máximo de tentativas de reenvio
  
  radio.startListening();
  Serial.println("Pronto e ouvindo!");
}

void loop() {
  if (radio.available()) {
    radio.read(&dados, sizeof(Pacote));
    ultimaRecepcao = millis(); 

    if (dados.x > 1024 || dados.y > 1024) return;

    // Conversão do analógico para o seu sistema de Strings
    if (dados.y > 700) direcao = "F";
    else if (dados.y < 300 && dados.y > 5) direcao = "T";
    else if (dados.x > 700) direcao = "D";
    else if (dados.x < 300) direcao = "E";
    else direcao = "N";
    
    aplicarMovimento();

    if (dados.botao == 1) botaoA(true);
    else if (dados.botao == 2) botaoA(false);
  }

  // Failsafe: se perder o controle por 1 segundo, desliga tudo
  if (millis() - ultimaRecepcao > 1000) {
    botaoB();
  }
}
