#include "nexgen/clock/ClockWindow.h"

#include <QLabel>
#include <QTimer>
#include <QVBoxLayout>
#include <QDateTime>
#include <QTimeZone>
#include <QGuiApplication>
#include <QScreen>
#include <QPainter>
#include <QApplication>
#include <QMouseEvent>
#include <QSettings>

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
  f.setBold(true);
  m_label->setFont(f);

  syncPalette();

  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(24, 18, 24, 18);
  layout->addWidget(m_label);

  m_timer = new QTimer(this);
  m_timer->setInterval(250);
  connect(m_timer, &QTimer::timeout, this, &ClockWindow::updateTimeText);
  m_timer->start();

  // defaults
  applySizePreset();
  setOpacity(m_opacity);
  setTranslucentBackground(m_translucentBackground);

  resize(320, 140);
}

void ClockWindow::applyWindowFlags() {
  setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint);
}

void ClockWindow::setTimeZoneId(const QByteArray& tzId) {
  m_tzId = tzId;
  updateTimeText();
}

static constexpr auto kClockGroup = "Clock";
static constexpr auto kUiGroup = "Ui";

static QString sizePresetToString(ClockWindow::SizePreset p) {
  switch (p) {
    case ClockWindow::SizePreset::Tiny: return QStringLiteral("tiny");
    case ClockWindow::SizePreset::Small: return QStringLiteral("small");
    case ClockWindow::SizePreset::Default: return QStringLiteral("default");
    case ClockWindow::SizePreset::Large: return QStringLiteral("large");
    case ClockWindow::SizePreset::VeryLarge: return QStringLiteral("very_large");
  }
  return QStringLiteral("default");
}

static ClockWindow::SizePreset sizePresetFromString(const QString& s) {
  const auto v = s.trimmed().toLower();
  if (v == QStringLiteral("tiny")) return ClockWindow::SizePreset::Tiny;
  if (v == QStringLiteral("small")) return ClockWindow::SizePreset::Small;
  if (v == QStringLiteral("large")) return ClockWindow::SizePreset::Large;
  if (v == QStringLiteral("very_large") || v == QStringLiteral("verylarge") || v == QStringLiteral("very-large")) {
    return ClockWindow::SizePreset::VeryLarge;
  }
  return ClockWindow::SizePreset::Default;
}

void ClockWindow::loadUiSettings(QSettings& s) {
  // position
  s.beginGroup(kClockGroup);
  m_lastPos = s.value(QStringLiteral("pos"), QPoint()).toPoint();
  m_sizePreset = sizePresetFromString(s.value(QStringLiteral("sizePreset"), QStringLiteral("default")).toString());
  m_opacity = s.value(QStringLiteral("opacity"), 1.0).toDouble();
  m_translucentBackground = s.value(QStringLiteral("translucent"), true).toBool();
  s.endGroup();

  // clamp
  if (m_opacity < 0.2) m_opacity = 0.2;
  if (m_opacity > 1.0) m_opacity = 1.0;

  applySizePreset();
  setOpacity(m_opacity);
  setTranslucentBackground(m_translucentBackground);

  if (!m_lastPos.isNull()) {
    move(m_lastPos);
  }
}

void ClockWindow::saveUiSettings(QSettings& s) const {
  s.beginGroup(kClockGroup);
  s.setValue(QStringLiteral("pos"), pos());
  s.setValue(QStringLiteral("sizePreset"), sizePresetToString(m_sizePreset));
  s.setValue(QStringLiteral("opacity"), m_opacity);
  s.setValue(QStringLiteral("translucent"), m_translucentBackground);
  s.endGroup();
}

void ClockWindow::setSizePreset(SizePreset p) {
  if (m_sizePreset == p) return;
  m_sizePreset = p;
  applySizePreset();
}

