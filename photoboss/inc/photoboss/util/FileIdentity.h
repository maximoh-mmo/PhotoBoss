#pragma once
#include <QString>
#include "util/ExifData.h"

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
			QString path = {},
			QString extension = {},
			quint64 size = 0,
			quint64 modifiedTime = 0,
			ExifData exif = {}
		)
			: m_path(std::move(path))
			, m_extension(std::move(extension))
			, m_size(size)
			, m_modifiedTime(modifiedTime)
			, m_exif(exif)
		{
		}
		FileIdentity& operator=(const FileIdentity&) = delete;

		// Getters
		const QString& path() const noexcept { return m_path; }
		quint64 size() const noexcept { return m_size; }
		quint64 modifiedTime() const noexcept { return m_modifiedTime; }
		const QString& extension() const noexcept { return m_extension; }
		const ExifData& exif() const noexcept { return m_exif; }

		// Comparison
		bool operator==(const FileIdentity& other) const noexcept {
			return m_path == other.m_path &&
				m_extension == other.m_extension &&
				m_size == other.m_size &&
				m_modifiedTime == other.m_modifiedTime &&
				m_exif == other.m_exif;
		}

		bool operator!=(const FileIdentity& other) const noexcept {
			return !(*this == other);
		}

	private:
		const QString m_path;
		const quint64 m_size;
		const quint64 m_modifiedTime;
		const QString m_extension;
		const ExifData m_exif;
	};
}