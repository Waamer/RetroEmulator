/**
 * @file sdl_ui.h
 * @brief Implementation of the SDLUI class for rendering a game launcher UI using SDL.
 * @author Talha and Waleed
 */

#include "sdl_ui.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <SDL_image.h>

namespace fs = std::filesystem;

/**
 * @brief Constructs an SDLUI object and initializes colors.
 */
SDLUI::SDLUI() : window(nullptr), renderer(nullptr), font(nullptr), initialized(false),
                 selectedIndex(0), gameSelected(false), igdbInitialized(false) {
    // Initialize colors
    backgroundColor = {32, 32, 32, 255};    // Dark gray
    textColor = {200, 200, 200, 255};       // Light gray
    selectedColor = {0, 0, 0, 255};         // Black for selection
    errorColor = {255, 0, 0, 255};          // Red
    linkColor = {0, 120, 215, 255};         // Blue for links
}
/**
 * @brief Destroys the SDLUI object and cleans up resources.
 */
SDLUI::~SDLUI() {
    clearTextureCache();
    cleanup();
}

/**
 * @brief Initializes SDL, SDL_ttf, and SDL_image for rendering.
 * @return True if initialization succeeds, false otherwise.
 */
bool SDLUI::init() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }

    if (TTF_Init() < 0) {
        std::cerr << "SDL_ttf could not initialize! TTF_Error: " << TTF_GetError() << std::endl;
        return false;
    }

    window = SDL_CreateWindow("NES Game Launcher",
                            SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                            WINDOW_WIDTH, WINDOW_HEIGHT,
                            SDL_WINDOW_SHOWN);

    if (!window) {
        std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        std::cerr << "Renderer could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }

    // Initialize SDL_image
    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        std::cerr << "SDL_image could not initialize! SDL_image Error: " << IMG_GetError() << std::endl;
        return false;
    }

    font = TTF_OpenFont("Urbanist-VariableFont_wght.ttf", 18);
    if (!font) {
        font = TTF_OpenFont("../Urbanist-VariableFont_wght.ttf", 18);
        if (!font) {
            std::cerr << "Failed to load font! TTF_Error: " << TTF_GetError() << std::endl;
            return false;
        }
    }

    initialized = true;
    return true;
}

/**
 * @brief Initializes the IGDB client for retrieving game metadata.
 * @param client_id The client ID for IGDB API authentication.
 * @param client_secret The client secret for IGDB API authentication.
 * @return Always returns true (functionality is optional).
 */
bool SDLUI::initIGDB(const std::string& client_id, const std::string& client_secret) {
    igdbInitialized = igdbClient.init(client_id, client_secret);
    if (!igdbInitialized) {
        std::cerr << "Note: IGDB features will be disabled. Using basic game information." << std::endl;
    }
    return true; // Always return true as this is optional functionality
}

/**
 * @brief Loads an image file into an SDL texture.
 * @param path The file path of the image.
 * @return The loaded SDL_Texture or nullptr if loading fails.
 */
