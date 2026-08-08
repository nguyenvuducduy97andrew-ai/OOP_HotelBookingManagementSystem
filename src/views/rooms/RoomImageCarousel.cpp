#include "RoomImageCarousel.h"

#include <QGraphicsOpacityEffect>
#include <QHBoxLayout>
#include <QImage>
#include <QImageReader>
#include <QLabel>
#include <QPixmap>
#include <QPixmapCache>
#include <QPushButton>
#include <QResizeEvent>
#include <QTimer>
#include <QVBoxLayout>

namespace {
QStringList roomImageResources(const QString& roomTypeName, int imageCount)
{
    QStringList images;
    const QString resourceKey = roomTypeName.trimmed().toLower();
    // Modified: Present a Deluxe bedroom as the gallery hero instead of opening on a bathroom photo.
    QList<int> displayOrder;
    if (resourceKey == "deluxe" && imageCount >= 3) {
        displayOrder.append(3);
    }
    for (int index = 1; index <= imageCount; ++index) {
        if (!displayOrder.contains(index)) {
            displayOrder.append(index);
        }
    }
    for (const int index : displayOrder) {
        images.append(QString(":/room_images/%1/%1_%2.jpg")
                          .arg(resourceKey)
                          .arg(index, 2, 10, QLatin1Char('0')));
    }
    return images;
}
}

RoomImageCarousel::RoomImageCarousel(int imageHeight, QWidget* parent)
    : QWidget(parent)
{
    setObjectName("roomImageCarousel");
    // Modified: Reserve the complete image-and-counter height so a parent layout cannot place room specifications over the gallery.
    setFixedHeight(imageHeight + 20);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(2);

    auto* imageRow = new QHBoxLayout();
    imageRow->setContentsMargins(0, 0, 0, 0);
    imageRow->setSpacing(5);

    m_previousButton = new QPushButton("<", this);
    m_nextButton = new QPushButton(">", this);
    for (QPushButton* button : {m_previousButton, m_nextButton}) {
        button->setFixedSize(28, 38);
        button->setCursor(Qt::PointingHandCursor);
        button->setFocusPolicy(Qt::NoFocus);
        button->setStyleSheet(
            "QPushButton { background:#FFFFFF; color:#475569; border:1px solid #E2E8F0; border-radius:8px; font-size:16px; font-weight:700; padding:0px; }"
            "QPushButton:hover { background:#F8FAFC; color:#005BFE; border-color:#CBD5E1; }"
            "QPushButton:pressed { background:#F1F5F9; }");
    }
    m_previousButton->setToolTip("Previous room image");
    m_nextButton->setToolTip("Next room image");

    m_previousPreview = new QLabel(this);
    m_currentImage = new QLabel(this);
    m_nextPreview = new QLabel(this);
    const int previewWidth = qBound(34, imageHeight / 3, 48);
    m_previousPreview->setFixedSize(previewWidth, imageHeight - 8);
    m_nextPreview->setFixedSize(previewWidth, imageHeight - 8);
    m_currentImage->setMinimumWidth(170);
    m_currentImage->setFixedHeight(imageHeight);
    m_currentImage->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    for (QLabel* image : {m_previousPreview, m_currentImage, m_nextPreview}) {
        image->setAlignment(Qt::AlignCenter);
        image->setStyleSheet("background:#F1F5F9; border:1px solid #E2E8F0; border-radius:10px;");
    }
    auto* previousOpacity = new QGraphicsOpacityEffect(m_previousPreview);
    previousOpacity->setOpacity(0.34);
    m_previousPreview->setGraphicsEffect(previousOpacity);
    auto* nextOpacity = new QGraphicsOpacityEffect(m_nextPreview);
    nextOpacity->setOpacity(0.34);
    m_nextPreview->setGraphicsEffect(nextOpacity);

    imageRow->addWidget(m_previousButton, 0, Qt::AlignVCenter);
    imageRow->addWidget(m_previousPreview, 0, Qt::AlignVCenter);
    imageRow->addWidget(m_currentImage, 1);
    imageRow->addWidget(m_nextPreview, 0, Qt::AlignVCenter);
    imageRow->addWidget(m_nextButton, 0, Qt::AlignVCenter);

    m_positionLabel = new QLabel(this);
    m_positionLabel->setFixedHeight(16);
    m_positionLabel->setAlignment(Qt::AlignCenter);
    m_positionLabel->setStyleSheet("color:#6B7FA8; font-size:11px; font-weight:700; background:transparent; border:none;");

    rootLayout->addLayout(imageRow);
    rootLayout->addWidget(m_positionLabel);

    connect(m_previousButton, &QPushButton::clicked, this, [this]() { showPreviousImage(); });
    connect(m_nextButton, &QPushButton::clicked, this, [this]() { showNextImage(); });

    m_refreshTimer = new QTimer(this);
    m_refreshTimer->setSingleShot(true);
    connect(m_refreshTimer, &QTimer::timeout, this, [this]() { refreshImages(); });
}

