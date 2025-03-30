/**
 * @file emulator_launcher.cpp
 * Implements functionality for launching and managing an external game emulator.
 */

#include "emulator_launcher.h"
#include <stdexcept>
#include <cstdlib>

/**
 * Constructor initializes the emulator launcher with uninitialized state
 */
EmulatorLauncher::EmulatorLauncher() : initialized(false) {}

/**
 * Destructor - no special cleanup needed
 */
EmulatorLauncher::~EmulatorLauncher() {}

/**
 * Initializes the emulator with the specified path
 * @param path Path to the emulator executable
 * @return true if initialization successful, false otherwise
 */
bool EmulatorLauncher::init(const std::string& path) {
    // Check if emulator exists in PATH or use provided path
    emulatorPath = path;
    
    // Basic validation - in a real implementation, we would verify the emulator
    // exists and is executable
    if (emulatorPath.empty()) {
        setError("No emulator path specified");
        return false;
    }
    
    initialized = true;
    return true;
}

/**
 * Launches a game ROM using the initialized emulator
 * @param romPath Path to the ROM file to launch
 * @return true if game launched successfully, false otherwise
 */
bool EmulatorLauncher::launchGame(const std::filesystem::path& romPath) {
    if (!initialized) {
        setError("Emulator not initialized");
        return false;
    }
    
    if (!validateRom(romPath)) {
        return false;
    }
    
    // Construct command to launch emulator with ROM
    std::string command;
    #ifdef _WIN32
        command = "start \"\" \"" + emulatorPath + "\" \"" + romPath.string() + "\"";
    #else
        command = "\"" + emulatorPath + "\" \"" + romPath.string() + "\" &";
    #endif
    
    // Launch the emulator
    int result = std::system(command.c_str());
    if (result != 0) {
        setError("Failed to launch emulator");
        return false;
    }
    
    return true;
}

/**
 * Validates if the ROM file exists and has the correct file extension (.nes)
 * @param romPath Path to the ROM file to validate
 * @return true if ROM is valid, false otherwise
 */
bool EmulatorLauncher::validateRom(const std::filesystem::path& romPath) {
    if (!std::filesystem::exists(romPath)) {
        setError("ROM file does not exist: " + romPath.string());
        return false;
    }
    
    // Check file extension
    if (romPath.extension() != ".nes") {
        setError("Invalid ROM file type: " + romPath.string());
        return false;
    }
    
    return true;
}

/**
 * Retrieves the last error message if any operation failed
 * @return String containing the last error message
 */
std::string EmulatorLauncher::getLastError() const {
    return lastError;
}

/**
 * Sets the error message when an operation fails
 * @param error Error message to store
 */
void EmulatorLauncher::setError(const std::string& error) {
    lastError = error;
}
