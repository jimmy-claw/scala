#pragma once

#include <QString>

/**
 * QrGenerator — Generates a data: URL with a QR-like representation of text.
 *
 * Without an external QR library, this returns a data: URL containing a simple
 * SVG placeholder that displays the link text. When a real QR library becomes
 * available, swap the implementation to render an actual QR code as PNG.
 */
class QrGenerator {
public:
    /// Returns a data: URL (SVG) encoding the given text.
    /// The SVG shows the text in a bordered box as a placeholder for a real QR code.
    static QString generateQrDataUrl(const QString &text);
};
