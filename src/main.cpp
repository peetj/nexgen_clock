#include <QApplication>
#include <QSettings>
#include <QJsonObject>
#include <QPalette>
#include <QColor>

#include "nexgen/themes/ThemeManager.h"
#include "nexgen/sys/ipc/IpcServer.h"

#include "nexgen/clock/ClockWindow.h"

static constexpr auto kServerName = "nexgen.clock";
static inline QString serverName() { return QStringLiteral("nexgen.clock"); }

int main(int argc, char** argv) {
  QApplication app(argc, argv);
  app.setQuitOnLastWindowClosed(false);

  QSettings settings("Nexgen", "Utilities");

  // Apply shared theme
  nexgen::themes::ThemeManager themes;
  settings.sync();
  themes.load(settings);
  themes.applyTo(app);
  QObject::connect(&themes, &nexgen::themes::ThemeManager::themeChanged, [&] {
    themes.applyTo(app);
    themes.save(settings);
  });

  // Window
  nexgen::clock::ClockWindow w;
  w.loadUiSettings(settings);

  // IPC
  nexgen::sys::ipc::IpcServer server;
  server.setHandler([&](const QJsonObject& msg) -> QJsonObject {
    const QString cmd = msg.value("cmd").toString();
    if (cmd == QStringLiteral("ping")) {
      return QJsonObject{{"ok", true}, {"pong", true}};
    }
    if (cmd == QStringLiteral("toggle")) {
      w.toggleVisible();
      return QJsonObject{{"ok", true}, {"visible", w.isVisible()}};
    }
    if (cmd == QStringLiteral("show")) {
      if (!w.isVisible()) w.toggleVisible();
      return QJsonObject{{"ok", true}, {"visible", true}};
    }
    if (cmd == QStringLiteral("hide")) {
      w.hide();
      return QJsonObject{{"ok", true}, {"visible", false}};
    }
    if (cmd == QStringLiteral("setTimezone")) {
      const QString tz = msg.value("tz").toString();
      w.setTimeZoneId(tz.toUtf8());
      settings.setValue(QStringLiteral("Clock/tz"), tz);
      settings.sync();
      return QJsonObject{{"ok", true}, {"tz", tz}};
    }
    if (cmd == QStringLiteral("move")) {
      const int x = msg.value("x").toInt(0);
      const int y = msg.value("y").toInt(0);
      w.moveTo(QPoint(x, y));
      w.saveUiSettings(settings);
      settings.sync();
      return QJsonObject{{"ok", true}, {"x", x}, {"y", y}};
    }
    if (cmd == QStringLiteral("setSize")) {
      const QString preset = msg.value("preset").toString();
      const auto p = preset.toLower();
      using P = nexgen::clock::ClockWindow::SizePreset;
      if (p == QStringLiteral("tiny")) w.setSizePreset(P::Tiny);
      else if (p == QStringLiteral("small")) w.setSizePreset(P::Small);
      else if (p == QStringLiteral("large")) w.setSizePreset(P::Large);
      else if (p == QStringLiteral("very_large") || p == QStringLiteral("verylarge") || p == QStringLiteral("very-large")) w.setSizePreset(P::VeryLarge);
      else w.setSizePreset(P::Default);
      w.saveUiSettings(settings);
      settings.sync();
      return QJsonObject{{"ok", true}, {"preset", preset}};
    }
    if (cmd == QStringLiteral("setOpacity")) {
      const double v = msg.value("value").toDouble(1.0);
      w.setOpacity(v);
      w.saveUiSettings(settings);
      settings.sync();
      return QJsonObject{{"ok", true}, {"value", v}};
    }
    if (cmd == QStringLiteral("setTranslucent")) {
      const bool on = msg.value("on").toBool(true);
      w.setTranslucentBackground(on);
      w.saveUiSettings(settings);
      settings.sync();
      return QJsonObject{{"ok", true}, {"on", on}};
    }
        if (cmd == QStringLiteral("reloadTheme")) {
      // IMPORTANT: QSettings can cache values across the process lifetime.
      // When another app (tray) edits the settings, creating a fresh QSettings
      // instance here is the most reliable way to pick up external changes.
      QSettings s("Nexgen", "Utilities");
      s.sync();
      themes.load(s);
      themes.applyTo(app);
      w.refreshTheme();
      return QJsonObject{{"ok", true}};
    }
    if (cmd == QStringLiteral("getThemeDebug")) {
      QSettings s("Nexgen", "Utilities");
      s.sync();
      const int rawMode = s.value(QStringLiteral("Theme/mode"), -999).toInt();
      const QString rawThemeId = s.value(QStringLiteral("Theme/themeId"), QString()).toString();

      const QColor wcol = w.palette().color(QPalette::Window);
      const QColor tcol = w.palette().color(QPalette::WindowText);
      return QJsonObject{{"ok", true},
        {"rawMode", rawMode},
        {"rawThemeId", rawThemeId},
        {"window", QJsonObject{{"r", wcol.red()}, {"g", wcol.green()}, {"b", wcol.blue()}, {"a", wcol.alpha()}}},
        {"text", QJsonObject{{"r", tcol.red()}, {"g", tcol.green()}, {"b", tcol.blue()}, {"a", tcol.alpha()}}}
      };
    }
    if (cmd == QStringLiteral("getState")) {
      return QJsonObject{{"ok", true}, {"visible", w.isVisible()}, {"tz", QString::fromUtf8(w.timeZoneId())}};
    }
    return QJsonObject{{"ok", false}, {"error", QStringLiteral("unknown cmd")}, {"cmd", cmd}};
  });

  if (!server.listen(serverName())) {
    return 0; // already running
  }

  // restore timezone if present
  const QString tz = settings.value(QStringLiteral("Clock/tz"), QString()).toString();
  if (!tz.isEmpty()) {
    w.setTimeZoneId(tz.toUtf8());
  }

  return app.exec();
}
