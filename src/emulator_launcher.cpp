/**
 * @file emulator_launcher.cpp
 * @brief Implements functionality for launching and managing an external game emulator.
 *
 * This file contains the implementation of the EmulatorLauncher class,
 * which provides methods for initializing an emulator, launching games,
 * validating ROM files, and handling errors.
 *
 * @author Shiv
 */

#include "emulator_launcher.h"
#include <stdexcept>
#include <cstdlib>

/**
 * @class EmulatorLauncher
 * @brief Manages launching an external emulator and running game ROMs.
 *
 * The EmulatorLauncher class is responsible for initializing an emulator,
 * validating ROM files, launching games, and managing errors related to the process.
 */

/**
 * @brief Constructs an EmulatorLauncher object with an uninitialized state.
 *
 * The emulator is not initialized until the init() method is called with
 * a valid emulator path.
 */
EmulatorLauncher::EmulatorLauncher() : initialized(false) {}

/**
 * @brief Destructor for EmulatorLauncher.
 *
 * Cleans up resources if necessary. Currently, no special cleanup is required.
 */
EmulatorLauncher::~EmulatorLauncher() {}

/**
 * @brief Initializes the emulator with the specified path.
 *
 * @param path Path to the emulator executable.
 * @return true if initialization is successful, false otherwise.
 */
bool EmulatorLauncher::init(const std::string& path) {
    emulatorPath = path;
    
    if (emulatorPath.empty()) {
        setError("No emulator path specified");
        return false;
    }
    
    initialized = true;
    return true;
}

/**
 * @brief Launches a game ROM using the initialized emulator.
 *
 * @param romPath Path to the ROM file to launch.
 * @return true if the game launches successfully, false otherwise.
 */
bool EmulatorLauncher::launchGame(const std::filesystem::path& romPath) {
    if (!initialized) {
        setError("Emulator not initialized");
        return false;
    }
    
    if (!validateRom(romPath)) {
        return false;
    }
    
    std::string command;
    #ifdef _WIN32
        command = "start \"\" \"" + emulatorPath + "\" \"" + romPath.string() + "\"";
    #else
        command = "\"" + emulatorPath + "\" \"" + romPath.string() + "\" &";
    #endif
    
    int result = std::system(command.c_str());
    if (result != 0) {
        setError("Failed to launch emulator");
        return false;
    }
    
    return true;
}

/**
 * @brief Validates if the ROM file exists and has the correct file extension (.nes).
 *
 * @param romPath Path to the ROM file to validate.
 * @return true if the ROM file is valid, false otherwise.
 */
bool EmulatorLauncher::validateRom(const std::filesystem::path& romPath) {
    if (!std::filesystem::exists(romPath)) {
        setError("ROM file does not exist: " + romPath.string());
        return false;
    }
    
    if (romPath.extension() != ".nes") {
        setError("Invalid ROM file type: " + romPath.string());
        return false;
    }
    
    return true;
}

/**
 * @brief Retrieves the last error message if any operation fails.
 *
 * @return A string containing the last recorded error message.
 */
std::string EmulatorLauncher::getLastError() const {
    return lastError;
}

/**
 * @brief Sets the error message when an operation fails.
 *
 * @param error The error message to store.
 */
void EmulatorLauncher::setError(const std::string& error) {
    lastError = error;
}
