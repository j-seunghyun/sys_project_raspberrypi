#include <stdio.h>
#include <wiringPi.h>
#include <softPwm.h>

const int pinServoUD = 17;
const int pinServoLR = 18;

int main(void)
{
	wiringPiSetupGpio();

	softPwmCreate(pinServoUD, 0, 200);
	softPwmCreate(pinServoLR, 0, 200);
	
	while(1)
	{
		softPwmWrite(pinServoUD, 1);
		delay(500);
		softPwmWrite(pinServoUD, 10);
		delay(500);
		softPwmWrite(pinServoUD, 20);
		delay(500);
		softPwmWrite(pinServoUD, 10);
		delay(500);

		system("raspistill -t 10000 -o video.mp4");

		softPwmWrite(pinServoLR, 5);
		delay(500);
		softPwmWrite(pinServoLR, 15);
		delay(500);
		softPwmWrite(pinServoLR, 25);
		delay(500);
		softPwmWrite(pinServoLR, 15);
		delay(500);

		system("raspistill -t 10000 -o video.mp4");
		}
	return 0;
}