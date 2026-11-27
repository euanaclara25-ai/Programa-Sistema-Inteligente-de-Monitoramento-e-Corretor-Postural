// Partcipantes do grupo: Ana Clara Fernandes Gonçalves, Bruna Souza Sudário França, Davi Leza Piló.
//bibliotecas baixadas para a execução do projeto: Adafruit MPU6050, Adafruit Unified Sensor.

#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <math.h>

Adafruit_MPU6050 mpu;

// Pinos
const int motorPin = 9;
const int botaoCalibracao = 2;
const int botaoReset = 5;

// Configuracoes
const float limiteDesvio = 10.0;

const unsigned long tempoEspera = 5000;
const unsigned long debounce = 50;

float anguloReferencia = 0;
float anguloAtual = 0;
float desvio = 0;

bool calibrado = false;
bool motorLigado = false;
bool contandoTempo = false;

// Controle do botao de calibracao
bool leituraAnterior = LOW;
bool estadoBotao = LOW;

unsigned long tempoBotao = 0;

// Controle do botao de reset
bool leituraAnteriorReset = LOW;
bool estadoBotaoReset = LOW;

unsigned long tempoBotaoReset = 0;

// Controle de tempo
unsigned long tempoInicial = 0;


// =================================
// LEITURA DO ANGULO
// =================================

float lerAngulo() {

  sensors_event_t a, g, temp;

  mpu.getEvent(&a, &g, &temp);

  float angulo = atan2(
    a.acceleration.y,
    a.acceleration.z
  ) * 180.0 / PI;

  return angulo;
}


// =================================
// CALIBRACAO DA POSTURA
// =================================

void calibrarPostura() {

  digitalWrite(motorPin, LOW);

  motorLigado = false;
  contandoTempo = false;

  float somaY = 0;
  float somaZ = 0;

  sensors_event_t a, g, temp;

  Serial.println();
  Serial.println("================================");
  Serial.println("CALIBRANDO POSTURA...");
  Serial.println("Fique na posicao correta!");
  Serial.println("================================");

  for (int i = 0; i < 50; i++) {

    mpu.getEvent(&a, &g, &temp);

    somaY += a.acceleration.y;
    somaZ += a.acceleration.z;

    delay(20);
  }

  float mediaY = somaY / 50.0;
  float mediaZ = somaZ / 50.0;

  anguloReferencia = atan2(mediaY, mediaZ)
                     * 180.0 / PI;

  calibrado = true;

  Serial.print("Angulo de referencia: ");
  Serial.println(anguloReferencia, 2);

  Serial.println("CALIBRACAO CONCLUIDA!");
  Serial.println("Monitoramento iniciado.");
  Serial.println();

  tempoInicial = 0;
  contandoTempo = false;
}


// =================================
// RESET DA CALIBRACAO
// =================================

void resetarCalibracao() {

  // Desliga o motor
  digitalWrite(motorPin, LOW);

  motorLigado = false;
  contandoTempo = false;

  // Apaga a calibracao anterior
  calibrado = false;

  anguloReferencia = 0;
  anguloAtual = 0;
  desvio = 0;

  tempoInicial = 0;

  Serial.println();
  Serial.println("================================");
  Serial.println("CALIBRACAO RESETADA!");
  Serial.println("A referencia anterior foi apagada.");
  Serial.println("Pressione o botao de calibracao");
  Serial.println("para registrar uma nova postura.");
  Serial.println("================================");
  Serial.println();
}


// =================================
// INICIALIZACAO
// =================================

