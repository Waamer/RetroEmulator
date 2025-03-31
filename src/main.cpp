/**
 * @file main.cpp
 * Main entry point for the retro game emulator launcher application.
 * Handles ROM file scanning, UI initialization, and game launching.
 */

 #include <iostream>
 #include <filesystem>
 #include <vector>
 #include <cstdlib>
 #include "sdl_ui.h"
 #include "emulator_launcher.h"

 
 namespace fs = std::filesystem;
 
 /**
  * Scans the specified directory for .nes ROM files
  * Creates the games directory if it doesn't exist
  * @param gamesDir Path to the directory containing ROM files
  * @return Vector of ROM filenames found in the directory
  */
 std::vector<std::string> scanForRoms(const fs::path& gamesDir) {
     std::vector<std::string> roms;
     
     if (!fs::exists(gamesDir)) {
         fs::create_directory(gamesDir);
         return roms;
     }
 
     for (const auto& entry : fs::directory_iterator(gamesDir)) {
         if (entry.path().extension() == ".nes") {
             roms.push_back(entry.path().filename().string());
         }
     }
     
     return roms;
 }
 
 /**
  * Main program entry point
  * Initializes the UI and emulator, scans for ROMs,
  * and runs the main game selection loop
  */
 int main() {
     // Initialize the SDL-based user interface system
     SDLUI ui;
     if (!ui.init()) {
         std::cerr << "Failed to initialize UI" << std::endl;
         return 1;
     }
 
     // Initialize IGDB client with hardcoded credentials
     // Note: IGDB is optional, the app will work without it
     try {
         if (!ui.initIGDB("sa09yuxskyo4guu5d1pgntjoc3ucw0", "wu99x3crhhckdbqb41hw5u7q4sjbao")) {
             std::cerr << "Warning: Failed to initialize IGDB client. Using basic metadata." << std::endl;
         }
     } catch (const std::exception& e) {
         std::cerr << "Warning: IGDB initialization error: " << e.what() << std::endl;
         std::cerr << "Continuing with basic metadata..." << std::endl;
     }
 
     // Set up the emulator launcher with nestopia emulator
     EmulatorLauncher emulator;
     if (!emulator.init("nestopia")) {  // Assuming nestopia is in PATH
         ui.showError("Failed to initialize emulator: " + emulator.getLastError());
         return 1;
     }

 
     // Determine the games directory path relative to the executable
     // Structure: project_root/
     //           ├── build/     (executable location)
     //           └── games/     (ROM files location)
     fs::path exePath = fs::current_path();
     fs::path projectRoot = exePath.parent_path(); // Go up from build directory
     fs::path gamesDir = projectRoot / "games";
     auto roms = scanForRoms(gamesDir);
 
     // Check if any ROM files were found
     if (roms.empty()) {
         ui.showError("No ROM files found in games directory. Please add some .nes files.");
         return 1;
     }
 
     // Main program loop - display game list and handle selection
     while (true) {
         // Display game list and get selection
         int selection = ui.displayGameList(roms);
         
         // Check for exit condition (-1 returned when user chooses to exit)
         if (selection == -1) {
             break;
         }
 
         // Attempt to launch the selected game using the emulator
         fs::path romPath = gamesDir / roms[selection];
         if (!emulator.launchGame(romPath)) {
             ui.showError("Failed to launch game: " + emulator.getLastError());
             continue;
         }

     }
 
     ui.cleanup();
     return 0;
 }