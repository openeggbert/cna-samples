#pragma once
#include <string>

namespace InputReporter {

struct InputReporterResources {
    static constexpr const char* A                  = "Buttons.A:";
    static constexpr const char* B                  = "Buttons.B:";
    static constexpr const char* Back               = "Buttons.Back:";
    static constexpr const char* BothVibrationMotors= "Supports Left and Right Vibration Motors";
    static constexpr const char* ButtonPressed      = "Pressed";
    static constexpr const char* ButtonReleased     = "Released";
    static constexpr const char* DeadZoneInstructions = "Hold START to Change Dead Zone";
    static constexpr const char* Disconnected       = "(Disconnected)";
    static constexpr const char* DPadDown           = "DPad.Down:";
    static constexpr const char* DPadLeft           = "DPad.Left:";
    static constexpr const char* DPadRight          = "DPad.Right:";
    static constexpr const char* DPadUp             = "DPad.Up:";
    static constexpr const char* ExitInstructions   = "Hold BACK to Exit";
    static constexpr const char* LeftShoulder       = "Buttons.LeftShoulder:";
    static constexpr const char* LeftStick          = "Buttons.LeftStick:";
    static constexpr const char* LeftThumbstickX    = "Thumbsticks.Left.X:";
    static constexpr const char* LeftThumbstickY    = "Thumbsticks.Left.Y:";
    static constexpr const char* LeftTrigger        = "Triggers.Left:";
    static constexpr const char* LeftVibrationMotor = "Supports Left Vibration Motor Only";
    static constexpr const char* NoVibration        = "No Vibration Support";
    static constexpr const char* PacketNumber       = "PacketNumber:";
    static constexpr const char* RightShoulder      = "Buttons.RightShoulder:";
    static constexpr const char* RightStick         = "Buttons.RightStick:";
    static constexpr const char* RightThumbstickX   = "Thumbsticks.Right.X:";
    static constexpr const char* RightThumbstickY   = "Thumbsticks.Right.Y:";
    static constexpr const char* RightTrigger       = "Triggers.Right:";
    static constexpr const char* RightVibrationMotor= "Supports Right Vibration Motor Only";
    static constexpr const char* Start              = "Buttons.Start:";
    static constexpr const char* Title              = "Controller ";
    static constexpr const char* X                  = "Buttons.X:";
    static constexpr const char* Y                  = "Buttons.Y:";
};

} // namespace InputReporter
