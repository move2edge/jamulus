#include "analyzerconsole.h"
#include "audiomixerboard.h"
#include "client.h"
#include "global.h"
#include "multicolorled.h"
#include "ui_clientdlgbase.h"
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QMenuBar>
#include <QProgressBar>
#include <QPushButton>
#include <QRadioButton>
#include <QSlider>
#include <QString>
#include <QThread>
#include <QTimer>
#include <QSettings>
#include <amqpcpp.h>
#include <iostream>
#include <string>
#include <QNetworkInterface>
#include "conn_handler.h"

/* Classes ********************************************************************/
class ReceiveAmqpWorker : public QObject {
  Q_OBJECT
  public:
  ReceiveAmqpWorker(QTextStream& tsConsole, QSettings *settings);
  QString m_message;
  AMQP::TcpChannel *m_channel;
  AMQP::TcpConnection *m_connection;

  protected:
  QTextStream& console;
  QSettings *m_settings;
  QString m_client_name;
  ConnHandler handler;
  std::string m_queue_name;
  QString m_exchange;

  public slots:
  void receiveMessage(const QString& parameter);
  signals:
  void resultReady(const QString& result);
  void errorEncountered();
};

class SendAmqpWorker : public QObject {
  Q_OBJECT
  public:
  SendAmqpWorker(QTextStream& tsConsole, QSettings *settings);

  protected:
  QTextStream& console;
  QSettings *m_settings;
  QString m_client_name;
  QString m_exchange;
  AMQP::Address *m_address;

  public slots:
  void sendMessage(const QString& parameter, const QString& type);
  signals:
  void sendDone(const QString& result);
};
