/*
 ___  __  __   _    __
   / |   |  | | |  |  |
  /  |-  |--  |-\  |--|
 /__ |__ |__| |  \ |  |
 
 Zebra 1.0
 Voiture radiocommandée
 jcl 2021 - jcenligne.fr

*/ 

#include <Servo.h> 
#include <PS2X_lib.h> 

#define mstop		0
#define mavant		1
#define marriere	2
#define mlateral	3
#define mdiagonal	4
#define mdroite		5
#define mgauche		6
#define slow 10
#define full 400
#define stepRun 5
#define rapportmax 10
#define minguidon 20
#define maxguidon 170
#define stepguidon 5

// moteur shield

const int motorArriereGauche_pwm = 4; // PWM
const int motorArriereGauche_dir = 3; // DIR
int motorArriereGauche_state = mstop;
int motorArriereGauche_speed = slow;

const int motorArriereDroite_pwm = 19; // PWM
const int motorArriereDroite_dir = 18; // DIR
int motorArriereDroite_state = mstop;
int motorArriereDroite_speed = slow;

const int motorAvantGauche_pwm = 7; // PWM
const int motorAvantGauche_dir = 5; // DIR
int motorAvantGauche_state = mstop;
int motorAvantGauche_speed = slow;

const int motorAvantDroite_pwm = 8; // PWM
const int motorAvantDroite_dir = 9; // DIR
int motorAvantDroite_state = mstop;
int motorAvantDroite_speed = slow;

int speedRun = 0;
int rapport = 0;

PS2X ps2x; 
byte vibrate = 0;

Servo servotete;
int postete = 90;

bool modeAuto = false;

// Leds
// Analog pin leds
//  17  PHARE BLEU
//  16  PHARE ROUGE
//  15  PHARE VERT

bool ledphare = false;
bool ledphareBleu = true;
bool ledphareRouge = true;
bool ledphareVert = true;
const int ledphareBleuPin = 17;
const int ledphareRougePin = 16;
const int ledphareVertPin = 15;

// Sonar ( not in use )

const byte TRIGGER_PIN = 2;
const byte ECHO_PIN = 14;
const unsigned long MEASURE_TIMEOUT = 25000UL; // 25ms = ~8m à 340m/s
const float SOUND_SPEED = 340.0 / 1000;	//en mm/us

void setup(){

  Serial.begin(57600);
  Serial.println("Zebra init");

  // shield moteurs
  Serial.print("Shields moteurs ");
  Serial.print("arriere gauche | ");
  pinMode(motorArriereGauche_pwm, OUTPUT);
  pinMode(motorArriereGauche_dir, OUTPUT);
  Serial.print("arriere droite |");
  pinMode(motorArriereDroite_pwm, OUTPUT);
  pinMode(motorArriereDroite_dir, OUTPUT);
  Serial.print("avant gauche |");
  pinMode(motorAvantGauche_pwm, OUTPUT);
  pinMode(motorAvantGauche_dir, OUTPUT);
  Serial.print("avant droite");
  pinMode(motorAvantDroite_pwm, OUTPUT);
  pinMode(motorAvantDroite_dir, OUTPUT);

  // moteurs break
  digitalWrite(motorArriereGauche_pwm, LOW);
  digitalWrite(motorArriereGauche_dir, LOW);
  digitalWrite(motorArriereDroite_pwm, LOW);
  digitalWrite(motorArriereDroite_dir, LOW);
  digitalWrite(motorAvantGauche_pwm, LOW);
  digitalWrite(motorAvantGauche_dir, LOW);
  digitalWrite(motorAvantDroite_pwm, LOW);
  digitalWrite(motorAvantDroite_dir, LOW);
  Serial.print(" : stop");

  // Servo guidon
  Serial.println("Servo guidon");
  servotete.attach(6);	
  // Guidon droit devant
  servotete.write(postete);

  // Sonar
  //Serial.println("Sonar");
  //pinMode(TRIGGER_PIN, OUTPUT);
  //digitalWrite(TRIGGER_PIN, LOW);
  //pinMode(ECHO_PIN, INPUT);

  // manette
  int error;
  do {
	ledphare = !ledphare;
	ledphare ? analogWrite(ledphareBleuPin, 255) : analogWrite(ledphareBleuPin, 0);
	error = ps2x.config_gamepad(13,11,10,12, true, true);   //setup pins and settings:  GamePad(clock, command, attention, data, Pressures?, Rumble?) check for error
	if (error == 1)
	  Serial.println("No controller found, check wiring, see readme.txt to enable debug. visit www.billporter.info for troubleshooting tips");
	else if (error == 2)
	  Serial.println("Controller found but not accepting commands. see readme.txt to enable debug. Visit www.billporter.info for troubleshooting tips");
	else if (error == 3)
	  Serial.println("Controller refusing to enter Pressures mode, may not support it. ");
 } while ( error!=0 );

  ledphare = false;
  Serial.println("Found ps2 controller");

  Serial.println("== done");

}

