#include "igdb_client.h"
#include <iostream>
#include <sstream>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

IGDBClient::IGDBClient() : curl(nullptr) {
    curl = curl_easy_init();
}

IGDBClient::~IGDBClient() {
    if (curl) {
        curl_easy_cleanup(curl);
    }
}

size_t IGDBClient::WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

bool IGDBClient::init(const std::string& client_id, const std::string& client_secret) {
    std::cout << "Initializing IGDB client..." << std::endl;
    
    if (!curl) {
        std::cerr << "Failed to initialize CURL" << std::endl;
        return false;
    }

    // Set timeout values to prevent hanging
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);  // 10 seconds timeout for the entire request
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);  // 5 seconds timeout for the connection phase

    // Create images directory if it doesn't exist
    try {
        if (!fs::exists("images")) {
            fs::create_directory("images");
        }
    } catch (const std::exception& e) {
        std::cerr << "Failed to create images directory: " << e.what() << std::endl;
        // Continue anyway, we'll handle image saving failures later
    }

    return authenticate();
}

bool IGDBClient::authenticate() {
    std::cout << "Authenticating with IGDB..." << std::endl;
    
    // Set SSL verification options
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    
    std::string auth_url = "https://id.twitch.tv/oauth2/token";
    std::string post_data = "client_id=sa09yuxskyo4guu5d1pgntjoc3ucw0"
                           "&client_secret=wu99x3crhhckdbqb41hw5u7q4sjbao"
                           "&grant_type=client_credentials";

    std::string response;
    curl_easy_setopt(curl, CURLOPT_URL, auth_url.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    std::cout << "Sending authentication request..." << std::endl;
    CURLcode res = curl_easy_perform(curl);
    
    if (res != CURLE_OK) {
        std::cerr << "Failed to authenticate: " << curl_easy_strerror(res) << std::endl;
        std::cerr << "Continuing without IGDB integration" << std::endl;
        return false;
    }

    std::cout << "Got response from authentication server" << std::endl;
    
    try {
        auto json = nlohmann::json::parse(response);
        if (!json.contains("access_token")) {
            std::cerr << "Authentication response missing access token" << std::endl;
            std::cerr << "Response: " << response << std::endl;
            std::cerr << "Continuing without IGDB integration" << std::endl;
            return false;
        }
        access_token = json["access_token"];
        std::cout << "Successfully authenticated with IGDB" << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to parse authentication response: " << e.what() << std::endl;
        std::cerr << "Response: " << response << std::endl;
        std::cerr << "Continuing without IGDB integration" << std::endl;
        return false;
    }
}

std::string IGDBClient::cleanGameName(const std::string& filename) {
    std::string name = filename.substr(0, filename.find_last_of('.'));
    std::replace(name.begin(), name.end(), '_', ' ');
    // Remove common suffixes like (U), (E), (J), etc.
    size_t pos = name.find(" (");
    if (pos != std::string::npos) {
        name = name.substr(0, pos);
    }
    return name;
}

std::string IGDBClient::makeIGDBRequest(const std::string& endpoint, const std::string& query) {
    std::string url = "https://api.igdb.com/v4/" + endpoint;
    std::string response;

    std::cout << "Making IGDB request to: " << url << std::endl;
    std::cout << "Query: " << query << std::endl;

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, ("Authorization: Bearer " + access_token).c_str());
    headers = curl_slist_append(headers, "Client-ID: sa09yuxskyo4guu5d1pgntjoc3ucw0");
    headers = curl_slist_append(headers, "Content-Type: text/plain");

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, query.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    CURLcode res = curl_easy_perform(curl);
    curl_slist_free_all(headers);

    if (res != CURLE_OK) {
        std::cerr << "Failed to make IGDB request: " << curl_easy_strerror(res) << std::endl;
        return "";
    }

    std::cout << "IGDB Response: " << response << std::endl;
    return response;
}

bool IGDBClient::downloadGameCover(const std::string& url, const std::string& output_path) {
    if (url.empty()) return false;

    std::cout << "Downloading cover image from: " << url << std::endl;

    // Reset all options
    curl_easy_reset(curl);
    
    // Set SSL verification options
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    
    // Set timeouts
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);

    // Create a string to hold the image data
    std::string image_data;

    // Setup for binary file download
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);  // Follow redirects
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &image_data);

    std::cout << "Starting download..." << std::endl;
    CURLcode res = curl_easy_perform(curl);

    if (res != CURLE_OK) {
        std::cerr << "Failed to download image: " << curl_easy_strerror(res) << std::endl;
        return false;
    }

    // Write the image data to file
    std::ofstream outfile(output_path, std::ios::binary);
    if (!outfile.is_open()) {
        std::cerr << "Failed to open output file: " << output_path << std::endl;
        return false;
    }

    outfile.write(image_data.data(), image_data.size());
    outfile.close();

    // Verify the file was actually downloaded
    if (fs::exists(output_path) && fs::file_size(output_path) > 0) {
        std::cout << "Successfully downloaded cover image to: " << output_path << std::endl;
        return true;
    } else {
        std::cerr << "Download appeared to succeed but file is empty or missing" << std::endl;
        if (fs::exists(output_path)) {
            fs::remove(output_path);
        }
        return false;
    }
}

