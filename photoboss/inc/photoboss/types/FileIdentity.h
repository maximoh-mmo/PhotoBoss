#pragma once
#include <QString>
#include "types/ExifData.h"

/// <summary>
/// 
/// FileIdentity class represents the identity of a file based on its path, size, modified time, and extension.
/// It provides accessors for these properties and supports equality comparison.
/// Construction is explicit and complete; there are no setters to modify the properties after creation.
/// 
/// </summary>

namespace photoboss {
	
	class FileIdentity final {
	public:
		FileIdentity(
			QString name = {},
			QString path = {},
			QString extension = {},
			quint64 size = 0,
			quint64 modifiedTime = 0,
			ExifData exif = {}
		)
			: m_name_(std::move(name))
			, m_path_(std::move(path))
			, m_extension_(std::move(extension))
			, m_size_(size)
			, m_modifiedTime_(modifiedTime)
			, m_exif_(exif)
		{
		}
		FileIdentity& operator=(const FileIdentity&) = delete;

		// Getters
		const QString& name() const noexcept { return m_name_; }
		const QString& path() const noexcept { return m_path_; }
		quint64 size() const noexcept { return m_size_; }
		quint64 modifiedTime() const noexcept { return m_modifiedTime_; }
		const QString& extension() const noexcept { return m_extension_; }
		const ExifData& exif() const noexcept { return m_exif_; }

		// Comparison
		bool operator==(const FileIdentity& other) const noexcept {
			return m_name_ == other.m_name_ &&
				m_path_ == other.m_path_ &&
				m_extension_ == other.m_extension_ &&
				m_size_ == other.m_size_ &&
				m_modifiedTime_ == other.m_modifiedTime_ &&
				m_exif_ == other.m_exif_;
		}

		bool operator!=(const FileIdentity& other) const noexcept {
			return !(*this == other);
		}

	private:
		const QString m_name_;
		const QString m_path_;
		const quint64 m_size_;
		const quint64 m_modifiedTime_;
		const QString m_extension_;
		const ExifData m_exif_;
	};
}