void gestionCommandes() {
       
	if (ps2x.NewButtonState())   {
		// phare on/off
		if (ps2x.Button(PSB_PINK)  ) {
			// CARRE/VIOLET
			ledphare = !ledphare;
		}
		// Phare bleu
		if (ps2x.Button(PSB_BLUE)) {
			// CROIX/BLEU
			ledphareBleu = !ledphareBleu;
		}
		// Phare vert
		if(ps2x.Button(PSB_GREEN)) {
		  // TRIANGLE/VERT
		  ledphareVert = !ledphareVert;
		}
		// Phare rouge
		if(ps2x.Button(PSB_RED)) {
			// CERCLE/ROUGE
			ledphareRouge = !ledphareRouge;
		}
		if(ps2x.Button(PSB_SELECT)) {
			// SELECT
			modeAuto = !modeAuto;
		}
	}
	
}

void gestionVitesse() {
   
	ps2x.read_gamepad(false, vibrate);

	// Vitesse

	if (ps2x.Button(PSB_L1)) { 
		if ( rapport>rapportmax )
			rapport = rapportmax;
		else if ( rapport>1 )
			rapport--;
	}
	
	if (ps2x.Button(PSB_L2) ) {
		if ( rapport<rapportmax )
			rapport++;
		else
			rapport = full/stepRun;
	}

	// Avant/arriere/break
  
	int sdir = ps2x.Analog(PSS_LX);
	int ssens = ps2x.Analog(PSS_LY);
	int posens;
  
	if ( ssens<127 ) {
		posens = mavant;
	} else if ( ssens>127 ) {
		posens = marriere;
	} else {
		posens = mstop;
	}

	// Direction

	if ( modeAuto ) {
		sdir = ps2x.Analog(PSS_RX);
		//if (sdir<128)
		//  sdir+=128;
		//else if (sdir>128)
		//  sdir-=128;
	}  
  
	if ( sdir<128 ) {
      switch (posens) {
        case mavant:
			// tourne gauche en avant
			// compatible avec toutes les roues...
			motorArriereDroite_state=mstop;
			motorArriereGauche_state=mavant;
			motorAvantDroite_state=mavant;
			motorAvantGauche_state=mavant;
			break;
        case marriere:
			// tourne a gauche en arriere
			motorArriereDroite_state=mstop;
			motorArriereGauche_state=marriere;
			motorAvantDroite_state=mstop;
			motorAvantGauche_state=marriere;
			break;
        case mstop:
        default:
          // tourne a gauche sur lui-même
          motorArriereDroite_state=marriere;
          motorArriereGauche_state=mavant;
          motorAvantDroite_state=marriere;
          motorAvantGauche_state=mavant;
      }
	} else if ( sdir>128 )  {
		switch (posens) {
			case mavant:
				// tourne droite avant
				motorArriereDroite_state=mavant;
				motorArriereGauche_state=mstop;
				motorAvantDroite_state=mavant;
				motorAvantGauche_state=mavant;        
				break;
			case marriere:        
				// tourne droite arriere
				motorArriereDroite_state=marriere;
				motorArriereGauche_state=mstop;
				motorAvantDroite_state=marriere;
				motorAvantGauche_state=mstop;
				break;       
			case mstop:
			default:
				// tourne a droite sur lui-même
				motorArriereDroite_state=mavant;
				motorArriereGauche_state=marriere;
				motorAvantDroite_state=mavant;
				motorAvantGauche_state=marriere;
      }
	} else {
		switch ( posens ) {
			case mavant:
			// en avant
			motorArriereDroite_state=mavant;
			motorArriereGauche_state=mavant;
			motorAvantDroite_state=mavant;
			motorAvantGauche_state=mavant;             
			break;
		case marriere:
			// en arriere
			motorArriereDroite_state=marriere;
			motorArriereGauche_state=marriere;
			motorAvantDroite_state=marriere;
			motorAvantGauche_state=marriere;
			break;
		case mstop:
		default:
			// stop
			motorArriereDroite_state=mstop;
			motorArriereGauche_state=mstop;
			motorAvantDroite_state=mstop;
			motorAvantGauche_state=mstop;
		}
	}

	// Pre-commandes

	if (ps2x.Button(PSB_PAD_RIGHT)) {
		motorArriereDroite_state=mavant;
		motorArriereGauche_state=marriere;
		motorAvantDroite_state=marriere;
		motorAvantGauche_state=mavant;
	}
	if (ps2x.Button(PSB_PAD_LEFT)) {
		motorArriereDroite_state=marriere;
		motorArriereGauche_state=mavant;
		motorAvantDroite_state=mavant;
		motorAvantGauche_state=marriere;
	}
	if (ps2x.Button(PSB_PAD_UP)) {
		motorArriereDroite_state=mavant;
		motorArriereGauche_state=mstop;
		motorAvantDroite_state=mstop;
		motorAvantGauche_state=mavant;
	}    
	if (ps2x.Button(PSB_PAD_DOWN)) {
		motorArriereDroite_state=mstop;
		motorArriereGauche_state=mavant;
		motorAvantDroite_state=mavant;
		motorAvantGauche_state=mstop;
	}
	
	speedRun = rapport * stepRun;
	
	// Slow
	if ( ps2x.Button(PSB_R1) )
		speedRun = slow;
		
	// Full speed
	if ( ps2x.Button(PSB_R2) )
		speedRun = full;

}

