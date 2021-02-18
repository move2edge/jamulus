#include "jsonparser.h"

QString connection_status_template = {
  " \
  { \
    \"meta\": { \
      \"type\": \"event\", \
      \"name\": \"connectionStatus\", \
      \"createdAt\": \"2019-09-02T22:15:30Z\" \
  }, \
  \"data\": { \
    \"clientName\": 1, \
    \"clientIP\": \"192.168.0.2\", \
    \"delay\": 54, \
    \"ping\": 44 \
  } \
  "
};

QString connect_template = {
  " \
  { \
    \"meta\": { \
      \"type\": \"command\", \
      \"name\": \"connect\", \
      \"createdAt\": \"2019-09-02T22:15:30Z\" \
  }, \
  \"data\": { \
    \"clientName\": \"tom_morello\", \
    \"clientIP\": \"192.168.0.2\", \
    \"serverIP\": \"192.168.0.1\", \
    \"serverPort\": 2222 \
  } \
  } \
  "
};

JsonParser::command_type JsonParser::message_get_type(const QString& message) const
{
  QJsonDocument jsonResponse = QJsonDocument::fromJson(message.toUtf8());
  QJsonObject jsonObject = jsonResponse.object();
  QJsonObject meta_obj = jsonObject["meta"].toObject();
  QJsonDocument meta_doc(meta_obj);
  QString type_str = meta_doc["type"].toString();
  console << "Type " << type_str << endl;
  if (type_str == "command") {
    QString command_str = meta_doc["name"].toString();
    console << "Command " << command_str;
    if (command_str == "connect") {
      return CMD_CONNECT;
    } else if (command_str == "disconnect") {
      return CMD_DISCONNECT;
    } else {
      return CMD_UNKNOWN;
    }
  } else if (type_str == "event") {
    QString event_str = meta_doc["name"].toString();
    console << "Event " << event_str;
    if (event_str == "configurationUpdated") {
      return CMD_CONFIGURATION_UPDATED;
    } else {
      return CMD_UNKNOWN;
    }
  }
  return CMD_UNKNOWN;
}

int JsonParser::config_parse(const QString& message, QString& client_name, QString& client_ip, int& nodes_number,
    QList<QString>& ip, QList<QString>& name, QList<int>& volume, QList<bool>& mute, QList<bool>& solo) const
{
  QJsonDocument jsonResponse = QJsonDocument::fromJson(message.toUtf8());
  QJsonObject jsonObject = jsonResponse.object();
  QJsonObject data_obj = jsonObject["data"].toObject();
  QJsonDocument data_doc(data_obj);

  client_name = data_doc["clientName"].toString();
  if (client_name == "") {
    console << "No client name provided" << endl;
    return -1;
  }
  client_ip = data_doc["clientIP"].toString();
  if (client_ip == "") {
    console << "No client ip provided" << endl;
    return -1;
  }

  QJsonArray jsonArray = data_doc["configuration"].toArray();
  nodes_number = jsonArray.count();

  console << "Nodes number: " << nodes_number << endl;

  foreach (const QJsonValue& value, jsonArray) {
    QJsonObject obj = value.toObject();
    console << "ABC" << endl;
    console << obj["IP"].toString() << obj["clientName"].toString() << obj["volume"].toInt() << obj["mute"].toBool() << obj["solo"].toBool() << endl;
    ip.append(obj["IP"].toString());
    name.append(obj["clientName"].toString());
    volume.append(obj["volume"].toInt());
    mute.append(obj["mute"].toBool());
    solo.append(obj["solo"].toBool());
  }
  return 0;
}

int JsonParser::connect_parse(const QString& message, QString& client_name, QString& client_ip, QString& server_ip, int& server_port,
    bool& jitter_buffer_auto, int& jitter_buffer_value, bool& small_network_buffer) const
{
  QJsonDocument jsonResponse = QJsonDocument::fromJson(message.toUtf8());
  QJsonObject jsonObject = jsonResponse.object();
  QJsonObject data_obj = jsonObject["data"].toObject();
  QJsonDocument data_doc(data_obj);
  QString jitter_buffer_auto_str = "";

  client_name = data_doc["clientName"].toString();
  if (client_name == "") {
    console << "No client name provided" << endl;
    return -1;
  }
  client_ip = data_doc["clientIP"].toString();
  if (client_ip == "") {
    console << "No client ip provided" << endl;
    return -1;
  }
  server_ip = data_doc["serverIP"].toString();
  if (server_ip == "") {
    console << "No server ip provided" << endl;
    return -1;
  }
  server_port = data_doc["serverPort"].toInt();
  if (server_port == 0) {
    console << "No server port provided" << endl;
    return -1;
  }
  jitter_buffer_auto_str = data_doc["jitterBufferType"].toString();
  if (jitter_buffer_auto_str == "") {
    console << "No jitter buffer type provided" << endl;
  }
  if (jitter_buffer_auto_str == "manual")
  {
    jitter_buffer_auto = 0;
  }
  else
  {
    jitter_buffer_auto = 1;
  }
  if(jitter_buffer_auto == 0)
  {
    jitter_buffer_value = data_doc["jitterBufferManualValue"].toInt();
    if (jitter_buffer_value == 0) {
      console << "No jitter buffer size provided" << endl;
      jitter_buffer_auto = 1;
      jitter_buffer_value = 4;
    }
  }
  small_network_buffer = data_doc["smallNetworkBuffer"].toBool();
  console << client_name << " " << client_ip << " " << server_port << " " << server_ip <<
    jitter_buffer_auto << " " << jitter_buffer_value << " " << small_network_buffer << endl;
  return 0;
}

int JsonParser::disconnect_parse(const QString& message, QString& client_name, QString& client_ip) const
{
  QJsonDocument jsonResponse = QJsonDocument::fromJson(message.toUtf8());
  QJsonObject jsonObject = jsonResponse.object();
  QJsonObject data_obj = jsonObject["data"].toObject();
  QJsonDocument data_doc(data_obj);

  client_name = data_doc["clientName"].toString();
  if (client_name == "") {
    console << "No client name provided" << endl;
    return -1;
  }
  client_ip = data_doc["clientIP"].toString();
  if (client_ip == "") {
    console << "No client ip provided" << endl;
    return -1;
  }
  console << client_name << " " << client_ip << endl;
  return 0;
}

void JsonParser::status(QString& message) const
{
  QJsonObject meta;
  QJsonObject data;
  QJsonObject json;
  meta["type"] = "event";
  meta["name"] = "connectionStatus";
  QString format = "yyyy-MM-ddThh:mm:ssZ";
  meta["createdAt"] = QDateTime::currentDateTimeUtc().toString(format);
  json["meta"] = meta;
  data["clientName"] = m_client_name;
  data["clientIp"] = m_client_ip;
  data["delay"] = QString::number(m_delay);
  data["ping"] = QString::number(m_ping);
  json["data"] = data;
  QJsonDocument doc(json);
  message = doc.toJson(QJsonDocument::Indented);
  //console << message << endl;
  //json["level"] = mLevel;
  //json["classType"] = mClassType;
}

JsonParser::JsonParser(QTextStream& tsConsole)
  : console(tsConsole)
{
  console << "Json Parser inited" << endl;
}
