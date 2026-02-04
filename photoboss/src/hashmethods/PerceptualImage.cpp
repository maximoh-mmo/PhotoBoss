#include "hashing/PerceptualImage.h"
#include "qpainter.h"

namespace photoboss
{
	PerceptualImage::PerceptualImage(const QImage& src)
	{
        // Scale keeping aspect ratio
        QImage scaled = src.scaled(Size, Size,
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation)
            .convertToFormat(QImage::Format_Grayscale8);

        padded_square = QImage(Size, Size, QImage::Format_Grayscale8);
        padded_square.fill(Qt::black);

        // Centre the scaled image
        QPainter painter(&padded_square);
        QPoint offset((Size - scaled.width()) / 2,
            (Size - scaled.height()) / 2);
        painter.drawImage(offset, scaled);
        painter.end();

        // 4. sanity checks
        Q_ASSERT(padded_square.size() == QSize(Size, Size));
        Q_ASSERT(padded_square.format() == QImage::Format_Grayscale8);
        Q_ASSERT(padded_square.bytesPerLine() == Size);
	}
    double PerceptualImage::pixel(int x, int y) const
    {
        if (x < 0) x = 0;
        if (y < 0) y = 0;
        if (x >= Size) x = Size - 1;
        if (y >= Size) y = Size - 1;
        return static_cast<double>(padded_square.constScanLine(y)[x]);
    }
}