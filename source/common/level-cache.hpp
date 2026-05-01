#pragma once
#include <string>
#include <unordered_map>
#include <fstream>
#include <json/json.hpp>

namespace our {
    class LevelCache {
        static inline std::unordered_map<std::string, nlohmann::json> cache;
    public:
        // Pre-parse and cache a level config file
        static void prefetch(const std::string& path) {
            if (cache.count(path)) return; // Already cached
            std::ifstream f(path);
            if (f) {
                try {
                    cache[path] = nlohmann::json::parse(f, nullptr, true, true);
                } catch (...) {
                    // Silently fail prefetch if file is malformed
                }
            }
        }

        // Get a cached config (returns nullptr-equivalent if not cached)
        static const nlohmann::json* get(const std::string& path) {
            auto it = cache.find(path);
            return it != cache.end() ? &it->second : nullptr;
        }

        // Clear the entire cache (e.g. on app shutdown)
        static void clear() { cache.clear(); }
    };
}