GameMetadata IGDBClient::extractMetadataFromFilename(const std::string& filename) {
    GameMetadata metadata;
    metadata.filename = filename;
    
    // Remove .nes extension and clean up filename
    std::string cleanName;
    size_t dotPos = filename.find_last_of('.');
    if (dotPos != std::string::npos) {
        cleanName = filename.substr(0, dotPos);
    } else {
        cleanName = filename;
    }
    std::replace(cleanName.begin(), cleanName.end(), '_', ' ');
    
    // Remove region codes and other common suffixes
    size_t pos = cleanName.find(" (");
    if (pos != std::string::npos) {
        cleanName = cleanName.substr(0, pos);
    }
    
    metadata.title = cleanName;
    metadata.description = "No description found in IGDB database";
    metadata.releaseYear = "Not Found";
    metadata.publisher = "Not Found";
    metadata.genre = "Not Found";
    metadata.igdbUrl = "";  // No URL for games not found in IGDB
    
    // Use the default "not found" image from assets directory
    metadata.imagePath = "../assets/not_found.png";
    
    return metadata;
}

GameMetadata IGDBClient::fetchGameMetadata(const std::string& game_name) {
    GameMetadata metadata;
    metadata.filename = game_name;
    
    // If we don't have an access token, return basic metadata
    if (access_token.empty()) {
        std::cout << "No access token available, using basic metadata" << std::endl;
        return extractMetadataFromFilename(game_name);
    }

    std::string clean_name;
    size_t dotPos = game_name.find_last_of('.');
    if (dotPos != std::string::npos) {
        clean_name = game_name.substr(0, dotPos);
    } else {
        clean_name = game_name;
    }
    
    // Remove region codes and other common suffixes
    size_t pos = clean_name.find(" (");
    if (pos != std::string::npos) {
        clean_name = clean_name.substr(0, pos);
    }
    
    // Replace underscores with spaces
    std::replace(clean_name.begin(), clean_name.end(), '_', ' ');
    
    std::cout << "Fetching metadata for game: " << clean_name << std::endl;

    try {
        // Escape quotes in the game name
        std::string escaped_name = clean_name;
        size_t pos = 0;
        while ((pos = escaped_name.find("\"", pos)) != std::string::npos) {
            escaped_name.replace(pos, 1, "\\\"");
            pos += 2;
        }

        // Search for the game with a simpler query first
        std::string query = "search \"" + escaped_name + "\"; fields name; where platforms = (18);";
        std::string response = makeIGDBRequest("games", query);

        if (response.empty()) {
            std::cout << "No IGDB data found, using filename-based metadata" << std::endl;
            return extractMetadataFromFilename(game_name);
        }

        auto json = nlohmann::json::parse(response);
        if (!json.is_array() || json.empty()) {
            std::cout << "No matching games found in IGDB" << std::endl;
            return extractMetadataFromFilename(game_name);
        }

        // Get the first game's ID
        if (!json[0].contains("id")) {
            std::cout << "Game ID not found in response" << std::endl;
            return extractMetadataFromFilename(game_name);
        }
        int game_id = json[0]["id"].get<int>();
        
        // Now fetch detailed information using the ID
        query = "fields name,first_release_date,genres.name,cover.url,summary,involved_companies.company.name,url; where id = " + std::to_string(game_id) + ";";
        response = makeIGDBRequest("games", query);
        
        if (response.empty()) {
            std::cout << "Failed to fetch detailed game data" << std::endl;
            return extractMetadataFromFilename(game_name);
        }

        json = nlohmann::json::parse(response);
        if (!json.is_array() || json.empty()) {
            std::cout << "Invalid detailed game data response" << std::endl;
            return extractMetadataFromFilename(game_name);
        }

        const auto& game = json[0];
        
        // Basic metadata with fallbacks
        metadata.title = game.value("name", clean_name);
        metadata.description = game.value("summary", "Classic NES game");
        metadata.releaseYear = "Unknown";
        metadata.publisher = "Unknown";
        metadata.genre = "Unknown";
        metadata.igdbUrl = game.value("url", "");  // Get the IGDB URL

        // Handle release date
        if (game.contains("first_release_date") && game["first_release_date"].is_number()) {
            try {
                std::time_t release_date = game["first_release_date"].get<std::time_t>();
                std::tm* timeinfo = std::localtime(&release_date);
                if (timeinfo) {
                    metadata.releaseYear = std::to_string(1900 + timeinfo->tm_year);
                }
            } catch (const std::exception& e) {
                std::cerr << "Failed to parse release date: " << e.what() << std::endl;
            }
        }

        // Handle publisher (use first company found)
        if (game.contains("involved_companies") && game["involved_companies"].is_array()) {
            const auto& companies = game["involved_companies"];
            for (const auto& company_data : companies) {
                if (company_data.contains("company") && 
                    company_data["company"].contains("name") &&
                    company_data["company"]["name"].is_string()) {
                    metadata.publisher = company_data["company"]["name"].get<std::string>();
                    break;  // Use first company found
                }
            }
        }

        // Handle genre (use first genre found)
        if (game.contains("genres") && game["genres"].is_array() && !game["genres"].empty()) {
            const auto& genres = game["genres"];
            for (const auto& genre : genres) {
                if (genre.contains("name") && genre["name"].is_string()) {
                    metadata.genre = genre["name"].get<std::string>();
                    break;  // Use first genre found
                }
            }
        }

        // Handle cover image
        if (game.contains("cover") && game["cover"].is_object() && 
            game["cover"].contains("url") && game["cover"]["url"].is_string()) {
            try {
                std::string cover_url = "https:" + game["cover"]["url"].get<std::string>();
                std::string image_path = "images/" + clean_name + ".png";
                if (downloadGameCover(cover_url, image_path)) {
                    metadata.imagePath = image_path;
                }
            } catch (const std::exception& e) {
                std::cerr << "Failed to handle cover image: " << e.what() << std::endl;
            }
        }
        
        std::cout << "Successfully fetched metadata for: " << metadata.title << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Failed to parse game metadata: " << e.what() << std::endl;
        return extractMetadataFromFilename(game_name);
    } catch (...) {
        std::cerr << "Unknown error while fetching game metadata" << std::endl;
        return extractMetadataFromFilename(game_name);
    }

    return metadata;
} 