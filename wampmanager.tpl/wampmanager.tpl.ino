#include <IRremote.h>
#include <Servo.h> 
#include <time.h>

#define led_green 10
#define led_yellow 11
#define led_red 12
#define RECV_PIN 13
#define Shift 4
#define Clock 3
#define Data 2

Servo servo; 

int servoPin = 7;
int servoAngle = 0;   
int array_display[7];

unsigned char mapping[] = {0xFC, 0x60, 0xDA, 0xF2, 0x66, 0xB6, 0x3E, 0xE0, 0xFE, 0xE6};
int teclas[20] = {
-24714,2057,-30583,18505,-14135,10281,-22359,26729,-4883,6169,-28013,-3341,21075
};
/*0,1,2,3,4,5,6,7,8,9,OK, SETA_DIR,SETA_ESQ*/

int tx_byte;
int count, valor_entrada = 0;
int estado = 0;
int velocidade_servo = 5;
int estado_servo = 0;
int pressed_key;
IRrecv irrecv(RECV_PIN);
decode_results results;

/*
No setup os 
componentes são inicializados,
saidas do arduino são definidas e 
relogios do Shift registres alimentados com sinal baixo
*/
void setup() {
  Serial.begin(9600);
  irrecv.enableIRIn();
  servo.attach(servoPin);
  for (int i = 0; i <= 12; i++) {
    pinMode(i, OUTPUT);
  }
  digitalWrite(Shift, LOW);
  digitalWrite(Clock, LOW);

}


/*
No loop 
é verificado se algum sinal infravermelho conhecido é emitido e manuseado de acordo com seu valor e estado,
a função debug() comentada exibe no monitor serial os estados das variáveis do projeto
a função cronometro() executa o cronometro caso possível
a função handleservo () executa o servo motor caso possível
*/
void loop() {
  if (irrecv.decode(&results)) {
    unsigned int ir_return = results.value;    
    pressed_key = getIndCode(ir_return);
    handleKey(pressed_key);
    irrecv.resume();
  }
  //debug();
  cronometro();
  handleservo();
  delay(500);

}

/*
A função getIndCode() recebe como parâmetro um valor inteiro, e caso este valor esteja mapeado na variável teclas, então é retornado seu valor convertido.
Ex: getIndCode(-30583)->retorna botão 2 digitado
Caso o valor pressionado seja conhecido, o Led verde é aceso, representando visualmente uma entrada conhecida recebida.
*/
int getIndCode(int value) {
  int sizeArray = 12;
  for (int i = 0; i <= sizeArray; i++) {
    if (teclas[i] == value) {
      digitalWrite(led_green, 0);
      delay(200);
      digitalWrite(led_green, 1);
      delay(200);
      digitalWrite(led_green, 0);
      return i;
    }
  }

  return -1;
}
/*
A função cronometro() executa o cronometro caso o seu estado seja ativo (estado = 1) e o contador for maior que zero (count >=0).
nesta função, é chamada a função de escrever o número atual no display de 7 segmentos, e acender os leds verde, amarelo e vermelho, de acordo com o contador.
Caso o contador seja finalizado, o servo motor é iniciado (estado_servo=2).
*/
void cronometro() {
  if (estado == 0 || estado == 2 || count < 0) {
    return 0;
  }
  escreveNumero(count);
  handleLed(count, valor_entrada);
  count--;
  if(count<=0){
    estado_servo=2;
  }
  
}
/*
A função escreveNumero() recebe como parâmetro um número de 0 a 9, escreve no Shift register de e ao oscilar o "Clock", enviado ao display para ser exibido.
*/
void escreveNumero(int valor) {
  tx_byte = mapping[valor % 10] | 1;
  Serial.print("\n ");
  Serial.print(tx_byte, BIN);
  Serial.print("\n Escrevendo: ");
  for (int cnt = 7; cnt >= 0; cnt--) {
    array_display[cnt] = ((tx_byte & 0x01) == 0);
    tx_byte = tx_byte >> 1;
  }
  for (int cnt = 0; cnt <= 7; cnt++) {
    Serial.print(array_display[cnt]);
    digitalWrite(Data, array_display[cnt]);
    digitalWrite(Shift, LOW);
    digitalWrite(Shift, HIGH);
  }
  Serial.print(" (Send) \n");
  digitalWrite(Clock, LOW);
  digitalWrite(Clock, HIGH);
}
/*
A função debug() exibe os estados atuais no serial monitor, para que quem for executar entenda o passo-a-passo da exceção.
*/
void debug() {
  Serial.print("\n std_count: ");
  Serial.print(estado);
  Serial.print(" vlr_entrada: ");
  Serial.print(valor_entrada);
  Serial.print(" count: ");
  Serial.print(count); 
  Serial.print("  *Std servo ");
  Serial.print(estado_servo);
  Serial.print(" Vel ");
  Serial.print(velocidade_servo);

}
/*
A função handleKeyNumCron() recebe como parâmetro um valor inteiro.
Caso o cronometro esteja em execução, ele é pausado, salva o valor_entrada,e aguarda novos digitos (estado =2).
Caso o cronometro já esteja pausado, ele atualiza o valor_entrada para o número final.
*/
void handleKeyNumCron(int key) {
  if (estado == 1 || estado == 0) {
    estado = 2;
    valor_entrada = key;
    count = valor_entrada;
  } else {
    valor_entrada = valor_entrada * 10 + key;
    count = valor_entrada;
  }
}