void ClockWindow::applySizePreset() {
  if (!m_label) return;

  int pt = 64;
  int w = 320;
  int h = 140;
  int mx = 24;
  int my = 18;

  switch (m_sizePreset) {
    case SizePreset::Tiny:      pt = 28; w = 170; h =  80; mx = 14; my = 10; break;
    case SizePreset::Small:     pt = 42; w = 240; h = 110; mx = 18; my = 14; break;
    case SizePreset::Default:   pt = 64; w = 320; h = 140; mx = 24; my = 18; break;
    case SizePreset::Large:     pt = 84; w = 420; h = 175; mx = 30; my = 22; break;
    case SizePreset::VeryLarge: pt = 110;w = 540; h = 220; mx = 34; my = 26; break;
  }

  QFont f = m_label->font();
  f.setPointSize(pt);
  f.setBold(true);
  m_label->setFont(f);

  if (auto* lay = qobject_cast<QVBoxLayout*>(layout())) {
    lay->setContentsMargins(mx, my, mx, my);
  }

  resize(w, h);
  update();
}

void ClockWindow::setOpacity(qreal opacity) {
  if (opacity < 0.2) opacity = 0.2;
  if (opacity > 1.0) opacity = 1.0;
  m_opacity = opacity;
  setWindowOpacity(m_opacity);
}

void ClockWindow::setTranslucentBackground(bool on) {
  m_translucentBackground = on;
  setAttribute(Qt::WA_TranslucentBackground, on);
  update();
}

void ClockWindow::moveTo(const QPoint& p) {
  move(p);
  m_lastPos = p;
}

void ClockWindow::syncPalette() {
  // ensure we inherit the latest application palette
  setPalette(QApplication::palette());
  if (m_label) {
    QPalette lp = m_label->palette();
    lp.setColor(QPalette::WindowText, palette().color(QPalette::WindowText));
    m_label->setPalette(lp);
  }
  update();
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

void ClockWindow::refreshTheme() {
  syncPalette();
}

void ClockWindow::mousePressEvent(QMouseEvent* e) {
  if (e->button() == Qt::LeftButton) {
    m_dragging = true;
    m_dragOffset = e->globalPosition().toPoint() - frameGeometry().topLeft();
    e->accept();
    return;
  }
  QWidget::mousePressEvent(e);
}

void ClockWindow::mouseMoveEvent(QMouseEvent* e) {
  if (m_dragging && (e->buttons() & Qt::LeftButton)) {
    const QPoint p = e->globalPosition().toPoint() - m_dragOffset;
    move(p);
    m_lastPos = p;
    e->accept();
    return;
  }
  QWidget::mouseMoveEvent(e);
}

void ClockWindow::mouseReleaseEvent(QMouseEvent* e) {
  if (m_dragging && e->button() == Qt::LeftButton) {
    m_dragging = false;
    e->accept();
    return;
  }
  QWidget::mouseReleaseEvent(e);
}

void ClockWindow::toggleVisible() {
  if (isVisible()) {
    hide();
    return;
  }

  if (!m_lastPos.isNull()) {
    move(m_lastPos);
  } else {
    const QScreen* s = QGuiApplication::primaryScreen();
    const QRect avail = s ? s->availableGeometry() : QRect(0,0,1920,1080);
    const int pad = 16;
    move(avail.right() - width() - pad, avail.top() + pad);
  }

  show();
}

void ClockWindow::showEvent(QShowEvent* e) {
  QWidget::showEvent(e);
  syncPalette();
  updateTimeText();
}

void ClockWindow::paintEvent(QPaintEvent* e) {
  Q_UNUSED(e);
  QPainter p(this);
  p.setRenderHint(QPainter::Antialiasing, true);

  const QRectF r = rect();
  const qreal radius = 18.0;

  // Background from palette Window color.
  QColor bg = palette().color(QPalette::Window);
  if (!m_translucentBackground) {
    bg.setAlpha(255);
  }
  p.setPen(Qt::NoPen);
  p.setBrush(bg);
  p.drawRoundedRect(r.adjusted(0.5, 0.5, -0.5, -0.5), radius, radius);
}

} // namespace nexgen::clock
