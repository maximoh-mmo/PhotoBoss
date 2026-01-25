#pragma once

#include <QMap>
#include <QString>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>
#include <functional>
#include <set>
#include "hashing/HashMethod.h"

namespace photoboss
{

    /// <summary>
    /// Thread-safe registry that holds factories to create HashMethod instances. 
    /// </summary>
    class HashRegistry
    {
    public:
        using Factory = std::function<std::unique_ptr<HashMethod>()>;

        struct Entry {
            QString key;
            Factory factory;
        };

        // Register a factory for a name (returns true on successful registration).
        // This is typically called from a static object in the hash implementation.
        static bool registerFactory(QString key, Factory factory)
        {
            auto& registry = getMutableRegistry();
            std::lock_guard<std::mutex> lock(getMutex());
            if (registry.count(key) != 0) {
                return false; // duplicate registration
            }
            registry.emplace(key, std::move(factory));
            return true;
        }

        // Convenience template to register a concrete HashMethod type.
        template <typename T>
        static bool registerFactory(const char* key)
        {
            static_assert(std::is_base_of<HashMethod, T>::value, "T must derive from HashMethod");
            return registerFactory(key, []() { return std::make_unique<T>(); });
        }

        // Create a snapshot of registered entries filtered by enabled keys.
        static std::vector<Entry> createSnapshot(const std::set<QString>& enabledKeys) {
            std::vector<Entry> snapshot;
            std::lock_guard<std::mutex> lock(getMutex());
            for (auto& kv : getMutableRegistry()) {
                if (enabledKeys.empty() || enabledKeys.count(kv.first)) {
                    snapshot.push_back(Entry{ kv.first, kv.second });
                }
            }
            return snapshot;
        }

        static std::vector<QString> registeredNames() {
            std::vector<QString> names;
            std::lock_guard<std::mutex> lock(getMutex());
            for (const auto& kv : getMutableRegistry()) {
                names.push_back(kv.first);
            }
            return names;
        }

        static void initializeBuiltIns() {
            // This empty reference forces the compiler to include the translation units
            extern const bool ph_registered;
            (void)ph_registered;
        }


    private:
        static std::unordered_map<QString, Factory>& getMutableRegistry()
        {
            static std::unordered_map<QString, Factory> registry;
            return registry;
        }

        static std::mutex& getMutex()
        {
            static std::mutex mutex;
            return mutex;
        }
    };
}