void RoomImageCarousel::setGallery(const QString& roomTypeName, int imageCount)
{
    const QString normalizedName = roomTypeName.trimmed();
    if (normalizedName.isEmpty() || imageCount <= 0) {
        m_galleryName.clear();
        m_imagePaths.clear();
        m_lastRenderSignature.clear();
        m_currentImage->clear();
        m_previousPreview->clear();
        m_nextPreview->clear();
        m_positionLabel->clear();
        return;
    }
    if (m_galleryName.compare(normalizedName, Qt::CaseInsensitive) == 0
        && m_imagePaths.size() == imageCount) {
        scheduleRefresh(0);
        return;
    }

    // Modified: Reuse one lazy gallery for each room type so Standard and Suite images are never loaded together unnecessarily.
    m_galleryName = normalizedName;
    m_imagePaths = roomImageResources(normalizedName, imageCount);
    m_currentIndex = 0;
    m_lastRenderSignature.clear();
    scheduleRefresh(0);
}

void RoomImageCarousel::showPreviousImage()
{
    m_currentIndex = (m_currentIndex - 1 + m_imagePaths.size()) % m_imagePaths.size();
    m_lastRenderSignature.clear();
    scheduleRefresh(0);
}

void RoomImageCarousel::showNextImage()
{
    m_currentIndex = (m_currentIndex + 1) % m_imagePaths.size();
    m_lastRenderSignature.clear();
    scheduleRefresh(0);
}

void RoomImageCarousel::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    // Modified: Debounce layout-driven resize bursts so high-quality image scaling occurs once after the geometry settles.
    scheduleRefresh(100);
}

void RoomImageCarousel::scheduleRefresh(int delayMilliseconds)
{
    if (m_refreshTimer) {
        m_refreshTimer->start(qMax(0, delayMilliseconds));
    }
}

void RoomImageCarousel::refreshImages()
{
    if (m_imagePaths.isEmpty() || m_currentImage->width() <= 0) {
        return;
    }
    const int previousIndex = (m_currentIndex - 1 + m_imagePaths.size()) % m_imagePaths.size();
    const int nextIndex = (m_currentIndex + 1) % m_imagePaths.size();
    const QString renderSignature = QString("%1:%2:%3x%4:%5x%6")
                                        .arg(m_galleryName)
                                        .arg(m_currentIndex)
                                        .arg(m_currentImage->width())
                                        .arg(m_currentImage->height())
                                        .arg(m_previousPreview->width())
                                        .arg(m_previousPreview->height());
    if (renderSignature == m_lastRenderSignature) {
        return;
    }

    // Modified: Render only once for an unchanged gallery/index/geometry combination and reuse cached display-sized pixmaps.
    const QPixmap currentPixmap = coverPixmap(m_imagePaths.at(m_currentIndex), m_currentImage->size());
    if (currentPixmap.isNull()) {
        return;
    }
    m_currentImage->setPixmap(currentPixmap);
    m_previousPreview->setPixmap(coverPixmap(m_imagePaths.at(previousIndex), m_previousPreview->size()));
    m_nextPreview->setPixmap(coverPixmap(m_imagePaths.at(nextIndex), m_nextPreview->size()));
    m_positionLabel->setText(QString("%1 gallery  •  Image %2 of %3")
                                 .arg(m_galleryName)
                                 .arg(m_currentIndex + 1)
                                 .arg(m_imagePaths.size()));
    m_lastRenderSignature = renderSignature;
}

QPixmap RoomImageCarousel::coverPixmap(const QString& resourcePath, const QSize& targetSize) const
{
    if (!targetSize.isValid()) {
        return {};
    }
    const QString cacheKey = QString("room-carousel:%1:%2x%3")
        .arg(resourcePath).arg(targetSize.width()).arg(targetSize.height());
    QPixmap result;
    if (QPixmapCache::find(cacheKey, &result)) {
        return result;
    }

    QImageReader reader(resourcePath);
    reader.setAutoTransform(true);
    const QSize sourceSize = reader.size();
    if (sourceSize.isValid()) {
        QSize decodeSize = sourceSize;
        decodeSize.scale(targetSize, Qt::KeepAspectRatioByExpanding);
        if (decodeSize.isValid() && decodeSize.width() < sourceSize.width()) {
            reader.setScaledSize(decodeSize);
        }
    }

    // Modified: Decode JPEG resources directly near their display size instead of expanding every source to its full-resolution bitmap on the UI thread.
    QImage decodedImage = reader.read();
    if (decodedImage.isNull()) {
        return {};
    }
    QPixmap scaled = QPixmap::fromImage(std::move(decodedImage));
    if (scaled.width() < targetSize.width() || scaled.height() < targetSize.height()) {
        scaled = scaled.scaled(targetSize, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    }
    const int cropX = qMax(0, (scaled.width() - targetSize.width()) / 2);
    const int cropY = qMax(0, (scaled.height() - targetSize.height()) / 2);
    result = scaled.copy(cropX, cropY, targetSize.width(), targetSize.height());
    QPixmapCache::insert(cacheKey, result);
    return result;
}