void setup() {

  Serial.begin(115200);

  pinMode(motorPin, OUTPUT);

  // Botoes com resistores externos de pull-down
  pinMode(botaoCalibracao, INPUT);
  pinMode(botaoReset, INPUT);

  digitalWrite(motorPin, LOW);

  Wire.begin();

  Serial.println("Iniciando sistema...");

  if (!mpu.begin()) {

    Serial.println("ERRO: MPU6050 NAO ENCONTRADO!");

    while (1) {
      digitalWrite(motorPin, LOW);
      delay(100);
    }
  }

  // Configuracao do sensor
  mpu.setAccelerometerRange(
    MPU6050_RANGE_4_G
  );

  mpu.setGyroRange(
    MPU6050_RANGE_500_DEG
  );

  mpu.setFilterBandwidth(
    MPU6050_BAND_21_HZ
  );

  Serial.println("MPU6050 encontrado!");
  Serial.println("Sistema iniciado!");
  Serial.println();
  Serial.println("Posicione as costas corretamente.");
  Serial.println("Pressione o botao de calibracao.");
}


// =================================
// LOOP PRINCIPAL
// =================================

void loop() {

  // =================================
  // BOTAO DE CALIBRACAO
  // =================================

  bool leituraBotao = digitalRead(botaoCalibracao);

  if (leituraBotao != leituraAnterior) {

    tempoBotao = millis();

    leituraAnterior = leituraBotao;
  }

  if (millis() - tempoBotao > debounce) {

    if (leituraBotao != estadoBotao) {

      estadoBotao = leituraBotao;

      // Botao pressionado (HIGH)
      if (estadoBotao == HIGH && !calibrado) {

        calibrarPostura();
      }
    }
  }


  // =================================
  // BOTAO DE RESET
  // =================================

  bool leituraReset = digitalRead(botaoReset);

  if (leituraReset != leituraAnteriorReset) {

    tempoBotaoReset = millis();

    leituraAnteriorReset = leituraReset;
  }

  if (millis() - tempoBotaoReset > debounce) {

    if (leituraReset != estadoBotaoReset) {

      estadoBotaoReset = leituraReset;

      // Botao pressionado (HIGH)
      if (estadoBotaoReset == HIGH && calibrado) {

        resetarCalibracao();
      }
    }
  }


  // =================================
  // AGUARDA CALIBRACAO
  // =================================

  if (!calibrado) {

    digitalWrite(motorPin, LOW);

    return;
  }


  // =================================
  // LEITURA DO MPU6050
  // =================================

  anguloAtual = lerAngulo();

  // Calcula desvio da referencia
  desvio = anguloAtual - anguloReferencia;

  // Corrige valores acima de 180 graus
  if (desvio > 180) {
    desvio -= 360;
  }

  if (desvio < -180) {
    desvio += 360;
  }


  // =================================
  // VERIFICACAO DA POSTURA
  // =================================

  if (fabs(desvio) > limiteDesvio) {

    if (!motorLigado) {

      if (!contandoTempo) {

        tempoInicial = millis();

        contandoTempo = true;

        Serial.println("Postura incorreta!");
        Serial.println("Iniciando contagem de 5 segundos...");
      }

      // Aguarda 5 segundos
      if (millis() - tempoInicial >= tempoEspera) {

        motorLigado = true;

        contandoTempo = false;

        Serial.println("ATENCAO! MOTOR ATIVADO!");
      }
    }
  }

  else {

    // Postura corrigida
    motorLigado = false;

    contandoTempo = false;

    tempoInicial = 0;
  }


  // =================================
  // CONTROLE DO MOTOR
  // =================================

  digitalWrite(
    motorPin,
    motorLigado ? HIGH : LOW
  );


  // =================================
  // MONITOR SERIAL
  // =================================

  Serial.print("Angulo atual: ");
  Serial.print(anguloAtual, 2);

  Serial.print(" | Referencia: ");
  Serial.print(anguloReferencia, 2);

  Serial.print(" | Desvio: ");
  Serial.print(desvio, 2);

  Serial.print(" | Contagem: ");

  if (contandoTempo) {

    Serial.print(
      (millis() - tempoInicial) / 1000.0
    );

    Serial.print(" s");

  } else {

    Serial.print("Parada");
  }

  Serial.print(" | Motor: ");

  Serial.println(
    motorLigado ? "LIGADO" : "DESLIGADO"
  );

  delay(100);
}
