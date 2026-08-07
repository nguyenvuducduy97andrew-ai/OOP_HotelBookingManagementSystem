#pragma once

#include <QWidget>
#include <QStringList>

class QLabel;
class QPushButton;
class QResizeEvent;
class QPixmap;
class QTimer;

class RoomImageCarousel final : public QWidget {
public:
    explicit RoomImageCarousel(int imageHeight, QWidget* parent = nullptr);
    void setGallery(const QString& roomTypeName, int imageCount);

protected:
    void resizeEvent(QResizeEvent* event) override;

private:
    void showPreviousImage();
    void showNextImage();
    void scheduleRefresh(int delayMilliseconds);
    void refreshImages();
    QPixmap coverPixmap(const QString& resourcePath, const QSize& targetSize) const;

    QStringList m_imagePaths;
    QString m_galleryName;
    QString m_lastRenderSignature;
    int m_currentIndex = 0;
    QLabel* m_previousPreview = nullptr;
    QLabel* m_currentImage = nullptr;
    QLabel* m_nextPreview = nullptr;
    QLabel* m_positionLabel = nullptr;
    QPushButton* m_previousButton = nullptr;
    QPushButton* m_nextButton = nullptr;
    QTimer* m_refreshTimer = nullptr;
};
