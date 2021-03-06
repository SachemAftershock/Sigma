#include "Shooter.h"
#include "Utilities.h"

Shooter::Shooter()
{
	topWheel = new Victor(TOP_SHOOTER_WHEEL);
	bottomWheel = new Victor(BOTTOM_SHOOTER_WHEEL);
	turretMotor = new Victor(TURRET_MOTOR);

	shooterNotifier = new Notifier(Shooter::DriveShooter, this);

	turretPID = new AfterPID(6.5, .075, 0);
	topWheelPID = new AfterPID(1.125, .01, 0);
	bottomWheelPID = new AfterPID(1.125, .01, 0);
	//trackingPID = new AfterPID(.11, .0008, .00045);
	//trackingPID = new AfterPID(.01350, 0.001350, .1450);
	trackingPID = new AfterPID(0.001f, 0.0f, 0.0f);
	
	shooterAngle = new AnalogChannel(TOWER_POT);
	shooterAngle->SetOversampleBits(2);
	
	shooterTracker = new VisionTracking();
	
	bottomShooterTach = new AnalogChannel(4);
	topShooterTach = new AnalogChannel(7);
	
	dash = new DashboardConnecter();
}
void Shooter::StartPID()
{
	shooterNotifier->StartPeriodic(0.05);
}

void Shooter::ResetPID()
{
	topWheelPID->ResetPID();
	bottomWheelPID->ResetPID();
}

void Shooter::DriveShooter(void *nshooter)
{
	DriverStation *ds = DriverStation::GetInstance();
	Shooter *shooter = (Shooter *)nshooter;
	
	float turretPotFeedback = 0.0f;
	static float turretCamFeedback = 0.0f;
	static float turretSetpoint = 0.0f;
	float turretOutput = 0.0f;

	float topWheelFeedback = shooter->topShooterTach->GetAverageVoltage() / -1.8;
	float topWheelOutput = 0.0f;

	float bottomWheelFeedback = shooter->bottomShooterTach->GetAverageVoltage() / 2.05;
	float bottomWheelOutput = 0.0f;
	
	static float wheelSpeedSetpoint = ds->GetEnhancedIO().GetAnalogInRatio(3);

	turretPotFeedback = (((float)(shooter->shooterAngle->GetAverageValue() - TURRET_POT_ADC_MIN))/TURRET_POT_ADC_MAX);
	//printf("Pot Feedback: %f\n", turretPotFeedback);
	ShooterInformation shooterInfo = shooter->shooterTracker->GetShooterInformation();

	//Vision or Manual Mode
	if(ds->GetEnhancedIO().GetDigital(DS_SHOOTER_MODE))
	{
		float turretPotDS = 1.0f - (ds->GetEnhancedIO().GetAnalogInRatio(DS_TOWER_POT));
		//printf("Turret Pot Normal: %f Turret Pot Flipped: %f\n", turretPotDS, 1.0f - turretPotDS);
		turretSetpoint = (turretPotDS * (TURRET_MAX - TURRET_MIN)) + TURRET_MIN;
		wheelSpeedSetpoint = (ds->GetEnhancedIO().GetAnalogIn(3) / 3.3);
		
		//Run PID Calculation
		if(turretPotFeedback < 0) // Pot unplugged
		{
			printf("Pot is offscale low, check connection!\n");
			turretOutput = 0;
		}
		else
		{
			turretOutput = shooter->turretPID->GetOutput(turretPotFeedback, turretSetpoint, TURRET_DEADBAND) * -1;
			//printf("Manual turretSetpoint: %f\n", turretPotFeedback);
		}
		//shooter->trackingPID->ResetPID();
	}

	else if(!ds->GetEnhancedIO().GetDigital(DS_SHOOTER_MODE)){
		//turretSetpoint = turretPotFeedback;
		if(shooter->shooterTracker->IsFreshData())
		{
			printf("potFeedback: %f/n turretSetpoint: %f/n deadband: %f/n", turretPotFeedback,turretSetpoint, TURRET_DEADBAND);
			//turretSetpoint -= shooter->trackingPID->GetOutput(turretCamFeedback, 0.33, .01);
			//turretOutput = shooter->turretPID->GetOutput(turretPotFeedback, turretSetpoint, TURRET_DEADBAND) * -1;
			
			if(turretOutput <= 0.1f)
			{
				//turretSetpoint = (shooter->shooterTracker->GetShooterInformation().particleOffset - .33) * -1;
				//turretCamFeedback = shooterInfo.particleOffset * -1;
				turretSetpoint = turretPotFeedback - ((((TURRET_MAX - TURRET_MIN) * 47)/270) * (shooterInfo.particleOffset));
				turretOutput = shooter->turretPID->GetOutput(turretPotFeedback, turretSetpoint, TURRET_DEADBAND) * -1;
				//printf("New setpoint: %f\n", turretSetpoint);
				//printf("Particle Offset: %f\n", shooterInfo.particleOffset);
				//printf("Turret Pot Feedback: %f Turret Setpoint: %f\n", turretPotFeedback, turretSetpoint);
				
			}
			wheelSpeedSetpoint = 3.81*pow(shooterInfo.particleArea,-0.217);
			//printf("Vision turretSetpoint: %f\n", turretSetpoint);
		}
		if(shooterInfo.numParticles == 0)
		{
			turretOutput = 0;
			wheelSpeedSetpoint = 0;
			//turretCamFeedback = .33;
		}
		//turretOutput = shooter->turretPID->GetOutput(turretPotFeedback, turretSetpoint, TURRET_DEADBAND) * -1;
		//turretOutput = shooter->trackingPID->GetOutput(turretCamFeedback, turretSetpoint, .01);
		//printf("Turret Output: %f, Turret Feedback: %f\n", turretOutput, turretCamFeedback);
		//printf("Turret Output: %f\n", turretOutput);
	}
	
	if((ds->GetEnhancedIO().GetAnalogIn(3) / 3.3) < .02)
	{
		wheelSpeedSetpoint = 0.0f;
		shooter->ResetPID();
	}
	
	//printf("Top Wheel Feedback: %f Bottom Wheel Feedback: %f\n", topWheelFeedback, bottomWheelFeedback);
	topWheelOutput = shooter->topWheelPID->GetOutput(topWheelFeedback, wheelSpeedSetpoint, .01, false);
	bottomWheelOutput = shooter->bottomWheelPID->GetOutput(bottomWheelFeedback, wheelSpeedSetpoint, .01, false) * -1.0f;
	//printf("Bottom Tach: %f Top Tach: %f Speed: %f\n", bottomWheelFeedback, topWheelFeedback, wheelSpeedSetpoint);
	//printf("Bottom Wheel Output: %f, Top Wheel Output: %f\n", bottomWheelOutput, topWheelOutput);
	//printf("Wheel Speed Setpoint: %f Top Speed Output: %f Botom Speed Output %f\n", wheelSpeedSetpoint, topWheelOutput, bottomWheelOutput);
	// Limit tower rotation based on potentiometer
	if(turretPotFeedback <= TURRET_MIN && turretOutput > 0)
	{
		turretOutput = 0;
	}
	else if(turretPotFeedback >= TURRET_MAX && turretOutput < 0)
	{
		turretOutput = 0;
	}
	
	//printf("Wheel Setpoint: %f\n", topWheelOutput);
	//printf("Turret Output: %f\n", turretOutput);
	//turretOutput = 0;
	shooter->turretMotor->Set(turretOutput);
	
	//topWheelOutput = ds->GetEnhancedIO().GetAnalogInRatio(SHOOTER_POT);
	//bottomWheelOutput = ds->GetEnhancedIO().GetAnalogInRatio(SHOOTER_POT) * -1;
	
	//printf("Shooter Output: %f\n", topWheelOutput);
	
	//printf("Pot val: %f\n", turretPotFeedback);
	
	shooter->topWheel->Set(topWheelOutput);
	shooter->bottomWheel->Set(bottomWheelOutput);
	
	shooter->dash->AddData("topWheel:", shooter->GetTopWheelVoltage());
	shooter->dash->AddData("botWheel:", shooter->GetBottomWheelVoltage());
	shooter->dash->AddData("wheelSpeed:", fabs(wheelSpeedSetpoint));
	shooter->dash->SendData();
}

