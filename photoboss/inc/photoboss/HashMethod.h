#pragma once

#include <QByteArray>
#include <QImage>
#include <QString>

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>
#include <functional>

namespace photoboss
{
    // Base class for hash / fingerprint algorithms
    class HashMethod
    {
    public:
        HashMethod() = default;
        virtual ~HashMethod() = default;

        virtual QString computeHash(const QByteArray& rawBytes) const = 0;
        virtual QString computeHash(const QImage& image) const = 0;
        virtual double compareHash(const QString& hash1, const QString& hash2) const = 0;
        virtual QString getName() const noexcept = 0;

        // Enable/disable at runtime (UI/settings). Default enabled to true.
        bool isEnabled() const noexcept { return enabled_; }
        void setEnabled(bool enabled) noexcept { enabled_ = enabled; }

    private:
        bool enabled_ = true;
    };

    // Thread-safe registry that holds factories to create HashMethod instances.
    class HashRegistry
    {
    public:
        using Factory = std::function<std::unique_ptr<HashMethod>()>;

        // Register a factory for a name (returns true on successful registration).
        // This is typically called from a static object in the hash implementation.
        static bool registerFactory(const QString& name, Factory factory)
        {
            auto& reg = getMutableRegistry();
            std::lock_guard<std::mutex> lk(getMutex());
            if (reg.count(name) != 0) {
                return false; // duplicate registration
            }
            reg.emplace(name, std::move(factory));
            return true;
        }

        // Convenience template to register a concrete HashMethod type.
        template <typename T>
        static bool registerFactory(const QString& name)
        {
            static_assert(std::is_base_of<HashMethod, T>::value, "T must derive from HashMethod");
            return registerFactory(name, []() { return std::make_unique<T>(); });
        }

        // Create an instance by name; returns nullptr if name not found.
        static std::unique_ptr<HashMethod> create(const QString& name)
        {
            std::lock_guard<std::mutex> lk(getMutex());
            auto& reg = getMutableRegistry();
            auto it = reg.find(name);
            if (it == reg.end()) return nullptr;
            return it->second();
        }

        // Create one instance of every registered method.
        static std::vector<std::unique_ptr<HashMethod>> createAll()
        {
            std::vector<std::unique_ptr<HashMethod>> result;
            std::lock_guard<std::mutex> lk(getMutex());
            for (auto& kv : getMutableRegistry()) {
                result.push_back(kv.second());
            }
            return result;
        }

        // Get list of registered names.
        static std::vector<QString> registeredNames()
        {
            std::vector<QString> names;
            std::lock_guard<std::mutex> lk(getMutex());
            names.reserve(getMutableRegistry().size());
            for (auto& kv : getMutableRegistry()) names.push_back(kv.first);
            return names;
        }

    private:
        static std::unordered_map<QString, Factory>& getMutableRegistry()
        {
            static std::unordered_map<QString, Factory> registry;
            return registry;
        }

        static std::mutex& getMutex()
        {
            static std::mutex m;
            return m;
        }
    };

    // Concrete implementations forward declarations
    class Md5 : public HashMethod
    {
    public:
        QString computeHash(const QByteArray& rawBytes) const override;
        QString computeHash(const QImage& image) const override;
        double compareHash(const QString& hash1, const QString& hash2) const override;
        QString getName() const noexcept override { return QString("MD5"); }
    };

    class PerceptualHash : public HashMethod
    {
    public:
        QString computeHash(const QByteArray& rawBytes) const override;
        QString computeHash(const QImage& image) const override;
        double compareHash(const QString& hash1, const QString& hash2) const override;
        QString getName() const noexcept override { return QString("Perceptual Hash"); }
    };
}