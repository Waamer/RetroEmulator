#include "sdl_ui.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <SDL_image.h>

namespace fs = std::filesystem;

SDLUI::SDLUI() : window(nullptr), renderer(nullptr), font(nullptr), initialized(false),
                 selectedIndex(0), gameSelected(false), igdbInitialized(false) {
    // Initialize colors
    backgroundColor = {32, 32, 32, 255};    // Dark gray
    textColor = {200, 200, 200, 255};       // Light gray
    selectedColor = {0, 0, 0, 255};         // Black for selection
    errorColor = {255, 0, 0, 255};          // Red
    linkColor = {0, 120, 215, 255};         // Blue for links
}

SDLUI::~SDLUI() {
    cleanup();
}

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

bool SDLUI::initIGDB(const std::string& client_id, const std::string& client_secret) {
    igdbInitialized = igdbClient.init(client_id, client_secret);
    if (!igdbInitialized) {
        std::cerr << "Note: IGDB features will be disabled. Using basic game information." << std::endl;
    }
    return true; // Always return true as this is optional functionality
}

SDL_Texture* SDLUI::loadTextureFromFile(const std::string& path) {
    // First try to load the image from the specified path
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
    }

    SDL_FreeSurface(surface);
    return texture;
}

void SDLUI::loadGameMetadata(const std::vector<std::string>& games) {
    std::cout << "Loading metadata for " << games.size() << " games..." << std::endl;
    gameList.clear();
    
    for (const auto& game : games) {
        try {
            std::cout << "Processing game: " << game << std::endl;
            gameList.push_back(igdbClient.fetchGameMetadata(game));
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

void SDLUI::renderText(const std::string& text, int x, int y, const SDL_Color& color) {
    if (!font) return;

    SDL_Surface* surface = TTF_RenderText_Blended(font, text.c_str(), color);
    if (!surface) {
        std::cerr << "Failed to render text surface! TTF_Error: " << TTF_GetError() << std::endl;
        return;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (!texture) {
        std::cerr << "Failed to create texture from rendered text! SDL_Error: " << SDL_GetError() << std::endl;
        SDL_FreeSurface(surface);
        return;
    }

    SDL_Rect dstRect = {x, y, surface->w, surface->h};
    SDL_RenderCopy(renderer, texture, NULL, &dstRect);

    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}

void SDLUI::renderWrappedText(const std::string& text, const SDL_Rect& bounds, const SDL_Color& color) {
    if (text.empty()) {
        return;
    }

    // Split text into lines and render
    std::string remainingText = text;
    int y = bounds.y;
    int lineWidth = bounds.w;
    int linesRendered = 0;
    
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
        
        // If this is the second line and there's more text, add "Read More"
        if (linesRendered == 1 && !remainingText.empty()) {
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

void SDLUI::renderGameList() {
    // Clear screen
    SDL_SetRenderDrawColor(renderer, backgroundColor.r, backgroundColor.g, backgroundColor.b, backgroundColor.a);
    SDL_RenderClear(renderer);

    // Render game list
    int y = GAME_ITEM_PADDING;
    for (size_t i = 0; i < gameList.size(); ++i) {
        const auto& game = gameList[i];
        
        // Draw selection highlight
        if (i == selectedIndex) {
            SDL_SetRenderDrawColor(renderer, selectedColor.r, selectedColor.g, selectedColor.b, selectedColor.a);
            SDL_Rect highlightRect = {10, y - GAME_ITEM_PADDING/2, WINDOW_WIDTH - 20, GAME_ITEM_HEIGHT};
            SDL_RenderFillRect(renderer, &highlightRect);
        }
        
        // Try to load game cover
        SDL_Texture* coverTexture = nullptr;
        if (!game.imagePath.empty()) {
            coverTexture = loadTextureFromFile(game.imagePath);
        }
        
        int textX = 20;
        if (coverTexture) {
            // Render cover image
            SDL_Rect coverRect = {20, y, COVER_IMAGE_SIZE, COVER_IMAGE_SIZE};
            SDL_RenderCopy(renderer, coverTexture, NULL, &coverRect);
            SDL_DestroyTexture(coverTexture);
            textX = TEXT_START_X;  // Use the new constant
        }
        
        // Render text
        renderText(game.title, textX, y, textColor);
        std::string info = game.releaseYear + " | " + game.publisher + " | " + game.genre;
        renderText(info, textX, y + 25, textColor);
        
        // Render description
        SDL_Rect descriptionBounds = {
            textX, y + 50,
            WINDOW_WIDTH - (textX + 20),
            MAX_DESCRIPTION_LINES * DESCRIPTION_LINE_HEIGHT
        };
        
        // Only show "Read More" for games with IGDB URLs
        if (!game.igdbUrl.empty()) {
            renderWrappedText(game.description, descriptionBounds, textColor);
        } else {
            // For non-IGDB games, just render the first two lines without "Read More"
            std::string remainingText = game.description;
            int lineY = y + 50;
            int linesRendered = 0;
            
            while (!remainingText.empty() && linesRendered < MAX_DESCRIPTION_LINES) {
                int textWidth, textHeight;
                TTF_SizeText(font, remainingText.c_str(), &textWidth, &textHeight);
                
                if (textWidth <= 0 || remainingText.empty()) break;
                
                int charsPerLine = std::max(1, (descriptionBounds.w * (int)remainingText.length()) / textWidth);
                charsPerLine = std::min(charsPerLine, (int)remainingText.length());
                
                size_t lineEnd = remainingText.find_last_of(" \n", charsPerLine);
                if (lineEnd == std::string::npos || lineEnd > charsPerLine) {
                    lineEnd = charsPerLine;
                }
                
                lineEnd = std::min(lineEnd, remainingText.length());
                std::string line = remainingText.substr(0, lineEnd);
                renderText(line, textX, lineY, textColor);
                
                lineY += DESCRIPTION_LINE_HEIGHT;
                linesRendered++;
                
                if (lineEnd >= remainingText.length()) break;
                remainingText = remainingText.substr(lineEnd + 1);
            }
        }
        
        y += GAME_ITEM_HEIGHT + GAME_ITEM_PADDING;
    }
    
    SDL_RenderPresent(renderer);
}

void SDLUI::handleInput() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        // Variables used in multiple case blocks
        bool isOverLink = false;
        int y = GAME_ITEM_PADDING;
        
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
                
            case SDL_MOUSEMOTION:
                // Check if mouse is over a "Read More" link
                for (size_t i = 0; i < gameList.size(); ++i) {
                    const auto& game = gameList[i];
                    if (!game.igdbUrl.empty()) {
                        int textX = 20;
                        if (!game.imagePath.empty()) {
                            textX = TEXT_START_X;
                        }
                        
                        // Calculate the position of the "Read More" link
                        int linkY = y + 50 + DESCRIPTION_LINE_HEIGHT;
                        
                        // Calculate the width of the current line
                        int textWidth, textHeight;
                        std::string line = game.description;
                        TTF_SizeText(font, line.c_str(), &textWidth, &textHeight);
                        
                        // Calculate the position of "Read More"
                        int readMoreWidth;
                        std::string readMore = " Read More";
                        TTF_SizeText(font, readMore.c_str(), &readMoreWidth, &textHeight);
                        
                        // Check if mouse is within the link area
                        if (event.motion.x >= textX + textWidth && 
                            event.motion.x <= textX + textWidth + readMoreWidth + 5 &&  // Add padding
                            event.motion.y >= linkY && 
                            event.motion.y <= linkY + DESCRIPTION_LINE_HEIGHT) {
                            isOverLink = true;
                            break;
                        }
                    }
                    y += GAME_ITEM_HEIGHT + GAME_ITEM_PADDING;
                }
                
                // Change cursor based on whether we're over a link
                if (isOverLink) {
                    SDL_SetCursor(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HAND));
                } else {
                    SDL_SetCursor(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW));
                }
                break;
                
            case SDL_MOUSEBUTTONDOWN:
                if (event.button.button == SDL_BUTTON_LEFT) {
                    // Check if click is on a "Read More" link
                    y = GAME_ITEM_PADDING;  // Reset y for the click check
                    for (size_t i = 0; i < gameList.size(); ++i) {
                        const auto& game = gameList[i];
                        if (!game.igdbUrl.empty()) {
                            int textX = 20;
                            if (!game.imagePath.empty()) {
                                textX = TEXT_START_X;
                            }
                            
                            // Calculate the position of the "Read More" link
                            int linkY = y + 50 + DESCRIPTION_LINE_HEIGHT;
                            
                            // Calculate the width of the current line
                            int textWidth, textHeight;
                            std::string line = game.description;
                            TTF_SizeText(font, line.c_str(), &textWidth, &textHeight);
                            
                            // Calculate the position of "Read More"
                            int readMoreWidth;
                            std::string readMore = " Read More";
                            TTF_SizeText(font, readMore.c_str(), &readMoreWidth, &textHeight);
                            
                            // Check if click is within the link area
                            if (event.button.x >= textX + textWidth && 
                                event.button.x <= textX + textWidth + readMoreWidth + 5 &&  // Add padding
                                event.button.y >= linkY && 
                                event.button.y <= linkY + DESCRIPTION_LINE_HEIGHT) {
                                
                                // Open the URL in the default browser
                                std::string command = "xdg-open \"" + game.igdbUrl + "\"";
                                system(command.c_str());
                                return;
                            }
                        }
                        y += GAME_ITEM_HEIGHT + GAME_ITEM_PADDING;
                    }
                }
                break;
        }
    }
}

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