#include "util/OrientImage.h"
#include <QTransform>

namespace photoboss {

    QImage OrientImage(QImage image, int rotation)
    {
        if (rotation <= 1 || image.isNull()) {
            return image;
        }

        QTransform t;
        switch (rotation) {
        case 2: // Mirror horizontal
            t.scale(-1, 1);
            break;

        case 3: // Rotate 180
            t.rotate(180);
            break;

        case 4: // Mirror vertical
            t.scale(1, -1);
            break;

        case 5: // Mirror horizontal + rotate 90 CCW (Actually Mirror + Rotate)
            t.rotate(90);
            t.scale(-1, 1);
            break;

        case 6: // Rotate 90 CW
            t.rotate(90);
            break;

        case 7: // Mirror horizontal + rotate 90 CW
            t.rotate(-90);
            t.scale(-1, 1);
            break;

        case 8: // Rotate 270
            t.rotate(270);
            break;

        default:
            return image;
        }

        return image.transformed(t, Qt::SmoothTransformation);
    }
}
