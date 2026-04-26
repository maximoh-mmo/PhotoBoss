#include "util/StorageInfo.h"
#include <QFile>
#include <QFileInfo>

#if defined(Q_OS_LINUX)
#include <sys/stat.h>
#include <unistd.h>
#endif

#if defined(Q_OS_WIN)
#include <windows.h>
#include <winioctl.h>

#define PHOTOBOSS_DEVICE_SEEK_PENALTY_PROPERTY 7
#endif

namespace photoboss {

#if defined(Q_OS_LINUX)

bool StorageInfo::isFastStorage(const QString& path)
{
    QStorageInfo storageInfo(path);
    if (!storageInfo.isValid()) {
        return false;
    }

    QString device = storageInfo.device();

    if (device.startsWith("/dev/mapper/") || device.startsWith("/dev/dm-")) {
        return false;
    }

    if (device.contains("/dev/sd")) {
        QString baseDevice = device.mid(0, device.indexOf("/"));
        QString rotationalPath = "/sys/block/" + baseDevice.mid(5) + "/queue/rotational";
        QFile f(rotationalPath);
        if (f.open(QIODevice::ReadOnly) && f.readAll().trimmed() == "0") {
            return true;
        }
        return false;
    }

    if (device.contains("/dev/nvme")) {
        return true;
    }

    return false;
}

#endif

#if defined(Q_OS_WIN)

static bool getPhysicalDiskNumber(const QString& driveLetter, DWORD& diskNumber)
{
    QString volumePath = QLatin1String("\\\\.\\") + driveLetter;
    HANDLE hVolume = CreateFileA(
        volumePath.toLocal8Bit().constData(),
        0,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr,
        OPEN_EXISTING,
        0,
        nullptr
    );

    if (hVolume == INVALID_HANDLE_VALUE) {
        return false;
    }

    VOLUME_DISK_EXTENTS extents;
    DWORD bytesReturned = 0;

    BOOL result = DeviceIoControl(
        hVolume,
        IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,
        nullptr,
        0,
        &extents,
        sizeof(extents),
        &bytesReturned,
        nullptr
    );

    CloseHandle(hVolume);

    if (!result || extents.NumberOfDiskExtents == 0) {
        return false;
    }

    diskNumber = extents.Extents[0].DiskNumber;
    return true;
}

bool StorageInfo::isFastStorage(const QString& path)
{
    QStorageInfo storageInfo(path);
    if (!storageInfo.isValid()) {
        return false;
    }

    QString rootPath = storageInfo.rootPath();
    if (rootPath.length() >= 2 && rootPath[1] == QLatin1Char(':')) {
        QString driveLetter = rootPath.left(1).toUpper();

        DWORD diskNumber = 0;
        if (!getPhysicalDiskNumber(driveLetter, diskNumber)) {
            return false;
        }

        QString physDrivePath = QLatin1String("\\\\.\\PhysicalDrive") + QString::number(diskNumber);
        HANDLE hFile = CreateFileA(
            physDrivePath.toLocal8Bit().constData(),
            0,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            nullptr,
            OPEN_EXISTING,
            0,
            nullptr
        );

        if (hFile == INVALID_HANDLE_VALUE) {
            return false;
        }

        STORAGE_PROPERTY_QUERY query;
        memset(&query, 0, sizeof(query));
        query.PropertyId = static_cast<STORAGE_PROPERTY_ID>(PHOTOBOSS_DEVICE_SEEK_PENALTY_PROPERTY);
        query.QueryType = PropertyStandardQuery;

        DEVICE_SEEK_PENALTY_DESCRIPTOR output;
        memset(&output, 0, sizeof(output));
        DWORD bytesReturned = 0;

        BOOL result = DeviceIoControl(
            hFile,
            IOCTL_STORAGE_QUERY_PROPERTY,
            &query,
            sizeof(query),
            &output,
            sizeof(output),
            &bytesReturned,
            nullptr
        );

        CloseHandle(hFile);

        if (result && output.Size >= sizeof(DEVICE_SEEK_PENALTY_DESCRIPTOR)) {
            return output.IncursSeekPenalty == FALSE;
        }

        return false;
    }

    return false;
}

#endif

}