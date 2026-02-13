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

        padded_square = QImage(settings::HashSampleSize, settings::HashSampleSize, QImage::Format_Grayscale8);
        padded_square.fill(Qt::black);

        // Centre the scaled image
        QPainter painter(&padded_square);
        QPoint offset((settings::HashSampleSize - scaled.width()) / 2,
            (settings::HashSampleSize - scaled.height()) / 2);
        painter.drawImage(offset, scaled);
        painter.end();

        // 4. sanity checks
        Q_ASSERT(padded_square.size() == QSize(settings::HashSampleSize, settings::HashSampleSize));
        Q_ASSERT(padded_square.format() == QImage::Format_Grayscale8);
        Q_ASSERT(padded_square.bytesPerLine() == settings::HashSampleSize);
	}
    double PerceptualImage::pixel(int x, int y) const
    {
        if (x < 0) x = 0;
        if (y < 0) y = 0;
        if (x >= settings::HashSampleSize) x = settings::HashSampleSize - 1;
        if (y >= settings::HashSampleSize) y = settings::HashSampleSize - 1;
        return static_cast<double>(padded_square.constScanLine(y)[x]);
    }
}