void Shooter::DriveAutonShooter(PolyurethaneBelt *belt)
{	
	float wheelSpeedSetpoint = .50;
	float AUTON_SPEED_DEADBAND = 0.004875;
	float topWheelFeedback = topShooterTach->GetAverageVoltage() / -1.8;
	float topWheelOutput = 0.0f;

	float bottomWheelFeedback = bottomShooterTach->GetAverageVoltage() / 2.05;
	float bottomWheelOutput = 0.0f;
	
	float turretOutput = 0;
	/*static int ticks = 0;
	
	ticks++;
	if(ticks > 1300)
	{
		wheelSpeedSetpoint = 0.493;		
	}*/
	//printf("Auto Top Wheel Feedback: %f Bottom Wheel Feedback: %f\n", topWheelFeedback, bottomWheelFeedback);
	topWheelOutput = topWheelPID->GetOutput(topWheelFeedback, wheelSpeedSetpoint, AUTON_SPEED_DEADBAND, false);
	bottomWheelOutput = bottomWheelPID->GetOutput(bottomWheelFeedback, wheelSpeedSetpoint, AUTON_SPEED_DEADBAND, false) * -1.0f;
	//printf("Top out: %f Bottom Out: %f\n", topWheelOutput, bottomWheelOutput);
	
	if(fabs(wheelSpeedSetpoint - topWheelFeedback) < AUTON_SPEED_DEADBAND && fabs(wheelSpeedSetpoint - bottomWheelFeedback) < AUTON_SPEED_DEADBAND)
	{
		// Belt on
		belt->DriveAutonBelt(true);
	}
	else
	{
		// Belt off
		belt->DriveAutonBelt(false);
	}

	turretMotor->Set(turretOutput);
	topWheel->Set(topWheelOutput);
	bottomWheel->Set(bottomWheelOutput);
	
	dash->AddData("topWheel:", GetTopWheelVoltage());
	dash->AddData("botWheel:", GetBottomWheelVoltage());
	dash->AddData("wheelSpeed:", fabs(wheelSpeedSetpoint));
	dash->SendData();
}

float Shooter::GetTopWheelVoltage()
{
	return fabs(topShooterTach->GetAverageVoltage() / 1.8);
	//return fabs(topShooterTach->GetAverageVoltage());
}

float Shooter::GetBottomWheelVoltage()
{
 return fabs(bottomShooterTach->GetAverageVoltage() / 2.05);
}

VisionTracking* Shooter::GetShooterTracking()
{
	return shooterTracker;
}
