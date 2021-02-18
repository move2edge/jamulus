#include "clientapi.h"

void CClientAPI::initReceiver()
{
  m_receiver = new ReceiveAmqpWorker(console, m_settings);
  m_receiver->moveToThread(&receiverThread);
  connect(&receiverThread, &QThread::finished, m_receiver, &QObject::deleteLater);
  connect(this, &CClientAPI::receive, m_receiver, &ReceiveAmqpWorker::receiveMessage);
  connect(m_receiver, &ReceiveAmqpWorker::resultReady, this, &CClientAPI::receiverResult);
  connect(m_receiver, &ReceiveAmqpWorker::errorEncountered, this, &CClientAPI::restartReceiver);
  receiverThread.start();
  receive("ABC");
}

void CClientAPI::restartReceiver ()
{
  sleep(3);
  receiverThread.quit();
  receiverThread.wait();
  initReceiver();
}

void CClientAPI::initSender()
{
  SendAmqpWorker* sender = new SendAmqpWorker(console, m_settings);
  sender->moveToThread(&senderThread);
  connect(&senderThread, &QThread::finished, sender, &QObject::deleteLater);
  connect(this, &CClientAPI::send, sender, &SendAmqpWorker::sendMessage);
  connect(sender, &SendAmqpWorker::sendDone, this, &CClientAPI::sendDoneResult);
  senderThread.start();
}

QString CClientAPI::getMyIpAddr()
{
  const QHostAddress& localhost = QHostAddress(QHostAddress::LocalHost);
  for (const QHostAddress& address : QNetworkInterface::allAddresses()) {
    if (address.protocol() == QAbstractSocket::IPv4Protocol && address != localhost) {
      console << "My ip" << address.toString() << endl;
      return address.toString();
    }
  }
  return "";
}

int CClientAPI::SetJitterBuffers(bool jitter_buffer_auto, int jitter_buffer_value, bool small_network_buffer)
{
  if(jitter_buffer_auto)
  {
    pClient->SetDoAutoSockBufSize (true);
  }
  else
  {
    pClient->SetDoAutoSockBufSize (false);
    pClient->SetSockBufNumFrames ( jitter_buffer_value, true );
    pClient->SetServerSockBufNumFrames ( jitter_buffer_value );
  }
  if(small_network_buffer)
  {
    pClient->SetEnableOPUS64 (true);
  }
  else
  {
    pClient->SetEnableOPUS64 (false);
  }
  return 0;
}

int CClientAPI::Connect(const QString& strSelectedAddress)
{
  if (pClient->SetServerAddr(strSelectedAddress)) {
    console << "Connecting " << strSelectedAddress << endl;
    // try to start client, if error occurred, do not go in
    // running state but show error message
    try {
      if (!pClient->IsRunning()) {
        pClient->Start();
      }
    }

    catch (CGenErr generr) {
      // show error message and return the function
      console << "Couldn't connect to " << strSelectedAddress << endl;
      return -1;
    }

    TimerPing.start(PING_UPDATE_TIME_MS);
  }
  return 0;
}

int CClientAPI::Disconnect()
{
  if (pClient->IsRunning()) {
    pClient->Stop();
  }
  TimerPing.stop();
  return 0;
}

int CClientAPI::Config(QString& ip, QString& name, int volume, bool mute, bool solo)
{
  console << "Config " << ip << " " << volume << " " << mute << " " << solo << endl;
  bool isMyGain = false;
  if(ip == m_my_ip_addr)
  {
    isMyGain=true;
  }
  for (int i = 0; i < number_active; i++)
  {
    console << "client check: " << vecpActiveChannels[i].strName << " ID: " << vecpActiveChannels[i].iChanID << endl;
    if(vecpActiveChannels[i].strName == name)
    {
      console << "APPLY" << i << endl;
      pClient->SetRemoteChanGain( i, (float)volume/100, isMyGain);
      if(mute)
      {
        pClient->SetRemoteChanGain( i, (float)0.0, isMyGain);
      }
      if(solo)
      {
        for (int j = 0; j < number_active; j++)
        {
          if( i != j)
          {
            pClient->SetRemoteChanGain( i, (float)0.0 , isMyGain);
          }
        }
        return 0;
      }
      break;
    }
  }
  return 0;
}

