#include "save_manager.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <cstdlib>
#include <chrono>
#include <ctime>

namespace fs = std::filesystem;

SaveManager::SaveManager() {}
SaveManager::~SaveManager() {}

std::string SaveManager::getLocalSavePath(const std::string& romName) {
    return "saves/" + romName + ".sav";
}

std::string SaveManager::getCloudSavePath(const std::string& romName) {
    return "cloud_saves/" + romName + ".sav";
}

bool SaveManager::isOnline() {
    return std::system("ping -c 1 google.com > /dev/null 2>&1") == 0;
}

std::string SaveManager::encrypt(const std::string& data) {
    std::string encrypted = data;
    for (char& c : encrypted) c ^= 0xAA;
    return encrypted;
}

std::string SaveManager::decrypt(const std::string& data) {
    return encrypt(data); // XOR symmetric
}

bool SaveManager::uploadToCloud(const std::string& romName, const std::string& saveData) {
    std::ofstream out(getCloudSavePath(romName));
    if (!out) return false;
    out << encrypt(saveData);
    return true;
}

std::string SaveManager::downloadFromCloud(const std::string& romName) {
    std::ifstream in(getCloudSavePath(romName));
    if (!in) return "";
    std::string data((std::istreambuf_iterator<char>(in)), {});
    return decrypt(data);
}

void SaveManager::handleSaveConflict(const std::string& romName) {
    std::string localPath = getLocalSavePath(romName);
    std::string cloudPath = getCloudSavePath(romName);

    auto localTime = fs::last_write_time(localPath);
    auto cloudTime = fs::last_write_time(cloudPath);

    // Convert file_time_type to duration since epoch
    auto localDuration = localTime.time_since_epoch();
    auto cloudDuration = cloudTime.time_since_epoch();

    // Convert to seconds for display
    auto localSeconds = std::chrono::duration_cast<std::chrono::seconds>(localDuration).count();
    auto cloudSeconds = std::chrono::duration_cast<std::chrono::seconds>(cloudDuration).count();

    std::cout << "\nSave conflict detected!\n";
    std::cout << "1. Keep Local Save (Modified " << localSeconds << " seconds since epoch)\n";
    std::cout << "2. Use Cloud Save (Modified " << cloudSeconds << " seconds since epoch)\n";
    std::cout << "Choose an option: ";

    int choice;
    std::cin >> choice;

    if (choice == 2) {
        std::ofstream out(localPath);
        out << downloadFromCloud(romName);
    } else {
        std::ifstream in(localPath);
        std::string data((std::istreambuf_iterator<char>(in)), {});
        uploadToCloud(romName, data);
    }
}

bool SaveManager::syncGameSave(const std::string& romName) {
    std::string path = getLocalSavePath(romName);
    std::ifstream in(path);
    if (!in) return false;
    std::string data((std::istreambuf_iterator<char>(in)), {});

    if (!isOnline()) {
        std::cout << "Offline mode: Save will sync later.\n";
        return false;
    }

    if (fs::exists(getCloudSavePath(romName))) {
        handleSaveConflict(romName);
    } else {
        uploadToCloud(romName, data);
    }

    return true;
}
