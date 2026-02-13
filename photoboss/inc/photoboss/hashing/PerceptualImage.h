#pragma once
#include <QImage>

namespace photoboss {

    class PerceptualImage
    {
    public:
        PerceptualImage() = delete;

        explicit PerceptualImage(const QImage& src);
        const QImage& image() const { return padded_square; }
        const uchar* bits() const { return padded_square.constBits(); }
        int bytesPerLine() const { return padded_square.bytesPerLine(); }
        double pixel(int x, int y) const;
    private:
        QImage padded_square;
    };

} // namespace photoboss