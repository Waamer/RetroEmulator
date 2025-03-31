/**
 * @file ui.h
 * Implements a simple console-based user interface for the retro game emulator launcher.
 * Handles game list display, user input, and error messages.
 * @author Talha
 */

#pragma once
#include <string>
#include <vector>

class UI {
public:
    UI();
    ~UI();
    
    // Initializes the UI system
    bool init();
    
    // Displays list of games and returns selected index
    int displayGameList(const std::vector<std::string>& games);
    
    // Displays error messages
    void showError(const std::string& message);
    
    // Cleans up UI resources
    void cleanup();

private:
    bool initialized;
    
    // Gets a valid number input from user within given range
    int getNumericInput(int min, int max);
    
    // Clears the console screen
    void clearScreen();
};
