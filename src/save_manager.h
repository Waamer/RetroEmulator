#pragma once
#include <string>

class SaveManager {
public:
    SaveManager();
    ~SaveManager();

    bool syncGameSave(const std::string& romName);
    bool isOnline();
    void handleSaveConflict(const std::string& romName);

private:
    std::string encrypt(const std::string& data);
    std::string decrypt(const std::string& data);
    bool uploadToCloud(const std::string& romName, const std::string& saveData);
    std::string downloadFromCloud(const std::string& romName);
    std::string getLocalSavePath(const std::string& romName);
    std::string getCloudSavePath(const std::string& romName);
};
