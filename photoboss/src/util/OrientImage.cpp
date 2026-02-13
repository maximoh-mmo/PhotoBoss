#include "util/OrientImage.h"

QPixmap photoboss::OrientImage(QString path, int rotation)
{
    QPixmap pix = QPixmap(path);

    if (rotation != 1 && !pix.isNull()) {

        QTransform t;

        switch (rotation) {
        case 2: // Mirror horizontal
            t.scale(-1, 1);
            t.translate(-pix.width(), 0);
            break;

        case 3: // Rotate 180
            t.rotate(180);
            t.translate(-pix.width(), -pix.height());
            break;

        case 4: // Mirror vertical
            t.scale(1, -1);
            t.translate(0, -pix.height());
            break;

        case 5: // Mirror horizontal + rotate 90 CCW
            t.rotate(90);
            t.translate(0, -pix.height());
            t.scale(-1, 1);
            break;

        case 6: // Rotate 90 CW
            t.rotate(90);
            t.translate(0, -pix.height());
            break;

        case 7: // Mirror horizontal + rotate 90 CW
            t.rotate(-90);
            t.translate(-pix.width(), 0);
            t.scale(-1, 1);
            break;

        case 8: // Rotate 270
            t.rotate(270);
            t.translate(-pix.width(), 0);
            break;
        }
        pix = pix.transformed(t, Qt::SmoothTransformation);
    }
    return std::move(pix);
}