/*
A função handleKeyFunCron() recebe como parâmetro um número inteiro, sendo que este deve representar apenas as funções OK, SETA_DIR e SETA_ESQ.
Caso SETA_DIR ou SETA_ESQ sejam pressionados, o cronômetro é pausado e o valor do contador incrementado ou decrementado em um.
Caso OK seja pressionado, o cronômetro é pausado ou continuado.
*/
void handleKeyFunCron(int key) {
  switch (key) {
  case 10:
    if (estado == 1) {
      estado = 0;
    } else if (estado == 2) { 
      estado = 1;
    } else {
      estado = 1;
    }
    break;
  case 11:
    estado = 0;
    count++;
    break;
  case 12:
    estado = 0;
    if (count > 0) {
      count--;
      if(count==0){
        estado_servo=2;
      }
    }
  case -1:
  default:
    break;
  }
}
/*
A função handleKeyFuncServo() recebe como parâmetro um valor numérico que manuseia as funções do Servo.
Caso o botão OK seja pressionado, o servo pausa/continua.
Caso os botões SETA_DIR ou SETA_ESQ sejam pressionados, a velocidade do servo é aumentada/diminuída em um.
*/
void handleKeyFuncServo(int key) {
  switch (key) {
  case 10:
    if (estado_servo == 2) {
      estado_servo = 1;
   } else {
      estado_servo = 2;
    }
    break;
  case 11:
    estado_servo = 2;
    if(velocidade_servo<10){
      velocidade_servo++;
    }
    break;
  case 12:
    estado_servo = 2;
    if (velocidade_servo > 1) {
      velocidade_servo--;
    }
  case -1:
  default:
    break;
  }
}
/*
A função handleLed() recebe como parametros os valores inteiros contador (count) e valor inicial (valor_entrada), e compara-os.
Caso o contador seja maior que a metade do valor inicial, então o led verde é acesso.
Caso o contador seja menor que a metade do valor inicial, então o led amarelo é acesso.
Caso o contador seja igual a zero então o led vermelho é acesso.
*/
void handleLed(int count, int valor_entrada) {
  if (count > 0) {
    int progressao = valor_entrada / count;
    if (progressao < 2) {
      digitalWrite(led_green, 1);
      digitalWrite(led_yellow, 0);
      digitalWrite(led_red, 0);
    } else {
      digitalWrite(led_yellow, 1);
      digitalWrite(led_green, 0);
      digitalWrite(led_red, 0);
    }
  } else {
    digitalWrite(led_red, 1);
    digitalWrite(led_green, 0);
    digitalWrite(led_yellow, 0);
  }
}

/*
A função handleKey() recebe como parametro a representação numérica ou funcional da tecla do controle remoto pressionado.
Caso o cronometro esteja ativo e: 
Caso o parâmetro seja um número (entre 0 e 9), então a função handleKeyNumCron() é chamada.
Caso o parâmetro seja uma função (10, 11 ou 12), (OK, SETA_DIR, SETA_ESQ), então a funçao handleKeyFunCron() é chamada.
Caso o cronometro não esteja ativo, o servo é controlado pela função handleKeyFuncServo().
*/
void handleKey(int key) {
  if (estado_servo == 0) {
      if (key >= 0 && key <= 9) {
        handleKeyNumCron(key);
      } else {
        handleKeyFunCron(key);
      }
    } else {
    handleKeyFuncServo(key);
  }
}
/*
A função handleservo() verifica se o servo esta ativo, e caso esteja, ela faz com que ele funcione de acordo com a velocidade definida pelo usuario (velocidade_servo).
*/
void handleservo(){
  if(estado_servo==2){
    for(int i =0;i<=180;i+=20){
     servo.write(i);
     delay(500/velocidade_servo);
    }
  }
}

