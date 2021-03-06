#ifndef MECANUMDRIVE_H_
#define MECANUMDRIVE_H_

#include "WPILib.h"
#include "../Misc/AfterPID.h"
#include "../Defines.h"
#include <math.h>

class MecanumDrive {
public: 
	MecanumDrive(void);
	~MecanumDrive(void);

	void Drive();
	void AutonDrive();
	void KinectDrive();
	void StopDrive();
	void SetupDriveTrain();
	
	AfterPID* DriveTrainPID;
	AfterPID* EncoderPID;
	
private:
	
	CANJaguar *leftFrontMotor;
	CANJaguar *rightFrontMotor;
	CANJaguar *leftBackMotor;
	CANJaguar *rightBackMotor;
	KinectStick *leftArm;
	KinectStick *rightArm;
		
	Joystick *driveStick;
	
	float FrontLeftSpeed, FrontRightSpeed, BackLeftSpeed, BackRightSpeed;

	void SetupJaguar(CANJaguar *jag);
	
	void CartesianSpeedCalculation(float x, float y, float rotation );
	void Normalize();
};

#endif