SDL_Texture* SDLUI::loadTextureFromFile(const std::string& path) {
    auto it = textureCache.find(path);
    if (it != textureCache.end()) {
        return it->second;
    }

    SDL_Surface* surface = IMG_Load(path.c_str());
    if (!surface) {
        std::cerr << "Failed to load image " << path << "! SDL_image Error: " << IMG_GetError() << std::endl;
        
        // Try loading from the assets directory
        std::string assetPath = "../assets/" + path;
        surface = IMG_Load(assetPath.c_str());
        
        if (!surface) {
            std::cerr << "Failed to load image from assets directory: " << assetPath << std::endl;
            return nullptr;
        }
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (!texture) {
        std::cerr << "Failed to create texture from " << path << "! SDL Error: " << SDL_GetError() << std::endl;
        SDL_FreeSurface(surface);
        return nullptr;
    }

    SDL_FreeSurface(surface);
    textureCache[path] = texture;
    return texture;
}

/**
 * @brief Loads metadata for a list of games.
 * @param games A vector containing game filenames.
 */
void SDLUI::loadGameMetadata(const std::vector<std::string>& games) {
    std::cout << "Loading metadata for " << games.size() << " games..." << std::endl;
    gameList.clear();
    
    // Pre-allocate space for better performance
    gameList.reserve(games.size());
    
    for (const auto& game : games) {
        try {
            std::cout << "Processing game: " << game << std::endl;
            gameList.push_back(igdbClient.fetchGameMetadata(game));
            
            // Pre-load the cover image texture if available
            if (!gameList.back().imagePath.empty()) {
                loadTextureFromFile(gameList.back().imagePath);
            }
        } catch (const std::exception& e) {
            std::cerr << "Error processing game " << game << ": " << e.what() << std::endl;
            // Create basic metadata for this game
            GameMetadata basic;
            basic.filename = game;
            basic.title = game.substr(0, game.find_last_of('.'));
            basic.description = "Classic NES game";
            basic.releaseYear = "Unknown";
            basic.publisher = "Unknown";
            basic.genre = "Unknown";
            gameList.push_back(basic);
        }
    }
    
    std::cout << "Finished loading metadata for " << gameList.size() << " games" << std::endl;
}

/**
 * @brief Renders text onto the screen at a specified position.
 * @param text The text string to render.
 * @param x The x-coordinate for rendering.
 * @param y The y-coordinate for rendering.
 * @param color The SDL_Color to use for text rendering.
 */
void SDLUI::renderText(const std::string& text, int x, int y, const SDL_Color& color) {
    if (!font) return;

    SDL_Texture* texture = getOrCreateTextTexture(text, color);
    if (!texture) return;

    int w, h;
    SDL_QueryTexture(texture, NULL, NULL, &w, &h);
    SDL_Rect dstRect = {x, y, w, h};
    SDL_RenderCopy(renderer, texture, NULL, &dstRect);
}

/**
 * @brief Renders wrapped text within a specified boundary.
 * @param text The text string to render.
 * @param bounds The SDL_Rect specifying the text boundaries.
 * @param color The SDL_Color to use for text rendering.
 */
void SDLUI::renderWrappedText(const std::string& text, const SDL_Rect& bounds, const SDL_Color& color) {
    if (text.empty()) {
        return;
    }

    // Split text into lines and render
    std::string remainingText = text;
    int y = bounds.y;
    int lineWidth = bounds.w;
    int linesRendered = 0;
    
    // Check if this text should have a Read More link
    bool shouldAddReadMore = text.find(" Read More") != std::string::npos;
    if (shouldAddReadMore) {
        // Remove the " Read More" from the text before processing
        remainingText = text.substr(0, text.find(" Read More"));
    }
    
    while (!remainingText.empty() && linesRendered < MAX_DESCRIPTION_LINES) {
        // Calculate how many characters fit in one line
        int textWidth, textHeight;
        TTF_SizeText(font, remainingText.c_str(), &textWidth, &textHeight);
        
        // Ensure we don't divide by zero
        if (textWidth <= 0 || remainingText.empty()) {
            break;
        }
        
        // Calculate characters per line, ensuring at least one character
        int charsPerLine = std::max(1, (lineWidth * (int)remainingText.length()) / textWidth);
        
        // Ensure we don't exceed string length
        charsPerLine = std::min(charsPerLine, (int)remainingText.length());
        
        // Find the last space before the cut-off point
        size_t lineEnd = remainingText.find_last_of(" \n", charsPerLine);
        if (lineEnd == std::string::npos || lineEnd > charsPerLine) {
            lineEnd = charsPerLine;
        }
        
        // Ensure lineEnd is valid
        lineEnd = std::min(lineEnd, remainingText.length());
        
        // Extract and render the line
        std::string line = remainingText.substr(0, lineEnd);
        
        // If this is the second line and we should add Read More
        if (linesRendered == 1 && shouldAddReadMore) {
            // Calculate how much space we need for "Read More"
            int readMoreWidth, readMoreHeight;
            std::string readMore = " Read More";
            TTF_SizeText(font, readMore.c_str(), &readMoreWidth, &readMoreHeight);
            
            // Calculate the width of the current line
            int currentLineWidth;
            TTF_SizeText(font, line.c_str(), &currentLineWidth, &readMoreHeight);
            
            // If we have enough space, add "Read More" to the line
            if (lineWidth - currentLineWidth > readMoreWidth + 10) {  // 10 pixels padding
                renderText(line, bounds.x, y, color);
                renderText(readMore, bounds.x + currentLineWidth + 5, y, linkColor);
            } else {
                // If not enough space, truncate the line and add "Read More" at the end
                // Find the last space before the cut-off point that leaves room for "Read More"
                while (currentLineWidth + readMoreWidth + 10 > lineWidth && !line.empty()) {
                    line = line.substr(0, line.find_last_of(" "));
                    TTF_SizeText(font, line.c_str(), &currentLineWidth, &readMoreHeight);
                }
                renderText(line, bounds.x, y, color);
                renderText(readMore, bounds.x + currentLineWidth + 5, y, linkColor);
            }
        } else {
            renderText(line, bounds.x, y, color);
        }
        
        // Move to next line
        y += DESCRIPTION_LINE_HEIGHT;
        linesRendered++;
        
        // Move to next portion of text, handling the case where lineEnd is at the end
        if (lineEnd >= remainingText.length()) {
            break;
        }
        remainingText = remainingText.substr(lineEnd + 1);
    }
}

/**
 * @brief Renders the list of available games on the screen.
 */
void SDLUI::renderGameList() {
    // Clear the screen
    SDL_SetRenderDrawColor(renderer, backgroundColor.r, backgroundColor.g, backgroundColor.b, backgroundColor.a);
    SDL_RenderClear(renderer);

    // Render game list
    int y = GAME_ITEM_PADDING;
    for (size_t i = 0; i < gameList.size(); ++i) {
        const auto& game = gameList[i];
        
        // Draw selection background if this is the selected item
        if (i == selectedIndex) {
            SDL_SetRenderDrawColor(renderer, selectedColor.r, selectedColor.g, selectedColor.b, selectedColor.a);
            SDL_Rect selectionRect = {0, y - 5, WINDOW_WIDTH, GAME_ITEM_HEIGHT + 10};
            SDL_RenderFillRect(renderer, &selectionRect);
        }

        // Render game cover image
        if (!game.imagePath.empty()) {
            SDL_Texture* coverTexture = loadTextureFromFile(game.imagePath);
            if (coverTexture) {
                SDL_Rect coverRect = {GAME_ITEM_PADDING, y, COVER_IMAGE_SIZE, COVER_IMAGE_SIZE};
                SDL_RenderCopy(renderer, coverTexture, NULL, &coverRect);
            }
        }

        // Render game title
        renderText(game.title, TEXT_START_X, y, textColor);

        // Render game details in one line
        std::string details = game.releaseYear + " | " + game.publisher + " | " + game.genre;
        renderText(details, TEXT_START_X, y + 25, textColor);

        // Render description with Read More link only if game is found in IGDB
        SDL_Rect descBounds = {TEXT_START_X, y + 50, WINDOW_WIDTH - TEXT_START_X - GAME_ITEM_PADDING, 40};
        if (!game.igdbUrl.empty()) {
            // Game found in IGDB, show description with Read More
            renderWrappedText(game.description + " Read More", descBounds, textColor);
        } else {
            // Game not found in IGDB, show description without Read More
            renderWrappedText(game.description, descBounds, textColor);
        }

        y += GAME_ITEM_HEIGHT + GAME_ITEM_PADDING;
    }

    SDL_RenderPresent(renderer);
}

/**
 * @brief Handles user input, such as keyboard and mouse events.
 */
void SDLUI::handleInput() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                selectedIndex = -1;  // Signal to exit
                return;
                
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                    case SDLK_UP:
                        if (selectedIndex > 0) selectedIndex--;
                        break;
                    case SDLK_DOWN:
                        if (selectedIndex < static_cast<int>(gameList.size() - 1)) selectedIndex++;
                        break;
                    case SDLK_RETURN:
                        gameSelected = true;  // Set flag to indicate game was selected
                        return;
                    case SDLK_ESCAPE:
                        selectedIndex = -1;  // Signal to exit
                        return;
                }
                break;
                
            case SDL_MOUSEBUTTONDOWN:
                if (event.button.button == SDL_BUTTON_LEFT) {
                    // Calculate which game item was clicked
                    int clickY = event.button.y;
                    int itemY = GAME_ITEM_PADDING;
                    
                    for (size_t i = 0; i < gameList.size(); ++i) {
                        const auto& game = gameList[i];
                        
                        // Check if click is in the description area and game has IGDB URL
                        if (!game.igdbUrl.empty() && 
                            clickY >= itemY + 50 && 
                            clickY <= itemY + 90 && 
                            event.button.x >= TEXT_START_X) {
                            
                            // Open the URL in the default browser
                            #ifdef _WIN32
                                std::string command = "start " + game.igdbUrl;
                            #else
                                std::string command = "xdg-open \"" + game.igdbUrl + "\"";
                            #endif
                            system(command.c_str());
                            return;
                        }
                        
                        itemY += GAME_ITEM_HEIGHT + GAME_ITEM_PADDING;
                    }
                }
                break;
        }
    }
}

