#pragma once
#include <SDL.h>
#include <SDL_ttf.h>
#include <string>
#include <vector>
#include "game_metadata.h"
#include "igdb_client.h"

class SDLUI {
public:
    SDLUI();
    ~SDLUI();
    
    bool init();
    bool initIGDB(const std::string& client_id, const std::string& client_secret);
    void loadGameMetadata(const std::vector<std::string>& games);
    int displayGameList(const std::vector<std::string>& games);
    void showError(const std::string& message);
    void cleanup();

private:
    static const int WINDOW_WIDTH = 800;
    static const int WINDOW_HEIGHT = 600;
    static const int GAME_ITEM_HEIGHT = 140;
    static const int GAME_ITEM_PADDING = 20;
    static const int DESCRIPTION_LINE_HEIGHT = 25;
    static const int MAX_DESCRIPTION_LINES = 2;
    static const int COVER_IMAGE_SIZE = 100;
    static const int TEXT_START_X = 130;

    SDL_Window* window;
    SDL_Renderer* renderer;
    TTF_Font* font;
    bool initialized;
    bool igdbInitialized;
    int selectedIndex;
    bool gameSelected;
    std::vector<GameMetadata> gameList;
    IGDBClient igdbClient;

    // Colors
    SDL_Color backgroundColor;
    SDL_Color textColor;
    SDL_Color selectedColor;
    SDL_Color errorColor;
    SDL_Color linkColor;

    void renderText(const std::string& text, int x, int y, const SDL_Color& color);
    void renderWrappedText(const std::string& text, const SDL_Rect& bounds, const SDL_Color& color);
    void renderGameList();
    void handleInput();
    SDL_Texture* loadTextureFromFile(const std::string& path);
}; 