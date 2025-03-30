/**
 * @file ui.cpp
 * Implements a simple console-based user interface for the retro game emulator launcher.
 * Handles game list display, user input, and error messages.
 */

#include "ui.h"
#include <iostream>
#include <limits>
#include <cstdlib>

/**
 * Constructor initializes UI system in uninitialized state
 */
UI::UI() : initialized(false) {}

/**
 * Destructor ensures cleanup is performed
 */
UI::~UI() {
    cleanup();
}

/**
 * Initializes the UI system
 * @return true if initialization successful
 */
bool UI::init() {
    initialized = true;
    return true;
}

/**
 * Cleans up UI resources
 */
void UI::cleanup() {
    initialized = false;
}

/**
 * Displays the list of available games and handles user selection
 * @param games Vector of game filenames to display
 * @return Selected game index or -1 for exit
 */
int UI::displayGameList(const std::vector<std::string>& games) {
    if (!initialized || games.empty()) {
        return -1;
    }

    while (true) {
        clearScreen();
        std::cout << "=== Retro Gaming Console ===" << std::endl;
        std::cout << "Available Games:" << std::endl;
        
        // Display games with numbers
        for (size_t i = 0; i < games.size(); ++i) {
            std::cout << i + 1 << ". " << games[i] << std::endl;
        }
        
        std::cout << "\n0. Exit" << std::endl;
        std::cout << "\nEnter game number to play: ";
        
        int choice = getNumericInput(0, games.size());
        
        if (choice == 0) {
            return -1;  // Exit
        }
        
        return choice - 1;  // Convert to zero-based index
    }
}

/**
 * Displays an error message and waits for user acknowledgment
 * @param message Error message to display
 */
void UI::showError(const std::string& message) {
    std::cerr << "\nError: " << message << std::endl;
    std::cout << "Press Enter to continue...";
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

/**
 * Gets validated numeric input from the user within specified range
 * @param min Minimum acceptable value
 * @param max Maximum acceptable value
 * @return User's validated input
 */
int UI::getNumericInput(int min, int max) {
    int choice;
    while (true) {
        if (std::cin >> choice) {
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            if (choice >= min && choice <= max) {
                return choice;
            }
        } else {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        }
        std::cout << "Please enter a number between " << min << " and " << max << ": ";
    }
}

/**
 * Clears the console screen in a platform-independent way
 * Uses 'cls' for Windows and 'clear' for Unix-like systems
 */
void UI::clearScreen() {
    #ifdef _WIN32
        system("cls");
    #else
        system("clear");
    #endif
}
