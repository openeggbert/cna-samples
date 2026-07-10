#pragma once

// Direct port of Tank.cs. Each player controls a tank, which they can drive around the
// screen. This class implements the logic for moving and drawing the tank, sending and
// receiving network packets, and applying prediction and smoothing to compensate for
// network latency.

#include <Microsoft/Xna/Framework/Content/ContentManager.hpp>
#include <Microsoft/Xna/Framework/GameTime.hpp>
#include <Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp>
#include <Microsoft/Xna/Framework/Graphics/SpriteEffects.hpp>
#include <Microsoft/Xna/Framework/Graphics/Texture2D.hpp>
#include <Microsoft/Xna/Framework/MathHelper.hpp>
#include <Microsoft/Xna/Framework/Net/PacketReader.hpp>
#include <Microsoft/Xna/Framework/Net/PacketWriter.hpp>
#include <Microsoft/Xna/Framework/Vector2.hpp>
#include <Microsoft/Xna/Framework/Color.hpp>
#include <System/TimeSpan.hpp>

#include "RollingAverage.hpp"

#include <algorithm>
#include <cmath>

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;
using namespace Microsoft::Xna::Framework::Content;
using namespace Microsoft::Xna::Framework::Net;

namespace NetworkPrediction {

class Tank {
public:
    Tank(int gamerIndex, ContentManager& content, int screenWidth, int screenHeight) {
        // Use the gamer index to compute a starting position, so each player starts in
        // a different place as opposed to all on top of each other.
        float x = (float)(screenWidth / 4 + (gamerIndex % 5) * screenWidth / 8);
        float y = (float)(screenHeight / 4 + (gamerIndex / 5) * screenHeight / 5);

        simulationState_.Position = Vector2(x, y);
        simulationState_.TankRotation = -MathHelper::PiOver2;
        simulationState_.TurretRotation = -MathHelper::PiOver2;

        // Initialize all three versions of our state to the same values.
        previousState_ = simulationState_;
        displayState_ = simulationState_;

        // Load textures.
        tankTexture_ = content.Load<Texture2D>("Tank");
        turretTexture_ = content.Load<Texture2D>("Turret");

        screenSize_ = Vector2((float)screenWidth, (float)screenHeight);
    }

    // Gets the current position of the tank.
    [[nodiscard]] Vector2 Position() const { return displayState_.Position; }

    // Moves a locally controlled tank in response to the specified inputs.
    void UpdateLocal(Vector2 tankInput, Vector2 turretInput) {
        tankInput_ = tankInput;
        turretInput_ = turretInput;

        // Update the master simulation state.
        UpdateState(simulationState_);

        // Locally controlled tanks have no prediction or smoothing, so we just copy the
        // simulation state directly into the display state.
        displayState_ = simulationState_;
    }

    // Applies prediction and smoothing to a remotely controlled tank.
    void UpdateRemote(int framesBetweenPackets, bool enablePrediction) {
        // Update the smoothing amount, which interpolates from the previous state
        // toward the current simulation state. The speed of this decay depends on the
        // number of frames between packets: we want to finish our smoothing
        // interpolation at the same time the next packet is due.
        float smoothingDecay = 1.0f / (float)framesBetweenPackets;

        currentSmoothing_ -= smoothingDecay;

        if (currentSmoothing_ < 0)
            currentSmoothing_ = 0;

        if (enablePrediction) {
            // Predict how the remote tank will move by updating our local copy of its
            // simulation state.
            UpdateState(simulationState_);

            // If both smoothing and prediction are active, also apply prediction to the
            // previous state.
            if (currentSmoothing_ > 0) {
                UpdateState(previousState_);
            }
        }

        if (currentSmoothing_ > 0) {
            // Interpolate the display state gradually from the previous state to the
            // current simulation state.
            ApplySmoothing();
        } else {
            // Copy the simulation state directly into the display state.
            displayState_ = simulationState_;
        }
    }

    // Writes our local tank state into a network packet.
    void WriteNetworkPacket(PacketWriter& packetWriter, const GameTime& gameTime) {
        // Send our current time.
        packetWriter.Write((float)gameTime.getTotalGameTimeProperty().getTotalSecondsProperty());

        // Send the current state of the tank.
        packetWriter.Write(simulationState_.Position);
        packetWriter.Write(simulationState_.Velocity);
        packetWriter.Write(simulationState_.TankRotation);
        packetWriter.Write(simulationState_.TurretRotation);

        // Also send our current inputs. These can be used to more accurately predict how
        // the tank is likely to move in the future.
        packetWriter.Write(tankInput_);
        packetWriter.Write(turretInput_);
    }

