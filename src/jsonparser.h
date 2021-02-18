#include "analyzerconsole.h"
#include "audiomixerboard.h"
#include "client.h"
#include "global.h"
#include "multicolorled.h"
#include "settings.h"
#include "ui_clientdlgbase.h"
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QMenuBar>
#include <QProgressBar>
#include <QPushButton>
#include <QRadioButton>
#include <QSettings>
#include <QSlider>
#include <QString>
#include <QThread>
#include <QTimer>
#include <QWhatsThis>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QDateTime>
#include <iostream>
#include <string>

class JsonParser : public QObject {
  Q_OBJECT
  public:
  enum command_type { CMD_CONNECT, CMD_DISCONNECT, CMD_CONFIGURATION_UPDATED, CMD_UNKNOWN };
  command_type message_get_type(const QString& message) const;
  JsonParser(QTextStream& tsConsole);
  QTextStream& console;
  void status(QString& message) const;
  int connect_parse(const QString& message, QString& client_name, QString& client_ip, QString& server_ip, int& server_port,
          bool& jitter_buffer_auto, int& jitter_buffer_value, bool& small_network_buffer) const;
  int disconnect_parse(const QString& message, QString& client_name, QString& client_ip) const;
  int config_parse(const QString& message, QString& client_name, QString& client_ip, int& nodes_number, QList<QString>& ip, QList<QString>& name, QList<int>& volume, QList<bool>& mute, QList<bool>& solo) const;

  QString m_client_name;
  QString m_client_ip;
  int m_delay;
  int m_ping;
  protected:

  public slots:
  signals:
};