// Guidon/Tete
void gestionGuidon() {
  
	int sguidon = ps2x.Analog(PSS_RX);
	vibrate = 255;

	if ( sguidon<128 ) {
		postete += stepguidon;
	} else if ( sguidon>128 ) {
		postete -= stepguidon;
	} else {
		vibrate = 0;
		// Mode auto
		if ( modeAuto ) {
			postete = 90;      
		} else {
		/*
			  postete = ps2x.Analog(PSS_LX);
			  postete-=38;
			  if ( postete<90 )
				postete = maxguidon;
			  else if ( postete>90 )
				postete = minguidon;
		*/
		}
	}

	if ( postete<minguidon )
		postete=minguidon;
	else if ( postete>maxguidon )
		postete=maxguidon;
 
	servotete.write(postete);

}

void gestionPropulsion() {

	// Moteurs on/off
	switch (motorArriereGauche_state) {
		case mstop:      
			digitalWrite(motorArriereGauche_pwm, LOW);
			break;
		case mavant:
			digitalWrite(motorArriereGauche_dir, LOW);
			digitalWrite(motorArriereGauche_pwm, HIGH);
			break;
		case marriere:
			digitalWrite(motorArriereGauche_dir, HIGH);
			digitalWrite(motorArriereGauche_pwm, HIGH);
			break;
	}

	switch (motorArriereDroite_state) {
	  case mstop:      
		digitalWrite(motorArriereDroite_pwm, LOW);
		break;
	  case mavant:
		digitalWrite(motorArriereDroite_dir, LOW);
		digitalWrite(motorArriereDroite_pwm, HIGH);
		break;
	  case marriere:
		digitalWrite(motorArriereDroite_dir, HIGH);
		digitalWrite(motorArriereDroite_pwm, HIGH);
		break;
	}    

	switch (motorAvantGauche_state) {
		case mstop:      
			digitalWrite(motorAvantGauche_pwm, LOW);
			break;
		case mavant:
			digitalWrite(motorAvantGauche_dir, LOW);
			digitalWrite(motorAvantGauche_pwm, HIGH);
			break;
		case marriere:
			digitalWrite(motorAvantGauche_dir, HIGH);
			digitalWrite(motorAvantGauche_pwm, HIGH);
			break;
	}

	switch (motorAvantDroite_state) {
	  case mstop:      
		digitalWrite(motorAvantDroite_pwm, LOW);
		break;
	  case mavant:
		digitalWrite(motorAvantDroite_dir, LOW);
		digitalWrite(motorAvantDroite_pwm, HIGH);
		break;
	  case marriere:
		digitalWrite(motorAvantDroite_dir, HIGH);
		digitalWrite(motorAvantDroite_pwm, HIGH);
		break;
	}
 
	// Cycle pwm
 	
 	if ( speedRun!=full ) {
		delay(speedRun);
		digitalWrite(motorArriereGauche_pwm, LOW);
		digitalWrite(motorArriereDroite_pwm, LOW);
		digitalWrite(motorAvantGauche_pwm, LOW);
		digitalWrite(motorAvantDroite_pwm, LOW);
	}

}

void gestionEclairage() {
	    		
	if ( ledphare && ledphareBleu )
		analogWrite(ledphareBleuPin, 255);
	else
		analogWrite(ledphareBleuPin, 0);
	
	if ( ledphare && ledphareRouge )
		analogWrite(ledphareRougePin, 255);
	else
		analogWrite(ledphareRougePin, 0);
	
	if ( ledphare && ledphareVert )
		analogWrite(ledphareVertPin, 255);
	else
		analogWrite(ledphareVertPin, 0);
}

void loop() {
	
   // memo calibrage manette sticks analogiques
   // left X point 0 a 128
   // left Y point 0 a 127
   
	gestionCommandes();
	gestionEclairage();
	gestionVitesse();
	gestionGuidon();
	gestionPropulsion();  
  
	// Wait for manette
	delay(50);
 
}