    // Reads the state of a remotely controlled tank from a network packet.
    void ReadNetworkPacket(PacketReader& packetReader, const GameTime& gameTime, System::TimeSpan latency,
                            bool enablePrediction, bool enableSmoothing) {
        if (enableSmoothing) {
            // Start a new smoothing interpolation from our current state toward this
            // new state we just received.
            previousState_ = displayState_;
            currentSmoothing_ = 1;
        } else {
            currentSmoothing_ = 0;
        }

        // Read what time this packet was sent.
        float packetSendTime = packetReader.ReadSingle();

        // Read simulation state from the network packet.
        simulationState_.Position = packetReader.ReadVector2();
        simulationState_.Velocity = packetReader.ReadVector2();
        simulationState_.TankRotation = packetReader.ReadSingle();
        simulationState_.TurretRotation = packetReader.ReadSingle();

        // Read remote inputs from the network packet.
        tankInput_ = packetReader.ReadVector2();
        turretInput_ = packetReader.ReadVector2();

        // Optionally apply prediction to compensate for how long it took this packet
        // to reach us.
        if (enablePrediction) {
            ApplyPrediction(gameTime, latency, packetSendTime);
        }
    }

    // Draws the tank and turret.
    void Draw(SpriteBatch& spriteBatch) {
        Vector2 origin((float)(tankTexture_.getWidthProperty() / 2), (float)(tankTexture_.getHeightProperty() / 2));

        spriteBatch.Draw(tankTexture_, displayState_.Position, std::nullopt, Color(255, 255, 255, 255),
                          displayState_.TankRotation, origin, 1.0f, SpriteEffects::None, 0.0f);

        spriteBatch.Draw(turretTexture_, displayState_.Position, std::nullopt, Color(255, 255, 255, 255),
                          displayState_.TurretRotation, origin, 1.0f, SpriteEffects::None, 0.0f);
    }

private:
    // Constants control how fast the tank moves and turns.
    static constexpr float kTankTurnRate = 0.01f;
    static constexpr float kTurretTurnRate = 0.03f;
    static constexpr float kTankSpeed = 0.3f;
    static constexpr float kTankFriction = 0.9f;

    // To implement smoothing, we need more than one copy of the tank state. We must
    // record both where it used to be, and where it is now, and also a smoothed value
    // somewhere in between these two states which is where we will draw the tank on the
    // screen. To simplify managing these three different versions of the tank state, we
    // move all the state fields into this internal helper structure.
    struct TankState {
        Vector2 Position;
        Vector2 Velocity;
        float TankRotation = 0.0f;
        float TurretRotation = 0.0f;
    };

    // This is the latest master copy of the tank state, used by our local physics
    // computations and prediction. This state will jerk whenever a new network packet
    // is received.
    TankState simulationState_;

    // This is a copy of the state from immediately before the last network packet was
    // received.
    TankState previousState_;

    // This is the tank state that is drawn onto the screen. It is gradually
    // interpolated from previousState_ toward simulationState_, in order to smooth out
    // any sudden jumps caused by discontinuities when a network packet suddenly
    // modifies the simulationState_.
    TankState displayState_;

    // Used to interpolate displayState_ from previousState_ toward simulationState_.
    float currentSmoothing_ = 0.0f;

    // Averaged time difference from the last 100 incoming packets, used to estimate how
    // our local clock compares to the time on the remote machine.
    RollingAverage clockDelta_{100};

    // Input controls can be read from keyboard, gamepad, or the network.
    Vector2 tankInput_;
    Vector2 turretInput_;

    // Textures used to draw the tank.
    Texture2D tankTexture_;
    Texture2D turretTexture_;

    Vector2 screenSize_;

