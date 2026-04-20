#include "hashing/PerceptualImage.h"
#include "util/AppSettings.h"
#include "qpainter.h"

namespace photoboss
{
	PerceptualImage::PerceptualImage(const QImage& src)
	{
        // Scale keeping aspect ratio
        QImage scaled = src.scaled(settings::HashSampleSize, settings::HashSampleSize,
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation)
            .convertToFormat(QImage::Format_Grayscale8);

        m_paddedSquare_ = QImage(settings::HashSampleSize, settings::HashSampleSize, QImage::Format_Grayscale8);
        m_paddedSquare_.fill(Qt::black);

        // Centre the scaled image
        QPainter painter(&m_paddedSquare_);
        QPoint offset((settings::HashSampleSize - scaled.width()) / 2,
            (settings::HashSampleSize - scaled.height()) / 2);
        painter.drawImage(offset, scaled);
        painter.end();

        // 4. sanity checks
        Q_ASSERT(m_paddedSquare_.size() == QSize(settings::HashSampleSize, settings::HashSampleSize));
        Q_ASSERT(m_paddedSquare_.format() == QImage::Format_Grayscale8);
        Q_ASSERT(m_paddedSquare_.bytesPerLine() == settings::HashSampleSize);
	}
    double PerceptualImage::pixel(int x, int y) const
    {
        if (x < 0) x = 0;
        if (y < 0) y = 0;
        if (x >= settings::HashSampleSize) x = settings::HashSampleSize - 1;
        if (y >= settings::HashSampleSize) y = settings::HashSampleSize - 1;
        return static_cast<double>(m_paddedSquare_.constScanLine(y)[x]);
    }
}