/**
 * @brief Displays the game list and handles user interaction.
 * @param games A vector containing game filenames.
 * @return The index of the selected game, or -1 if the user exits.
 */

int SDLUI::displayGameList(const std::vector<std::string>& games) {
    loadGameMetadata(games);
    gameSelected = false;  // Reset selection flag
    
    while (true) {
        renderGameList();
        handleInput();
        
        if (selectedIndex == -1) {
            return -1;  // Exit selected
        }
        
        if (gameSelected) {
            return selectedIndex;  // Return the selected game index
        }
        
        SDL_Delay(16);  // Cap at ~60 FPS
    }
}

/**
 * @brief Displays an error message on the screen.
 * @param message The error message to display.
 */

void SDLUI::showError(const std::string& message) {
    // Clear screen
    SDL_SetRenderDrawColor(renderer, backgroundColor.r, backgroundColor.g, backgroundColor.b, backgroundColor.a);
    SDL_RenderClear(renderer);
    
    // Render error message
    renderText("Error: " + message, 20, 20, errorColor);
    renderText("Press ESC to continue", 20, 40, textColor);
    
    SDL_RenderPresent(renderer);
    
    // Wait for ESC key
    SDL_Event event;
    bool waiting = true;
    while (waiting) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
                waiting = false;
                break;
            }
        }
        SDL_Delay(16);
    }
}
/**
 * @brief Cleans up SDL resources before exiting.
 */
