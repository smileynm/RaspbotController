#ifndef RASPBOTCLIENT_H
#define RASPBOTCLIENT_H

#include <QObject>
#include <QTcpSocket>
#include <QString>
#include <QJsonDocument>
#include <QJsonObject>
#include "commandprotocol.h"

class RaspbotClient : public QObject {
    Q_OBJECT

public:
    explicit RaspbotClient(QObject *parent = nullptr);
    ~RaspbotClient();

    bool connectToServer(const QString &host, int port);
    void disconnectFromServer();
    bool isConnected() const;
    QString errorString() const { return m_socket->errorString(); }

    // 명령어 전송 메소드
    bool sendCommand(const QString &command);

    // 직접 제어 메소드들 (CommandBuilder를 활용)
    bool controlMotor(MotorNumber motor, MotorDirection direction, int speed);
    bool controlServo(int servoNumber, int angle); // 1-2, 0-180도
    bool controlRgbAll(DeviceStatus status, RgbColor color);
    bool controlRgbIndividual(int ledNumber, DeviceStatus status, RgbColor color); // 1-14
    bool setRgbAllBrightness(int r, int g, int b);
    bool setRgbIndividualBrightness(int ledNumber, int r, int g, int b); // 1-14
    bool controlBuzzer(DeviceStatus status);
    bool controlUltrasonic(DeviceStatus status);

    // 센서 데이터 요청 및 수신 처리
    void requestUltrasonicDistance();
    void requestInfraredSensorData();
    void requestInfraredCodeValue();
    void requestKeyData();

signals:
    void connected();
    void disconnected();
    void errorOccurred(QTcpSocket::SocketError socketError);
    void messageReceived(const QString &message); // 서버 응답 메시지

private slots:
    void onConnected();
    void onDisconnected();
    void onReadyRead();
    void onErrorOccurred(QTcpSocket::SocketError socketError);

private:
    QTcpSocket *m_socket;
    QString m_host;
    int m_port;
    QByteArray m_readBuffer; // 수신 데이터 버퍼
};

#endif // RASPBOTCLIENT_H
