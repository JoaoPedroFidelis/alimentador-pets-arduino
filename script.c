#include <LiquidCrystal.h>
#include <Servo.h>

Servo motor; // Motor
LiquidCrystal lcd(12, 11, 5, 4, 3, 2); // Portas LCD

int /* bool */ motor_used = 0; // Motor foi usado ou nao.
int seconds = 0, hours = 0, minutes = 0; // Tempos exatos.
int h_limit = 0, m_limit = 1; // Tempos do timer - h: hora, m: minuto.
float time, offset = 0, time_mult = 1000; // Variaveis de controle de tempo

float add_time = 200; // Tempo em ms para servo voltar a 0 depois de abrir.
int motor_start_point = 0, motor_end_point = 45; // Ponto inicial e final do servo motor.
float motor_last_time; // Ultima vez que motor foi usado.

int motor_pin = 13; // Pino do motor. (Servo)
int config_btn = 6; // Pino do botão de configurações.

int left_btn = 7; // Pino de botão para selecionar o tempo da esquerda. (<)
int right_btn = 10; // Pino de botão para selecionar o tempo da direita. (>)
int plus_btn = 8; // Pino de botão de adicionar tempo. (+)
int minus_btn = 9; // Pino de botão de remover tempo. (-)

int time_selected = 0;  // Tempo selecionado para editar. (0-1 horas, 3-4 minutos)
int last_button_pressed; // Ultimo botão pressionado.
float button_offset = 100, button_timeout; // Controle de tempo de espera até botão poder ser pressionado de novo.

int cur_page = 0, last_page = -1, redraw = 0; // Controle de página no lcd.

char weeks[7][20] = {"Segunda-feira", "Terca-feira", "Quarta-feira", "Quinta-feira", "Sexta-feira", "Sabado", "Domingo"};
int cur_week = 0;

void setup()
{
  // Configurações de motor
  motor.attach(motor_pin);
  motor.write(motor_start_point);
  
  // Configurações de botões
  pinMode(config_btn, INPUT);
  pinMode(left_btn, INPUT);
  pinMode(right_btn, INPUT);
  pinMode(plus_btn, INPUT);
  pinMode(minus_btn, INPUT);
  
  // Configurações de tempo
  loadCurTime();
  loadLimitTime();
  
  // Configurações finais
  Serial.begin(9600);
  delay(100);
}

void loadCurTime(){
  // Adicionar códigos para pegar tempo da vida real.
  cur_week = 0;
  hours = 0;
  minutes = 0;
  seconds = 0;
  time_mult = 200; // Cada segundo é contado a 200ms do simulador.
}
void loadLimitTime(){
  // h_limit = (carregar arquivo que salva limite de tempo em horas);
  // m_limit = (carregar arquivo que salva limite de tempo em minutos);

  h_limit = 0;
  m_limit = 1;
  Serial.println("Tempo carregado");
}
void saveLimitTime(){
  // (Adicionar código para salvar tempo limite)
  Serial.println("Tempo salvo");
}

void changeTime(int change = 0){
  int mult = 1;
  if(time_selected == 0 || time_selected == 3) mult = 10;
  
  if(time_selected <= 1) h_limit += change * mult;
  else m_limit += change * mult;

  if(mult == 10){
    if(h_limit < 0) h_limit += 30;
    if(h_limit > 23) h_limit -= 20;

    if(m_limit < 0) m_limit += 60;
    if(m_limit > 59) m_limit -= 50;
  } else {
    if(h_limit < 0) h_limit = 23;
    if(h_limit > 23) h_limit = 0;

    if(m_limit < 0) m_limit = 59;
    if(m_limit > 59) m_limit = 0;
  }
}

void write_time(int x/* pos no lcd x */, int y/* pos no lcd y */, int h/* hora */, int m/* minutos */, int s = 0){ // Escreve o tempo no lcd no formato correto.
  if(m < 0){
    m = 60 + m;
  	h--;
  }
  if(h < 0) h = 24 + h;
  
  lcd.setCursor(x, y);
  if(h < 10) lcd.print(0);
  lcd.print(h);
  lcd.print(":");
  if(m < 10) lcd.print(0);
  lcd.print(m);
  if(s == 1){
    lcd.print(":");
    if(seconds < 10) lcd.print(0);
    lcd.print(seconds);
  }
  
  if(cur_page == 1){
    lcd.setCursor(x + time_selected, y);
    lcd.print(" ");
  }
}

