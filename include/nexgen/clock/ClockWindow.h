#pragma once

#include <QWidget>

class QLabel;
class QTimer;

namespace nexgen::clock {

class ClockWindow final : public QWidget {
  Q_OBJECT

public:
  explicit ClockWindow(QWidget* parent = nullptr);

  void setTimeZoneId(const QByteArray& tzId);
  QByteArray timeZoneId() const { return m_tzId; }

public slots:
  void toggleVisible();
  void refreshTheme();

protected:
  void showEvent(QShowEvent* e) override;
  void paintEvent(QPaintEvent* e) override;

private:

  void syncPalette();

  QLabel* m_label = nullptr;
  QTimer* m_timer = nullptr;
  QByteArray m_tzId; // IANA id, empty => local

  void updateTimeText();
  void applyWindowFlags();
};

} // namespace nexgen::clock
