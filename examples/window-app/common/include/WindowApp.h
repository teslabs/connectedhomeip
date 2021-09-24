/*
 *
 *    Copyright (c) 2021 Project CHIP Authors
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#pragma once

#include <app-common/zap-generated/enums.h>
#include <app/util/af-types.h>
#include <lib/core/CHIPError.h>
#include <app/clusters/window-covering-server/window-covering-server.h>


using namespace chip::app::Clusters::WindowCovering;

class WindowApp
{
public:
    struct Timer
    {
        typedef void (*Callback)(Timer & timer);

        Timer(const char * name, uint32_t timeoutInMs, Callback callback, void * context) :
            mName(name), mTimeoutInMs(timeoutInMs), mCallback(callback), mContext(context)
        {}
        virtual ~Timer()     = default;
        virtual void Start() = 0;
        virtual void Stop()  = 0;
        void Timeout();

        const char * mName    = nullptr;
        uint32_t mTimeoutInMs = 0;
        Callback mCallback    = nullptr;
        void * mContext       = nullptr;
        bool mIsActive        = false;
    };

    struct Button
    {
        enum class Id
        {
            Up   = 0,
            Down = 1
        };

        Button(Id id, const char * name) : mId(id), mName(name) {}
        virtual ~Button() = default;
        void Press();
        void Release();

        Id mId;
        const char * mName = nullptr;
    };

    enum class EventId
    {
        None = 0,
        Reset,
        ResetPressed,
        ResetWarning,
        ResetCanceled,
        // Button events
        UpPressed,
        UpReleased,
        DownPressed,
        DownReleased,
        BtnCycleType,
        BtnCycleActuator,

        // Cover Attribute update events
        Type,
        ConfigStatus,
        OperationalStatus,
        EndProductType,
        Mode,
        SafetyStatus,
        LiftCurrentPosition,
        TiltCurrentPosition,
        LiftTargetPosition,
        TiltTargetPosition,

        // Actuator Update Change
        LiftUpdate,
        TiltUpdate,

        StopMotion,
        // Provisioning events
        ProvisionedStateChanged,
        ConnectivityStateChanged,
        BLEConnectionsChanged,
    };

    enum ButtonCtrlMode
    {
        Tilt = 0,
        Lift,
    };

    struct Event
    {
        Event(EventId id) : mId(id), mEndpoint(0) {}
        Event(EventId id, chip::EndpointId endpoint) : mId(id), mEndpoint(endpoint) {}

        EventId mId;
        chip::EndpointId mEndpoint;
    };

    struct Actuator
    {

        uint16_t mOpenLimit          = 0;//WC_PERCENT100THS_MIN;
        uint16_t mClosedLimit        = 0;//WC_PERCENT100THS_MAX;
        uint16_t mCurrentPosition    = 0;//LimitStatus::IsUpOrOpen;//WC_PERCENT100THS_DEF;
        uint16_t mTargetPosition     = 0;//OperationalState::MovingDownOrClose;//WC_PERCENT100THS_DEF;

        uint16_t mStepDelta          = 1;
        uint16_t mStepMinimum        = 1;



        uint16_t mNumberOfActuations = 0; //Number of commands sent to the actuators

        void SetPosition(uint16_t value);
        void StepTowardUpOrOpen();
        void StepTowardDownOrClose();

        void GoToValue(uint16_t value);
        void GoToPercentage(chip::Percent100ths percent100ths);
        void StopMotion();
        void UpdatePosition();
        void Print();

        void TimerStart();
        void TimerStop();
        bool IsActive();
        LimitStatus GetLimitState();

        static void OnActuatorTimeout(Timer & timer);
        void Init(const char * name, uint32_t timeoutInMs, OperationalState * opState, EventId event);

//struct OperationalStatus ff;
      //  OperationalState * mOpState;


        Timer *          mTimer   = nullptr;
        EventId          mEvent  = EventId::None;
        OperationalState mOpState = OperationalState::Stall;
    };


    struct Cover
    {
        void Init(chip::EndpointId endpoint);
        void Finish();
        void StopMotion();
        EmberAfWcType CycleType();

        chip::EndpointId mEndpoint = 0;

        // Attribute: Id 10 OperationalStatus
        OperationalStatus mOperationalStatus = { .global = OperationalState::Stall,
                                            .lift   = OperationalState::Stall,
                                            .tilt   = OperationalState::Stall };


        Actuator mLift, mTilt;
    };



    static WindowApp & Instance();

    virtual ~WindowApp() = default;
    virtual CHIP_ERROR Init();
    virtual CHIP_ERROR Start() = 0;
    virtual CHIP_ERROR Run();
    virtual void Finish();
    virtual void PostEvent(const Event & event) = 0;

protected:
    struct StateFlags
    {
        bool isThreadProvisioned     = false;
        bool isThreadEnabled         = false;
        bool haveBLEConnections      = false;
        bool haveServiceConnectivity = false;
    };

    Cover & GetCover();
    Cover * GetCover(chip::EndpointId endpoint);

    virtual Button * CreateButton(Button::Id id, const char * name) = 0;
    virtual void DestroyButton(Button * b);
    virtual Timer * CreateTimer(const char * name, uint32_t timeoutInMs, Timer::Callback callback, void * context) = 0;
    virtual void DestroyTimer(Timer * timer);

    virtual void ProcessEvents() = 0;
    virtual void DispatchEvent(const Event & event);
    virtual void OnMainLoop() = 0;
    static void OnLongPressTimeout(Timer & timer);

    Timer * mLongPressTimer = nullptr;
    Button * mButtonUp      = nullptr;
    Button * mButtonDown    = nullptr;
    ButtonCtrlMode mButtonCtrlMode;

    StateFlags mState;

    bool mUpPressed      = false;
    bool mDownPressed    = false;
    bool mUpSuppressed   = false;
    bool mDownSuppressed = false;
    bool mResetWarning   = false;

private:
    void HandleLongPress();

    Cover mCoverList[WINDOW_COVER_COUNT];



    uint8_t mCurrentCover = 0;
};
