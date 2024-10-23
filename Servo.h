#include "esphome.h"
#include "ESP32Servo360.h"   
ESP32Servo360 servo;
    
class MyCustomFloatOutput : public Component, public FloatOutput {
  public:	//Servo
	//Servo myservo; 
	const int servoPin = 26;
	const int sensorPin = 34;
	const int servoSpeed = 140; //Speed of servo 0-85

	int value = 0;

	double lastLoc = 0;
	double reqLoc = 0; //Requested motor position 0-1
	double curLoc = 0.5; //Current motor position 0-1

	const unsigned long maxStallTime  = 1000;
	double minLoc = -10;
	double maxLoc = 100;
	bool isLocIncreasing = false;
	bool newData = true;

	void setup() override {
		servo.attach(26, 34);
		pinMode(sensorPin, INPUT); //Sensor
		//ResetMotor();
	}


	void loop() override {
		Rotate(calcLoc(reqLoc));
	}

	void write_state(float state) override {
		servo.attach(26, 34);
		newData = true; 
		reqLoc = state;
	}	


	//--------------------------------Motor----------------------------------//

	//Speed -140 to 140
	void Start(double Speed){
	    servo.spin(Speed);
	}

	void Stop(){
	    servo.spin(0);
		servo.detach();
	}

	//void ResetMotor(){
	//	servo.attach(26, 34);
	//	Rotate(-1000);
	//	servo.attach(26, 34);
	//	Rotate(1000);
	//}

	//Checks if second val between first val +- buffer
	bool LocBetween(double goal, double current){
		double buffer = 0.2;
		return (current < goal+buffer && current > goal-buffer);
	}

	bool LocStop(bool increasing, double goal, double current){
		return (increasing && goal <= current) || (!increasing && goal >= current);
	}

	unsigned long startMillis;
	double lastStop = 0;
	void Rotate(double newPos){
        if (curLoc < minLoc) { minLoc = curLoc; }
        if (curLoc > maxLoc) { maxLoc = curLoc; } 
		

		curLoc = Position();
		Serial.println((String)newPos + " " + (String)curLoc + " " + (String)minLoc + " " + (String)maxLoc + " " + (String)newData);
		if(LocBetween(newPos,curLoc)){
			Stop();
			newData = false; 
			return;
		}

		//set new min/max location if motor stopped longer than maxRotationTime
		if(millis() - startMillis >= maxStallTime && newData){
			if (!LocBetween(lastStop,curLoc)){
				lastStop = curLoc;
			}
			else{
				if (isLocIncreasing) maxLoc = curLoc;
				else minLoc = curLoc;
			}
			startMillis = millis();
		}
		
		isLocIncreasing = (newPos > curLoc); //Checks if the new position is less than or greater
		
		if (!newData) Stop();
		else if (!isLocIncreasing) Start(-servoSpeed); 
		else Start(servoSpeed); 
	}

	double curPos = 0;
	int curRotation = 0;
	double Position(){
		double sum = 0;
		double buffer = 100; //samples 
		double rotationBuffer = 0.45; //how much the gear moved to count as a rotation, intended to count the jumo from 1 to 0 on rotate

		for (int i = 0; i < buffer; i++){
			sum += analogRead(sensorPin) == 0 ? 0 : 1;
		}

		double pos = sum/buffer; //Rotational position 0-1
		if (abs(pos-curPos) > rotationBuffer){ 
			if (pos > curPos) curRotation -=1;
			else curRotation += 1;
			curPos=pos;
		}
		curPos=pos;
		return curRotation + curPos;
	}

	double calcLoc(double newLoc){
		return (newLoc*(maxLoc - minLoc))+minLoc;
	}

	double calcPercent(double newLoc){
		return ((newLoc-minLoc)/(maxLoc - minLoc))*100.0;
	}
};