void SDLUI::cleanup() {
    if (font) {
        TTF_CloseFont(font);
        font = nullptr;
    }
    if (renderer) {
        SDL_DestroyRenderer(renderer);
        renderer = nullptr;
    }
    if (window) {
        SDL_DestroyWindow(window);
        window = nullptr;
    }
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
    initialized = false;
}

void SDLUI::clearTextureCache() {
    for (auto& pair : textureCache) {
        if (pair.second) {
            SDL_DestroyTexture(pair.second);
        }
    }
    textureCache.clear();

    for (auto& pair : textTextureCache) {
        if (pair.second) {
            SDL_DestroyTexture(pair.second);
        }
    }
    textTextureCache.clear();
}

SDL_Texture* SDLUI::getOrCreateTextTexture(const std::string& text, const SDL_Color& color) {
    // Create a unique key for the text and color
    std::string key = text + std::to_string(color.r) + std::to_string(color.g) + 
                     std::to_string(color.b) + std::to_string(color.a);
    
    auto it = textTextureCache.find(key);
    if (it != textTextureCache.end()) {
        return it->second;
    }

    SDL_Surface* surface = TTF_RenderText_Blended(font, text.c_str(), color);
    if (!surface) {
        std::cerr << "Failed to render text surface! TTF_Error: " << TTF_GetError() << std::endl;
        return nullptr;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (!texture) {
        std::cerr << "Failed to create texture from rendered text! SDL Error: " << SDL_GetError() << std::endl;
        SDL_FreeSurface(surface);
        return nullptr;
    }

    SDL_FreeSurface(surface);
    textTextureCache[key] = texture;
    return texture;
} 