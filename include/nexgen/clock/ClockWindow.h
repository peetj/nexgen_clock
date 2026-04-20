#pragma once

#include <QWidget>

#include <QPoint>

class QLabel;
class QTimer;
class QSettings;

namespace nexgen::clock {

class ClockWindow final : public QWidget {
  Q_OBJECT

public:
  enum class SizePreset {
    Tiny,
    Small,
    Default,
    Large,
    VeryLarge
  };

  explicit ClockWindow(QWidget* parent = nullptr);

  void setTimeZoneId(const QByteArray& tzId);
  QByteArray timeZoneId() const { return m_tzId; }

  // Settings
  void loadUiSettings(QSettings& s);
  void saveUiSettings(QSettings& s) const;

  void setSizePreset(SizePreset p);
  SizePreset sizePreset() const { return m_sizePreset; }

  // 0.2 .. 1.0 (window opacity)
  void setOpacity(qreal opacity);
  qreal opacity() const { return m_opacity; }

  // Background translucency toggle (affects paint alpha)
  void setTranslucentBackground(bool on);
  bool translucentBackground() const { return m_translucentBackground; }

  // Move anywhere (IPC can call this; user can also drag the window)
  void moveTo(const QPoint& p);

public slots:
  void toggleVisible();
  void refreshTheme();

protected:
  void showEvent(QShowEvent* e) override;
  void paintEvent(QPaintEvent* e) override;

  void mousePressEvent(QMouseEvent* e) override;
  void mouseMoveEvent(QMouseEvent* e) override;
  void mouseReleaseEvent(QMouseEvent* e) override;

private:
  void syncPalette();
  void applySizePreset();

  QLabel* m_label = nullptr;
  QTimer* m_timer = nullptr;
  QByteArray m_tzId; // IANA id, empty => local

  // UI state
  SizePreset m_sizePreset = SizePreset::Default;
  qreal m_opacity = 1.0;
  bool m_translucentBackground = true;
  QPoint m_lastPos{0, 0};

  // drag move
  bool m_dragging = false;
  QPoint m_dragOffset{0, 0};

  void updateTimeText();
  void applyWindowFlags();
};

} // namespace nexgen::clock
