#include <QApplication>

#include <nqlogger.h>
#include "LStepExpressModel.h"

LStepExpressModel::LStepExpressModel(const char* port,
                                     int updateInterval,
                                     int motionUpdateInterval,
                                     QObject * /*parent*/)
    : QObject(),
      AbstractDeviceModel<LStepExpress_t>(),
      LStepExpress_PORT(port),
      updateInterval_(updateInterval),
      motionUpdateInterval_(motionUpdateInterval),
      updateCount_(0)
{
    std::vector<int> allZerosI{ 0, 0, 0, 0 };
    std::vector<double> allZerosD{ 0.0, 0.0, 0.0, 0.0 };

    axis_ = allZerosI;
    axisDirection_ = allZerosI;
    dim_ = allZerosI;
    pa_ = allZerosI;
    joystickEnabled_ = -1;
    joystickAxisEnabled_ = std::vector<int>{ -1, -1, -1, -1 };

    axisStatus_ = std::vector<int>{ -1, -1, -1, -1 };
    position_ = allZerosD;

    inMotion_ = false;
    isUpdating_ = false;
    isPaused_ = false;
    finishedCalibrating_ = false;
    
    timer_ = new QTimer(this);
    timer_->setInterval(motionUpdateInterval_);
    connect(timer_, SIGNAL(timeout()), this, SLOT(updateMotionInformation()));
}

void LStepExpressModel::pauseUpdate()
{
    isPaused_ = true;
    setControlsEnabled(false);
}

void LStepExpressModel::continueUpdate()
{
    isPaused_ = false;
    setControlsEnabled(true);
}

QString LStepExpressModel::getAxisName(unsigned int axis)
{
    QMutexLocker locker(&mutex_);

    QString temp(controller_->GetAxisName((VLStepExpress::Axis)axis));
    return temp;
}

QString LStepExpressModel::getAxisDimensionShortName(unsigned int axis)
{
    QMutexLocker locker(&mutex_);


    QString temp(controller_->GetAxisDimensionShortName((VLStepExpress::Dimension)dim_[axis]));
    return temp;
}

QString LStepExpressModel::getAxisStatusText(unsigned int axis)
{
    QMutexLocker locker(&mutex_);

    QString temp(controller_->GetAxisStatusText((VLStepExpress::AxisStatus)axisStatus_[axis]));
    return temp;
}

bool LStepExpressModel::getAxisState(unsigned int axis)
{
    QMutexLocker locker(&mutex_);

    return (axisStatus_[axis]!=VLStepExpress::AXISDISABLED);
}

bool LStepExpressModel::getAxisEnabled(unsigned int axis)
{
    QMutexLocker locker(&mutex_);

    return axis_[axis];
}

double LStepExpressModel::getPosition(unsigned int axis)
{
    QMutexLocker locker(&mutex_);

    return position_[axis];
}

void LStepExpressModel::moveRelative(std::vector<double> & values)
{
    controller_->MoveRelative(values);
    inMotion_ = true;
    emit motionStarted();
}

void LStepExpressModel::moveRelative(double x, double y, double z, double a)
{
    controller_->MoveRelative(x, y, z, a);
    inMotion_ = true;
    emit motionStarted();
}

void LStepExpressModel::moveRelative(unsigned int axis, double value)
{
    controller_->MoveRelative((VLStepExpress::Axis)axis, value);
    inMotion_ = true;
    emit motionStarted();
}

void LStepExpressModel::moveAbsolute(std::vector<double> & values)
{
    NQLog("LStepExpresModel", NQLog::Debug) << "moveAbsolute v1";
    controller_->MoveAbsolute(values);
    inMotion_ = true;
    emit motionStarted();
}

void LStepExpressModel::moveAbsolute(double x, double y, double z, double a)
{
    NQLog("LStepExpresModel", NQLog::Debug) << "moveAbsolute v2, go to " << x << " x, "<< y <<" y, "<< z <<" z, "<< a <<" a";
    controller_->MoveAbsolute(x, y, z, a);
    inMotion_ = true;
    emit motionStarted();
}

void LStepExpressModel::moveAbsolute(unsigned int axis, double value)
{
    controller_->MoveAbsolute((VLStepExpress::Axis)axis, value);
    inMotion_ = true;
    emit motionStarted();
}

void LStepExpressModel::calibrate()
{
    NQLog("LStepExpressModel", NQLog::Debug) << "calibrate: go to calibrate";                                                                 
    controller_->Calibrate();
    inMotion_ = true;
    finishedCalibrating_ = true;
    NQLog("LStepExpressModel", NQLog::Debug) << "calibrate: emit motionStarted";                                                                 
    emit motionStarted();

    /*
    NQLog("LStepExpressModel", NQLog::Debug) << "calibrate: set position to zero";                                                                 
    //set positions to all zeros
    bool changed = false;
    std::vector<double> dvalues{0.0, 0.0, 0.0, 0.0};
    controller_->SetPosition(dvalues);
    if (dvalues!=position_) {
        position_ = dvalues;
        changed = true;
    }
    if (changed) {
        // NQLog("LStepExpressModel", NQLog::Spam) << "information changed";                                                                 
        emit informationChanged();
    }
    */
}