CClientAPI::CClientAPI(CClient* pNCliP,
    CSettings* pNSetP,
    QTextStream& tsConsole,
    const QString& strConnOnStartupAddress,
    const bool bNewShowComplRegConnList,
    const bool bShowAnalyzerConsole)
    : pClient(pNCliP)
    , console(tsConsole)
{

  m_settings = new QSettings("config.ini", QSettings::IniFormat);

  QObject::connect(&TimerPing, SIGNAL(timeout()),
      this, SLOT(OnTimerPing()));
  QObject::connect(pClient,
      SIGNAL(PingTimeReceived(int)),
      this, SLOT(OnPingTimeResult(int)));
  QObject::connect(pClient,
      SIGNAL(ConClientListMesReceived(CVector<CChannelInfo>)),
      this, SLOT(OnConClientListMesReceived(CVector<CChannelInfo>)));

  vecpActiveChannels.Init(MAX_NUM_CHANNELS);
  initReceiver();
  initSender();
  m_jsonParser = new JsonParser(tsConsole);
  m_jsonParser->m_client_name = m_settings->value("name", "").toString();
  m_my_ip_addr = getMyIpAddr();
  pClient->ChannelInfo.strName = m_settings->value("name", "").toString();
  console << "API client ready!" << endl;
}

CClientAPI::~CClientAPI()
{
  receiverThread.quit();
  receiverThread.wait();
  senderThread.quit();
  senderThread.wait();
}

void CClientAPI::sendDoneResult(const QString&)
{
  // send ping message to the server
  console << "send done!" << endl;
}

void CClientAPI::ApplyNewConClientList(CVector<CChannelInfo>& vecChanInfo)
{
  // get number of connected clients
  console << "Client list arrived" << endl;
  const int iNumConnectedClients = vecChanInfo.Size();

  // search for channels with are already present and preserve their gain
  // setting, for all other channels reset gain
  for (int i = 0; i < MAX_NUM_CHANNELS; i++) {
    for (int j = 0; j < iNumConnectedClients; j++) {
      // check if current fader is used
      if (vecChanInfo[j].iChanID == i) {
        const CHostAddress TempAddr = CHostAddress(QHostAddress(vecChanInfo[j].iIpAddr), 0);
        QString tempAddr = TempAddr.toString();
        console << "Another client name: " << i << " " << j << " "  << vecChanInfo[j].strName << " IP: " << tempAddr << " ID: " << vecChanInfo[j].iChanID << endl;
        vecpActiveChannels[i] = vecChanInfo[i];
        number_active = i+1;
      }
    }
  }
}

void CClientAPI::OnNumClientsChanged(int iNewNumClients)
{
  // update window title
  console << "Client number" << iNewNumClients << endl;
}

void CClientAPI::receiverResult(const QString& message)
{
  // send ping message to the server
  console << "Received: " << message << endl;
  QString client_name;
  QString client_ip;
  QString server_ip;
  int server_port;
  int nodes_number;
  QList<QString> ip;
  QList<QString> name;
  QList<int> volume;
  QList<bool> mute;
  QList<bool> solo;
  bool jitter_buffer_auto = 0;
  int jitter_buffer_value = 0;
  bool small_network_buffer = false;
  JsonParser::command_type cmd_type = m_jsonParser->message_get_type(message);
  switch (cmd_type) {
  case JsonParser::CMD_CONFIGURATION_UPDATED:
    console << "CMD CONFIGURATION UPDATE" << endl;
    if (m_jsonParser->config_parse(message, client_name, client_ip, nodes_number,
            ip, name, volume, mute, solo)
        == 0) {
      console << "Here " << nodes_number << endl;
      for (int i = 0; i < nodes_number; i++) {
        Config(ip[i], name[i],  volume[i], mute[i], solo[i]);
      }
    }
    break;
  case JsonParser::CMD_CONNECT:
    console << "CMD CONNECT" << endl;
    if (m_jsonParser->connect_parse(message, client_name, client_ip, server_ip, server_port,
          jitter_buffer_auto, jitter_buffer_value, small_network_buffer) == 0) {
      SetJitterBuffers(jitter_buffer_auto, jitter_buffer_value, small_network_buffer);
      Connect(server_ip + ":" + QString::number(server_port));
    }
    break;
  case JsonParser::CMD_DISCONNECT:
    console << "CMD DISCONNECT" << endl;
    if (m_jsonParser->disconnect_parse(message, client_name, client_ip) == 0) {
      Disconnect();
    }
    break;
  case JsonParser::CMD_UNKNOWN:
  default:
    console << " Unknown message type" << endl;
    break;
  };
}

void CClientAPI::OnTimerPing()
{
  // send ping message to the server
  pClient->CreateCLPingMes();
}

void CClientAPI::OnPingTimeResult(int iPingTime)
{
  // calculate overall delay
  const int iOverallDelayMs = pClient->EstimatedOverallDelay(iPingTime);
  char buffer[128];
  snprintf(buffer, 128, "%dms", iOverallDelayMs);
  console << "D: " << buffer << endl;

  QString status_message;
  m_jsonParser->m_ping = iPingTime;
  m_jsonParser->m_delay = iOverallDelayMs;
  m_jsonParser->m_client_ip = m_my_ip_addr;
  m_jsonParser->status(status_message);
  send(status_message, ".connection.status");
  // color definition: <= 43 ms green, <= 68 ms yellow, otherwise red
}
