#pragma once
#include <QImage>

namespace photoboss {

    class PerceptualImage
    {
    public:
        PerceptualImage() = delete;

        explicit PerceptualImage(const QImage& src);
        const QImage& image() const { return m_paddedSquare_; }
        const uchar* bits() const { return m_paddedSquare_.constBits(); }
        int bytesPerLine() const { return m_paddedSquare_.bytesPerLine(); }
        double pixel(int x, int y) const;
    private:
        QImage m_paddedSquare_;
    };

} // namespace photoboss