    // Applies smoothing by interpolating the display state somewhere in between the
    // previous state and current simulation state.
    void ApplySmoothing() {
        displayState_.Position = Vector2::Lerp(simulationState_.Position, previousState_.Position, currentSmoothing_);

        displayState_.Velocity = Vector2::Lerp(simulationState_.Velocity, previousState_.Velocity, currentSmoothing_);

        displayState_.TankRotation =
            MathHelper::Lerp(simulationState_.TankRotation, previousState_.TankRotation, currentSmoothing_);

        displayState_.TurretRotation =
            MathHelper::Lerp(simulationState_.TurretRotation, previousState_.TurretRotation, currentSmoothing_);
    }

    // Incoming network packets tell us where the tank was at the time the packet was
    // sent. But packets do not arrive instantly! We want to know where the tank is now,
    // not just where it used to be. This method attempts to guess the current state by
    // figuring out how long the packet took to arrive, then running the appropriate
    // number of local updates to catch up to that time. This allows us to figure out
    // things like "it used to be over there, and it was moving that way while turning
    // to the left, so assuming it carried on using those same inputs, it should now be
    // over here".
    void ApplyPrediction(const GameTime& gameTime, System::TimeSpan latency, float packetSendTime) {
        // Work out the difference between our current local time and the remote time
        // at which this packet was sent.
        float localTime = (float)gameTime.getTotalGameTimeProperty().getTotalSecondsProperty();

        float timeDelta = localTime - packetSendTime;

        // Maintain a rolling average of time deltas from the last 100 packets.
        clockDelta_.AddValue(timeDelta);

        // The caller passed in an estimate of the average network latency, which is
        // provided by the XNA Framework networking layer. But not all packets will
        // take exactly that average amount of time to arrive! To handle varying
        // latencies per packet, we include the send time as part of our packet data.
        // By comparing this with a rolling average of the last 100 send times, we can
        // detect packets that are later or earlier than usual, even without having
        // synchronized clocks between the two machines. We then adjust our average
        // latency estimate by this per-packet deviation.

        float timeDeviation = timeDelta - clockDelta_.AverageValue();

        latency = latency + System::TimeSpan::FromSeconds(timeDeviation);

        System::TimeSpan oneFrame = System::TimeSpan::FromSeconds(1.0 / 60.0);

        // Apply prediction by updating our simulation state however many times is
        // necessary to catch up to the current time.
        while (latency >= oneFrame) {
            UpdateState(simulationState_);

            latency = latency - oneFrame;
        }
    }

    // Updates one of our state structures, using the current inputs to turn the tank,
    // and applying the velocity and inertia calculations. This method is used directly
    // to update locally controlled tanks, and also indirectly to predict the motion of
    // remote tanks.
    void UpdateState(TankState& state) {
        // Gradually turn the tank and turret to face the requested direction.
        state.TankRotation = TurnToFace(state.TankRotation, tankInput_, kTankTurnRate);

        state.TurretRotation = TurnToFace(state.TurretRotation, turretInput_, kTurretTurnRate);

        // How close the desired direction is the tank facing?
        Vector2 tankForward((float)std::cos(state.TankRotation), (float)std::sin(state.TankRotation));

        Vector2 targetForward(tankInput_.X, -tankInput_.Y);

        float facingForward = Vector2::Dot(tankForward, targetForward);

        // If we have finished turning, also start moving forward.
        if (facingForward > 0) {
            float speed = facingForward * facingForward * kTankSpeed;

            state.Velocity = state.Velocity + tankForward * speed;
        }

        // Update the position and velocity.
        state.Position = state.Position + state.Velocity;
        state.Velocity = state.Velocity * kTankFriction;

        // Clamp so the tank cannot drive off the edge of the screen.
        state.Position = Vector2::Clamp(state.Position, Vector2::Zero, screenSize_);
    }

    // Gradually rotates the tank to face the specified direction. See the Aiming sample
    // (creators.xna.com) for details.
    static float TurnToFace(float rotation, Vector2 target, float turnRate) {
        if (target == Vector2::Zero)
            return rotation;

        float angle = (float)std::atan2(-target.Y, target.X);

        float difference = rotation - angle;

        while (difference > MathHelper::Pi)
            difference -= MathHelper::TwoPi;

        while (difference < -MathHelper::Pi)
            difference += MathHelper::TwoPi;

        turnRate *= std::abs(difference);

        if (difference < 0)
            return rotation + std::min(turnRate, -difference);
        else
            return rotation - std::min(turnRate, difference);
    }
};

} // namespace NetworkPrediction
