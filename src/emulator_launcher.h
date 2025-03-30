#pragma once
#include <string>
#include <filesystem>

class EmulatorLauncher {
public:
    EmulatorLauncher();
    ~EmulatorLauncher();

    // Initialize emulator settings
    bool init(const std::string& emulatorPath);

    // Launch a game ROM with the emulator
    bool launchGame(const std::filesystem::path& romPath);

    // Check if a ROM file exists and is valid
    bool validateRom(const std::filesystem::path& romPath);

    // Get the last error message
    std::string getLastError() const;

private:
    std::string emulatorPath;
    std::string lastError;
    bool initialized;

    // Set error message
    void setError(const std::string& error);
};
