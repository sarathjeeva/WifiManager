#include "utama.h"
#include "ui_utama.h"
#include <dbusnetwork.h>
#include <QDebug>
#include <QDBusInterface>
#include <QMap>
#include <QVariant>
#include <QUuid>
#include <QDBusMetaType>
#include <QDBusMessage>
#include <QMetaObject>

typedef  QMap<QString, QMap<QString, QVariant> > NMVariantMap;

Q_DECLARE_METATYPE(NMVariantMap);

Utama::Utama(QWidget *parent) :
  QMainWindow(parent),
  ui(new Ui::Utama)
{
  ui->setupUi(this);

  debus = new DbusNetwork();
  wifi = new DbusWifi();
  this->programInit();
//  qDebug() << debus->getDevices();
//  qDebug()<<debus->getActiveConnection();
//  qDebug()<<debus->getStatus();
//  qDebug()<<debus->getDeviceType("/org/freedesktop/NetworkManager/Devices/0");
}

void Utama::programInit(){
  int status = debus->getStatus();

  if(status < 3){
    // No active connection / not connected
    ui->labelIndicator->setStyleSheet("QLabel#labelIndicator{background-color: #F44336; border-radius: 15px}");
    ui->statusLabel->setText("Disconnected..");
  }else{
    // Connected.
    ui->labelIndicator->setStyleSheet("QLabel#labelIndicator{background-color: #4CAF50; border-radius: 15px}");
    ui->statusLabel->setText("You are connected to the Network");
  }

  // Get wifi devices
  QStringList devices = debus->getDevices();
  QString wifiDev = "";
  foreach(QString device, devices){
    if(debus->getDeviceType(device) == 2){
      qDebug()<<"INFO : Get wifi devices on "+device;
      wifiDev = device;
      break;
    }
  }

  // Get scanned wifi SSID
  wifi->setDevice(wifiDev);
  QStringList listAp = wifi->getListAP();
  QStringList listSSID;
  foreach(QString ap, listAp){
    QString resolved = wifi->resolveSSID(ap);
    listSSID.append(resolved);
    ssids.insert(resolved, ap);
    // Tampilkan ke UI
    ui->listAccessPoint->addItem(resolved);
  }
  qDebug()<<listSSID;

  qDebug()<<listAp;

  QString active_ap = wifi->getActiveAP();
  qDebug() << "<<<<<<<: " << wifi->resolveSSID(active_ap);
  QString label = "You are connected to network: ";
  label.append(wifi->resolveSSID(active_ap));
  ui->statusLabel->setText(label);

  wifi->getAvailableConnectios();
  wifi->getConnections();

//  debus->getConnections();
}

Utama::~Utama()
{
  delete ui;
  delete debus;
  delete wifi;
}

void Utama::on_pushButton_clicked()
{
    int i = ui->listAccessPoint->currentRow();
    QString ssid = ui->listAccessPoint->item(i)->text();
    qDebug() << " :::: ::: :: ### " << ssid;

    QString apoint;
    QStringList listAp = wifi->getListAP();
    foreach(QString ap, listAp){
      QString resolved = wifi->resolveSSID(ap);
      if(resolved == ssid) {
        apoint = ap;
        break;
      }
    }
    qDebug() << apoint;

    QString nmDBus = "org.freedesktop.NetworkManager";

    QDBusInterface iface(nmDBus,
                         "/org/freedesktop/NetworkManager/Settings",
                         "org.freedesktop.NetworkManager.Settings",
                         QDBusConnection::systemBus());
  if(iface.isValid()) {
    QMap<QString, QVariant> connection;
    connection.insert("type", "802-11-wireless");
    connection.insert("uuid", QUuid::createUuid().toString().remove('{').remove('}')); // The UUID of the new connection
    connection.insert("id", "%%Walo this is the name of the connection%%"); // A name fo the connection

    QMap<QString, QVariant> wifi;
    wifi.insert("ssid", ssid.toLatin1()); // This is the name of the SSID --> must be a QByteArray
    wifi.insert("mode", "infrastructure");

    QMap<QString, QVariant> sec;
    sec.insert("key-mgmt", "wpa-psk");
    sec.insert("auth-alg", "open");
    sec.insert("psk", "qwertyuiop15"); // Here we specify the password of the wifi

    QMap<QString, QVariant> ip4;
    ip4["method"] = "auto";

    QMap<QString, QVariant> ip6;
    ip6["method"] = "ignore";

    QMap<QString, QMap<QString, QVariant> > map;
    map["connection"] = connection;
    map["802-11-wireless"] = wifi;
    map["802-11-wireless-security"] = sec;
    map["ipv4"] = ip4;
    map["ipv6"] = ip6;



    qRegisterMetaType<NMVariantMap>();
    qDBusRegisterMetaType<NMVariantMap>();
//    qRegisterMetaType<QMap<QString, QMap<QString, QVariant> > >();

    QVariant v = QVariant::fromValue(map);

    QDBusMessage query = iface.call("AddConnection", v);
    if(query.type() == QDBusMessage::ReplyMessage) {
      qDebug() << " %%%%%%%%%%%%%%%%%%%%";
    }
    else {
      qDebug() << query.errorMessage();
    }
  }
}