void loop()
{ 
  redraw = 0;
  calculate_time();
  
  if(cur_page != last_page){
    lcd.clear();
    last_page = cur_page;
    redraw = 1;
  }
  
  if(cur_page == 0){
    // Página inicial com o dia da semana e o tempo em h:m:s
    if(redraw == 1){
      lcd.begin(16, 2);
      lcd.print(weeks[cur_week]);
    }
    write_time(0, 1, hours, minutes, 1);
  } else {
    // Página de configurações que permite mudar o tempo de entregar comida.
    if(redraw == 1){
      lcd.begin(16, 2);
      lcd.print("Configurar:");
      time_selected = 0;
    }
    write_time(0, 1, h_limit, m_limit, 0);
    
    if(pressed(plus_btn) && !was_pressed(plus_btn)) changeTime(1);
    if(pressed(minus_btn) && !was_pressed(minus_btn)) changeTime(-1);
    
    if(pressed(left_btn) && !was_pressed(left_btn)) time_selected--;
    else if(pressed(right_btn) && !was_pressed(right_btn)) time_selected++;
    
    if(time_selected < 0) time_selected = 4;
    if(time_selected == 2 && pressed(right_btn)) time_selected++;
    else if(time_selected == 2 && pressed(left_btn)) time_selected--;
    if(time_selected > 4) time_selected = 0;
  }
  
  if(pressed(config_btn) && !was_pressed(config_btn)) {
  	if(cur_page == 0) cur_page = 1;
    else cur_page = 0;
  }

  buttons_check();
  if(cur_page == 0) motors_check();
}


// Funções customizadas
void calculate_time(){ // Calcula o tempo em segundos, minutos e horas.
  time = millis();
  if(time >= offset + time_mult){
  	seconds ++;
    offset = time;
  }
  if(seconds >= 60){
  	seconds = 0;
    minutes++;
  }
  if(minutes >= 60){
    minutes = 0;
  	hours++;
  }
  if(hours >= 24) {
    hours = 0;
    cur_week++;
    redraw = 1;
  }
  if(cur_week > 6) cur_week = 0;
}

void buttons_check(){ // Ve qual botão foi pressionado e adiciona um delay para que não possa ser pressionado de novo.
  int buttons[] = {config_btn, plus_btn, minus_btn, left_btn, right_btn};
  int btn = -1;
  for(int i = 0; i < 5; i++){
    if(digitalRead(buttons[i]) == LOW) btn = buttons[i];
  }
  if(btn >= 0) {
  	last_button_pressed = btn;
    if(button_timeout == 0) button_timeout = time;
  }
  if(time >= button_timeout + button_offset){
  	last_button_pressed = -1;
    button_timeout = 0;
  }
}
bool pressed(int num){ // Ve caso o botão está pressionado.
  return digitalRead(num) == LOW;
}
bool was_pressed(int num){ // Ve caso o botão foi o ultimo a ser pressionado.
  return num == last_button_pressed;
}

void motors_check(){ // Responsavel pelo controle do motor ao horario limite bater.
  if(hours == h_limit && minutes == m_limit && motor_used == 0){
  	motor.write(motor_end_point);
    motor_last_time = time;
    motor_used = 1;
    Serial.print("Servo ");
    Serial.print(motor_end_point);
    Serial.println(" graus");
    Serial.print(time / 1000);
    Serial.println("s");
  }
  if(motor_used == 1 && time >= motor_last_time + add_time && motor_last_time != 0){
    motor.write(motor_start_point);
    motor_last_time = 0;
    Serial.print("Servo resetado (");
    Serial.print(add_time);
    Serial.println("ms)");
  }
  if(motor_used == 1 && hours != h_limit || minutes != m_limit) motor_used = 0;
}
