#include "qr_generator.h"

#include <QByteArray>

QString QrGenerator::generateQrDataUrl(const QString &text) {
    if (text.isEmpty())
        return {};

    // TODO: Replace with actual QR code rendering when a QR library is available.
    // For now, generate a simple SVG placeholder that shows the link text
    // in a bordered box with a "QR" label.

    // Escape XML special characters in the text for safe SVG embedding
    QString escaped = text;
    escaped.replace(QLatin1Char('&'), QStringLiteral("&amp;"));
    escaped.replace(QLatin1Char('<'), QStringLiteral("&lt;"));
    escaped.replace(QLatin1Char('>'), QStringLiteral("&gt;"));
    escaped.replace(QLatin1Char('"'), QStringLiteral("&quot;"));

    // Truncate display text if too long for the SVG box
    QString displayText = escaped;
    if (displayText.length() > 60)
        displayText = displayText.left(57) + QStringLiteral("...");

    QString svg = QStringLiteral(
        "<svg xmlns='http://www.w3.org/2000/svg' width='200' height='200' viewBox='0 0 200 200'>"
        "<rect x='2' y='2' width='196' height='196' rx='8' fill='white' stroke='#333' stroke-width='2'/>"
        "<text x='100' y='40' text-anchor='middle' font-size='16' font-weight='bold' fill='#333'>QR Code</text>"
        "<text x='100' y='60' text-anchor='middle' font-size='10' fill='#999'>(placeholder)</text>"
        "<rect x='30' y='75' width='140' height='100' rx='4' fill='#f5f5f5' stroke='#ccc' stroke-width='1'/>"
        "<foreignObject x='35' y='80' width='130' height='90'>"
        "<p xmlns='http://www.w3.org/1999/xhtml' style='font-size:9px;word-break:break-all;color:#555;margin:0;'>"
        "%1"
        "</p>"
        "</foreignObject>"
        "</svg>"
    ).arg(displayText);

    QByteArray svgBytes = svg.toUtf8();
    QByteArray base64 = svgBytes.toBase64();

    return QStringLiteral("data:image/svg+xml;base64,") + QString::fromLatin1(base64);
}
