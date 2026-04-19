#include <QApplication>
#include <QSettings>
#include <QJsonObject>

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
  themes.load(settings);
  themes.applyTo(app);
  QObject::connect(&themes, &nexgen::themes::ThemeManager::themeChanged, [&] {
    themes.applyTo(app);
    themes.save(settings);
  });

  // Window
  nexgen::clock::ClockWindow w;

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
      return QJsonObject{{"ok", true}, {"tz", tz}};
    }
        if (cmd == QStringLiteral("reloadTheme")) {
      themes.load(settings);
      themes.applyTo(app);
      return QJsonObject{{"ok", true}};
    }
    if (cmd == QStringLiteral("getState")) {
      return QJsonObject{{"ok", true}, {"visible", w.isVisible()}, {"tz", QString::fromUtf8(w.timeZoneId())}};
    }
    return QJsonObject{{"ok", false}, {"error", QStringLiteral("unknown cmd")}, {"cmd", cmd}};
  });

  if (!server.listen(serverName())) {
    return 0; // already running
  }

  return app.exec();
}
