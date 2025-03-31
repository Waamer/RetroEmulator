/**
 * @file igdb_client.h
 * @brief Implementation of the IGDBClient class for interfacing with the IGDB API.
 * 
 * This class provides functionality for authenticating with IGDB, making API requests,
 * downloading game covers, and extracting game metadata.
 * 
 * @author Jagjot and Waleed
 */

#pragma once
#include <string>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include "game_metadata.h"

class IGDBClient {
public:
    IGDBClient();
    ~IGDBClient();

    bool init(const std::string& client_id, const std::string& client_secret);
    GameMetadata fetchGameMetadata(const std::string& game_name);
    bool downloadGameCover(const std::string& url, const std::string& output_path);

private:
    CURL* curl;
    std::string access_token;
    std::string client_id;
    std::string client_secret;
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);
    std::string makeIGDBRequest(const std::string& endpoint, const std::string& query);
    bool authenticate();
    std::string cleanGameName(const std::string& filename);
    GameMetadata extractMetadataFromFilename(const std::string& filename);
}; 