void LStepExpressModel::emergencyStop()
{
    controller_->EmergencyStop();
    inMotion_ = false;
    emit motionFinished();
}

bool LStepExpressModel::getJoystickEnabled()
{
    QMutexLocker locker(&mutex_);

    return (joystickEnabled_==1);
}

bool LStepExpressModel::getJoystickAxisEnabled(unsigned int axis)
{
    QMutexLocker locker(&mutex_);

    return (joystickAxisEnabled_[axis]==1);
}

void LStepExpressModel::setAxisEnabled(unsigned int axis, bool enabled)
{
    int temp = (int)enabled;

    NQLog("LStepExpressModel", NQLog::Debug) << "setAxisEnabled " << axis << " " << temp;

    if (axis_[axis]!=temp) {
        controller_->SetPowerAmplifierStatus((VLStepExpress::Axis)axis, temp);
        controller_->SetAxisEnabled((VLStepExpress::Axis)axis, temp);
        axis_[axis] = temp;
        emit deviceStateChanged(state_);
    }
}

void LStepExpressModel::setJoystickEnabled(bool enabled)
{
    int temp = (int)enabled;

    if (joystickEnabled_!=temp) {
        controller_->SetJoystickEnabled(enabled);
        joystickEnabled_ = temp;
        emit deviceStateChanged(state_);
    }
}

void LStepExpressModel::setJoystickAxisEnabled(unsigned int axis, bool enabled)
{
    int temp = (enabled == true)?1:0;

    if (joystickEnabled_ && joystickAxisEnabled_[axis]!=temp) {
        joystickAxisEnabled_[axis] = temp;
        NQLog("LStepExpressModel", NQLog::Debug) << "setJoystickAxisEnabled " << axis << " " << temp;
        controller_->SetJoystickAxisEnabled((VLStepExpress::Axis)axis, temp);   
    }
}

void LStepExpressModel::setValue(const QString & command, const QString & value)
{
    std::string temp;
    controller_->SetValue(command.toStdString(), value.toStdString());
}

void LStepExpressModel::getValue(const QString & command, QString & value)
{
    std::string temp;
    controller_->GetValue(command.toStdString(), temp);
    value = temp.c_str();
}

void LStepExpressModel::validConfig()
{
    controller_->ValidConfig();
}

void LStepExpressModel::validParameter()
{
    controller_->ValidParameter();
}

void LStepExpressModel::saveConfig()
{
    NQLog("LStepExpressModel", NQLog::Debug) << "save config";
    // controller_->SaveConfig();
}

void LStepExpressModel::reset()
{
    NQLog("LStepExpressModel", NQLog::Debug) << "reset to start-up conditions";
    controller_->Reset();
}

void LStepExpressModel::initialize()
{
    setDeviceState(INITIALIZING);

    renewController(LStepExpress_PORT);

    bool enabled = (controller_ != NULL) && (controller_->DeviceAvailable());

    if ( enabled ) {
        setDeviceState(READY);
        updateInformation();
        updateMotionInformation();
    }
    else {
        setDeviceState( OFF );
        delete controller_;
        controller_ = NULL;
    }
}

void LStepExpressModel::setDeviceState( State state )
{
    if ( state_ != state ) {
        state_ = state;

        if ( state_ == READY ) {
            timer_->start();
        } else {
            timer_->stop();
        }

        emit deviceStateChanged(state);
    }
}

void LStepExpressModel::updateInformation()
{

    /*
        NQLog("LStepExpressModel", NQLog::Debug) << "updateInformation()";

        if (thread()==QApplication::instance()->thread()) {
            NQLog("LStepExpressModel", NQLog::Debug) << " running in main application thread";
        } else {
            NQLog("LStepExpressModel", NQLog::Debug) << " running in dedicated thread";
        }
        */

    bool changed = false;

    std::vector<int> ivalues;

    controller_->GetAxisEnabled(ivalues);
    if (ivalues!=axis_) {
        axis_ = ivalues;
        changed = true;
    }

    controller_->GetDimension(ivalues);
    if (ivalues!=dim_) {
        dim_ = ivalues;
        changed = true;
    }

    int joystick = controller_->GetJoystickEnabled();
    if (joystick!=joystickEnabled_) {
        joystickEnabled_ = joystick;
        changed = true;
    }

    controller_->GetJoystickAxisEnabled(ivalues);
    if (ivalues!=joystickAxisEnabled_) {
        joystickAxisEnabled_ = ivalues;
        changed = true;
    }

    if (changed) {
        // NQLog("LStepExpressModel", NQLog::Spam) << "information changed";
        emit informationChanged();
    }
}

