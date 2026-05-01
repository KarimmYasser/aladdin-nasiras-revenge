#pragma once
#include <fstream>
#include <json/json.hpp>
#include <iostream>

namespace our {
    class SaveSystem {
    public:
        static int getUnlockedLevels() {
            std::ifstream f("save.json");
            if (!f.is_open()) return 1; // Default: only Level 1
            try {
                nlohmann::json save;
                f >> save;
                return save.value("unlockedLevels", 1);
            } catch (...) {
                return 1;
            }
        }

        static bool getDebugMode() {
            std::ifstream f("save.json");
            if (!f.is_open()) return false;
            try {
                nlohmann::json save;
                f >> save;
                return save.value("debugMode", false);
            } catch (...) {
                return false;
            }
        }

        static void setDebugMode(bool enabled) {
            nlohmann::json save;
            std::ifstream f_in("save.json");
            if (f_in.is_open()) {
                try { f_in >> save; } catch (...) {}
                f_in.close();
            }
            save["debugMode"] = enabled;
            std::ofstream f_out("save.json");
            f_out << save.dump(4);
        }

        static void unlockLevel(int levelIndex) {
            int current = getUnlockedLevels();
            if (levelIndex <= current) return;

            nlohmann::json save;
            std::ifstream f_in("save.json");
            if (f_in.is_open()) {
                try { f_in >> save; } catch (...) {}
                f_in.close();
            }
            save["unlockedLevels"] = levelIndex;
            std::ofstream f_out("save.json");
            f_out << save.dump(4);
        }

        static void unlockNextLevel(std::string currentLevelPath) {
            // Normalize slashes
            std::replace(currentLevelPath.begin(), currentLevelPath.end(), '\\', '/');

            int nextLevelIndex = 1;
            if (currentLevelPath == "config/levels/level1.jsonc") nextLevelIndex = 2;
            else if (currentLevelPath == "config/levels/level2.jsonc") nextLevelIndex = 3;
            else if (currentLevelPath == "config/levels/level3.jsonc") nextLevelIndex = 4;

            unlockLevel(nextLevelIndex);
        }
    };
}
