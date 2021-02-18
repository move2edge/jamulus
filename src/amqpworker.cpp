#include "amqpworker.h"

ReceiveAmqpWorker::ReceiveAmqpWorker(QTextStream& tsConsole, QSettings* settings)
  : console(tsConsole)
    , m_settings(settings)
{

  QString s_address = m_settings->value("rabbit/address", "").toString();
  QString s_user = m_settings->value("rabbit/user", "").toString();
  QString s_password = m_settings->value("rabbit/password", "").toString();
  QString s_port = m_settings->value("rabbit/port", "").toString();

  m_client_name = m_settings->value("name", "").toString();

  QString addr_buf = "amqp://" + s_user + ":" + s_password + "@" + s_address + ":" + s_port + "/";

  console << addr_buf << endl;
  m_exchange = m_settings->value("rabbit/exchange", "").toString();

  QString queue_buf = "client-" + m_client_name;
  std::string addr_buf_std = addr_buf.toUtf8().constData();
  m_queue_name = queue_buf.toUtf8().constData();
  AMQP::Address address(addr_buf_std);
  m_connection = new AMQP::TcpConnection(handler, address);
  m_channel = new AMQP::TcpChannel(m_connection);
  console << "ReceiveAmqpWorker" << endl;
}

void ReceiveAmqpWorker::receiveMessage(const QString& parameter)
{
  QString result;
  console << "Do work" << endl;
  QString binding_conf_updated = m_client_name + ".configuration.updated";
  QString binding_connect = m_client_name + ".connect";
  QString binding_disconnect = m_client_name + ".disconnect";
  QString queue = "client-" + m_client_name;
  /* ... here is the expensive or blocking operation ... */
  m_channel->onError([this](const char* message) {
      console << "Channel error on receive: " << message << endl;
      handler.Stop();
      });
  m_channel->declareQueue(m_queue_name, AMQP::durable)
    .onSuccess(
        [this](const std::string& name,
          uint32_t messagecount,
          uint32_t consumercount) {
        console << "Queue: " << QString::fromStdString(name) << endl;
        });
  m_channel->bindQueue(m_exchange.toUtf8().constData(), queue.toUtf8().constData(), binding_conf_updated.toUtf8().constData())
    .onSuccess(
        [this]() {
        console << "Queue bound " << endl;
        })
  .onFinalize(
      [this]() {
      console << "Finalize." << endl;
      });
  m_channel->bindQueue(m_exchange.toUtf8().constData(), queue.toUtf8().constData(), binding_connect.toUtf8().constData())
    .onSuccess(
        [this]() {
        console << "Queue bound " << endl;
        })
  .onFinalize(
      [this]() {
      console << "Finalize." << endl;
      });
  m_channel->bindQueue(m_exchange.toUtf8().constData(), queue.toUtf8().constData(), binding_disconnect.toUtf8().constData())
    .onSuccess(
        [this]() {
        console << "Queue bound " << endl;
        })
  .onFinalize(
      [this]() {
      console << "Finalize." << endl;
      });
  m_channel->consume(m_queue_name, AMQP::noack)
    .onReceived(
        [this](const AMQP::Message& msg, uint64_t tag, bool redelivered) {
        m_message = QString::fromStdString(msg.body());
        m_message.truncate(msg.bodySize());
        emit resultReady(m_message);
        });
  handler.Start();
  console << "Closing connection." << endl;
  m_connection->close();
  emit errorEncountered();
};


SendAmqpWorker::SendAmqpWorker(QTextStream& tsConsole, QSettings* settings)
  : console(tsConsole)
    , m_settings(settings)
{
  QString s_address = m_settings->value("rabbit/address", "").toString();
  QString s_user = m_settings->value("rabbit/user", "").toString();
  QString s_password = m_settings->value("rabbit/password", "").toString();
  QString s_port = m_settings->value("rabbit/port", "").toString();

  m_client_name = m_settings->value("name", "").toString();
  m_exchange = m_settings->value("rabbit/exchange", "").toString();

  QString addr_buf = "amqp://" + s_user + ":" + s_password + "@" + s_address + ":" + s_port + "/";
  std::string addr_buf_std = addr_buf.toUtf8().constData();

  m_address = new AMQP::Address(addr_buf_std);
  console << "SendAmqpWorker " << m_client_name << " " << addr_buf << endl;
}

void SendAmqpWorker::sendMessage(const QString& qs_message, const QString& type)
{
  std::string message = qs_message.toUtf8().constData();
  QString qs_routing_key = m_client_name + type;
  std::string routing_key = qs_routing_key.toUtf8().constData();
  auto evbase = event_base_new();
  LibEventHandlerMyError hndl(evbase);
  AMQP::TcpConnection connection(&hndl, *m_address);
  AMQP::TcpChannel channel(&connection);
  channel.onError([this, &evbase](const char* message) {
      console << "Channel error onsend: " << message << endl;
      event_base_loopbreak(evbase);
      });
  //connection.close();
  //channel.publish(m_exchange.toUtf8().constData(), routing_key, message)
    //console << "Exchange send: " << m_exchange <<  " " << qs_routing_key << endl;
  channel.declareExchange("topic", AMQP::topic)
    .onError([&](const char* msg)
        {
        console << "ERROR: " << endl;
        })
  .onSuccess
    (
     [&]()
     {
     //channel.publish("topic_logs", routing_key, msg);
      channel.publish(m_exchange.toUtf8().constData(), routing_key, message);
     //std::cout << "Sent " << routing_key << ": '"
     //<< message << "'" << std::endl;
     event_base_loopbreak(evbase);
     }
    );

  event_base_dispatch(evbase);
  event_base_free(evbase);
}

