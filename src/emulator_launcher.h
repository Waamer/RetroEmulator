/**
 * @file emulator_launcher.h
 * @brief Declares the EmulatorLauncher class for managing an external game emulator.
 *
 * This header file defines the EmulatorLauncher class, which provides methods for
 * initializing an emulator, launching games, validating ROM files, and handling errors.
 *
 * @author Shiv
 */

#pragma once
#include <string>
#include <filesystem>

/**
 * @class EmulatorLauncher
 * @brief Manages launching an external emulator and running game ROMs.
 *
 * The EmulatorLauncher class is responsible for initializing an emulator,
 * validating ROM files, launching games, and managing errors related to the process.
 */
class EmulatorLauncher {
public:
    /**
     * @brief Constructs an EmulatorLauncher object with an uninitialized state.
     */
    EmulatorLauncher();
    
    /**
     * @brief Destructor for EmulatorLauncher.
     */
    ~EmulatorLauncher();

    /**
     * @brief Initializes the emulator with the specified path.
     *
     * @param emulatorPath Path to the emulator executable.
     * @return true if initialization is successful, false otherwise.
     */
    bool init(const std::string& emulatorPath);

    /**
     * @brief Launches a game ROM using the initialized emulator.
     *
     * @param romPath Path to the ROM file to launch.
     * @return true if the game launches successfully, false otherwise.
     */
    bool launchGame(const std::filesystem::path& romPath);

    /**
     * @brief Validates if the ROM file exists and has the correct file extension.
     *
     * @param romPath Path to the ROM file to validate.
     * @return true if the ROM file is valid, false otherwise.
     */
    bool validateRom(const std::filesystem::path& romPath);

    /**
     * @brief Retrieves the last error message if any operation fails.
     *
     * @return A string containing the last recorded error message.
     */
    std::string getLastError() const;

private:
    std::string emulatorPath;  ///< Path to the emulator executable.
    std::string lastError;     ///< Stores the last error message.
    bool initialized;          ///< Tracks if the emulator has been initialized.

    /**
     * @brief Sets the error message when an operation fails.
     *
     * @param error The error message to store.
     */
    void setError(const std::string& error);
};
