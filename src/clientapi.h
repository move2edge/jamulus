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
#include <QSlider>
#include <QString>
#include <QThread>
#include <QTimer>
#include <QWhatsThis>
#include <QSettings>
#include <amqpcpp.h>
#include <iostream>
#include <string>
#include <QNetworkInterface>
#include "protocol.h"
#include "jsonparser.h"
#include "amqpworker.h"

class CClientAPI : public QObject {
  Q_OBJECT
  public:
  CClientAPI(CClient* pNCliP,
      CSettings* pNSetP,
      QTextStream& tsConsole,
      const QString& strConnOnStartupAddress,
      const bool bNewShowComplRegConnList,
      const bool bShowAnalyzerConsole);
  ~CClientAPI();
  QThread receiverThread;
  QThread senderThread;
  JsonParser *m_jsonParser;
  QString m_my_ip_addr;
  CVector<CChannelInfo>  vecpActiveChannels;
  int number_active = 0;

  private:
  QSettings *m_settings;
  void initReceiver();
  void initSender();
  QString getMyIpAddr();

  protected:
  QTimer TimerPing;
  CClient* pClient;
  QTextStream& console;
  void startWorkInAThread();
  int Connect(const QString& strSelectedAddress);
  int SetJitterBuffers(bool jitter_buffer_auto, int jitter_buffer_value, bool small_network_buffer);
  int Config(QString& ip, QString& name, int volume, bool mute, bool solo);
  int Disconnect();
  void OnNumClientsChanged ( int iNewNumClients );
  ReceiveAmqpWorker* m_receiver;

  public slots:
  void OnTimerPing();
  void OnPingTimeResult(int iPingTime);
  void restartReceiver ();
  void receiverResult(const QString&);
  void sendDoneResult(const QString&);
  void ApplyNewConClientList ( CVector<CChannelInfo>& vecChanInfo );
  void OnConClientListMesReceived ( CVector<CChannelInfo> vecChanInfo )
    { ApplyNewConClientList ( vecChanInfo ); }

  signals:
  void receive(const QString&);
  void send(const QString&, const QString&);
};
