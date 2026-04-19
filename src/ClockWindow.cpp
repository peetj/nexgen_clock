#include "nexgen/clock/ClockWindow.h"

#include <QLabel>
#include <QTimer>
#include <QVBoxLayout>
#include <QDateTime>
#include <QTimeZone>
#include <QGuiApplication>
#include <QScreen>

namespace nexgen::clock {

ClockWindow::ClockWindow(QWidget* parent) : QWidget(parent) {
  setObjectName(QStringLiteral("ClockWindow"));

  applyWindowFlags();

  setAttribute(Qt::WA_TranslucentBackground, true);
  setAttribute(Qt::WA_ShowWithoutActivating, true);

  m_label = new QLabel(this);
  m_label->setAlignment(Qt::AlignCenter);
  m_label->setText(QStringLiteral("--:--"));

  QFont f = m_label->font();
  f.setPointSize(64);
  f.setBold(true);
  m_label->setFont(f);

  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(24, 18, 24, 18);
  layout->addWidget(m_label);

  // Temporary styling. We'll replace with proper theme tokens from themeset.
  setStyleSheet(QStringLiteral(R"(
    #ClockWindow {
      border-radius: 18px;
      background: rgba(20,20,20,180);
    }
    QLabel { color: white; }
  )"));

  m_timer = new QTimer(this);
  m_timer->setInterval(250);
  connect(m_timer, &QTimer::timeout, this, &ClockWindow::updateTimeText);
  m_timer->start();

  resize(320, 140);
}

void ClockWindow::applyWindowFlags() {
  setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint);
}

void ClockWindow::setTimeZoneId(const QByteArray& tzId) {
  m_tzId = tzId;
  updateTimeText();
}

void ClockWindow::updateTimeText() {
  QDateTime now;
  if (!m_tzId.isEmpty()) {
    const QTimeZone tz(m_tzId);
    now = QDateTime::currentDateTime(tz);
  } else {
    now = QDateTime::currentDateTime();
  }
  m_label->setText(now.toString(QStringLiteral("HH:mm:ss")));
}

void ClockWindow::toggleVisible() {
  if (isVisible()) {
    hide();
    return;
  }

  const QScreen* s = QGuiApplication::primaryScreen();
  const QRect avail = s ? s->availableGeometry() : QRect(0,0,1920,1080);
  const int pad = 16;
  move(avail.right() - width() - pad, avail.top() + pad);

  show();
}

void ClockWindow::showEvent(QShowEvent* e) {
  QWidget::showEvent(e);
  updateTimeText();
}

} // namespace nexgen::clock
