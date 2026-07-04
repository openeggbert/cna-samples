#pragma once

// IDebugCommandHost.hpp — C++ port of GameDebugTools/IDebugCommandHost.cs
// (XNA 4.0 PerformanceMeasuring sample).

#include <functional>
#include <string>
#include <vector>

namespace PerformanceMeasuring::GameDebugTools {

class IDebugCommandHost;

// Message types for debug command output.
enum class DebugCommandMessage {
    Standard = 1,
    Error = 2,
    Warning = 3
};

// Debug command execution callback.
using DebugCommandExecute =
    std::function<void(IDebugCommandHost& host, const std::string& command,
                        const std::vector<std::string>& arguments)>;

// Interface for a debug command executioner.
class IDebugCommandExecutioner {
public:
    virtual ~IDebugCommandExecutioner() = default;
    virtual void ExecuteCommand(const std::string& command) = 0;
};

// Interface for a debug command message listener.
class IDebugEchoListener {
public:
    virtual ~IDebugEchoListener() = default;
    virtual void Echo(DebugCommandMessage messageType, const std::string& text) = 0;
};

// Interface for a debug command host.
class IDebugCommandHost : public IDebugEchoListener, public IDebugCommandExecutioner {
public:
    virtual void RegisterCommand(const std::string& command, const std::string& description,
                                  DebugCommandExecute callback) = 0;
    virtual void UnregisterCommand(const std::string& command) = 0;

    virtual void Echo(const std::string& text) = 0;
    virtual void EchoWarning(const std::string& text) = 0;
    virtual void EchoError(const std::string& text) = 0;

    virtual void RegisterEchoListener(IDebugEchoListener* listener) = 0;
    virtual void UnregisterEchoListener(IDebugEchoListener* listener) = 0;

    virtual void PushExecutioner(IDebugCommandExecutioner* executioner) = 0;
    virtual void PopExecutioner() = 0;
};

} // namespace PerformanceMeasuring::GameDebugTools