void LStepExpressModel::updateMotionInformation()
{
  NQLog("LStepExpressModel", NQLog::Debug) << "updateMotionInformation()";

    static const int nUpdates = updateInterval_/motionUpdateInterval_;

    if ( state_ == READY && !isPaused_) {

        QMutexLocker locker(&mutex_);

        isUpdating_ = true;

        updateCount_++;
        if (updateCount_==nUpdates) {
            updateInformation();
            updateCount_ = 0;
        }

        /*


        if (thread()==QApplication::instance()->thread()) {
            NQLog("LStepExpressModel", NQLog::Debug) << " running in main application thread";
        } else {
            NQLog("LStepExpressModel", NQLog::Debug) << " running in dedicated thread";
        }
        */

        bool changed = false;

        std::vector<int> ivalues;
        std::vector<double> dvalues;

        controller_->GetAxisStatus(ivalues);
        NQLog("LStepExpressModel", NQLog::Debug) << "updateMotionInformation() received axis status";
        NQLog("LStepExpressModel", NQLog::Debug) << " x-axis: new status = "<<(ivalues)[0]<<" old status = "<<(axisStatus_)[0];
        NQLog("LStepExpressModel", NQLog::Debug) << " y-axis: new status = "<<(ivalues)[1]<<" old status = "<<(axisStatus_)[1];
        NQLog("LStepExpressModel", NQLog::Debug) << " z-axis: new status = "<<(ivalues)[2]<<" old status = "<<(axisStatus_)[2];
        NQLog("LStepExpressModel", NQLog::Debug) << " a-axis: new status = "<<(ivalues)[3]<<" old status = "<<(axisStatus_)[3];
        if (ivalues!=axisStatus_) {
            axisStatus_ = ivalues;
            changed = true;
        }
        if (inMotion_) {
	NQLog("LStepExpressModel", NQLog::Debug) << "updateMotionInformation() inMotion_ = true";
	
	if (std::all_of(ivalues.begin(), ivalues.end(),
                            [](int i){	  NQLog("LStepExpressModel", NQLog::Debug) << "updateMotionInformation() axis status = "<<i; return (i==LStepExpress_t::AXISSTANDSANDREADY || i==LStepExpress_t::AXISACKAFTERCALIBRATION);})) {
	    NQLog("LStepExpressModel", NQLog::Debug) << "updateMotionInformation() finished moving inMotion_ = false";
                inMotion_ = false;
                emit motionFinished();
            }
        }

        controller_->GetPosition(dvalues);
        if (dvalues!=position_) {
	NQLog("LStepExpressModel", NQLog::Debug) << "updateMotionInformation() new position values";
            position_ = dvalues;
            changed = true;
        }

        NQLog("LStepExpressModel", NQLog::Debug) << "updateMotionInformation() finishedCalibrating_ = "<<finishedCalibrating_<<" in motion = "<<inMotion_;
        if(!inMotion_ && finishedCalibrating_){
	NQLog("LStepExpressModel", NQLog::Debug) << "updateMotionInformation() after calibration, setting position values to zero";
	std::vector<double> posvalues{0.0, 0.0, 0.0, 0.0};
	controller_->SetPosition(posvalues);
	position_ = posvalues;
	changed = true;
	finishedCalibrating_ = false;
        }
        
        if (changed) {
	NQLog("LStepExpressModel", NQLog::Debug) << "updateMotionInformation() motion information changed";
            emit motionInformationChanged();
        }

        isUpdating_ = false;
    }
}

void LStepExpressModel::setDeviceEnabled(bool enabled)
{
    NQLog("LStepExpressModel", NQLog::Debug) << "setDeviceEnabled(bool enabled), enabled = "<<enabled;

    if (state_ == READY && !enabled) {
        NQLog("LStepExpressModel", NQLog::Debug) << "setDeviceEnabled(bool enabled), turn off power amplifiers and axes";
        std::vector<int> allZeros{ 0, 0, 0, 0 };
        //std::vector<int> allOn{ 1, 1, 1, 1 };
        controller_->SetPowerAmplifierStatus(allZeros);
        controller_->SetAxisEnabled(allZeros);
    }

    AbstractDeviceModel<LStepExpress_t>::setDeviceEnabled(enabled);
}

void LStepExpressModel::setControlsEnabled(bool enabled)
{
    NQLog("LStepExpressModel", NQLog::Debug) << "setControlsEnabled(bool enabled), enabled = "<<enabled;
    emit controlStateChanged(enabled);
}
