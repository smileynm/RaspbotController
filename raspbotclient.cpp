#include "raspbotclient.h"
#include <QDebug>
#include <QHostAddress>

RaspbotClient::RaspbotClient(QObject *parent)
    : QObject(parent), m_socket(new QTcpSocket(this)) {
    connect(m_socket, &QTcpSocket::connected, this, &RaspbotClient::onConnected);
    connect(m_socket, &QTcpSocket::disconnected, this, &RaspbotClient::onDisconnected);
    connect(m_socket, &QTcpSocket::readyRead, this, &RaspbotClient::onReadyRead);
    connect(m_socket, QOverload<QTcpSocket::SocketError>::of(&QTcpSocket::errorOccurred),
            this, &RaspbotClient::onErrorOccurred);
}

RaspbotClient::~RaspbotClient() {
    disconnectFromServer();
}

bool RaspbotClient::connectToServer(const QString &host, int port) {
    if (m_socket->state() == QTcpSocket::ConnectedState) {
        qDebug() << "이미 서버에 연결되어 있습니다.";
        return true;
    }
    m_host = host;
    m_port = port;
    qDebug() << "서버 연결 시도:" << host << ":" << port;
    m_socket->connectToHost(host, port);
    // 연결은 비동기로 이루어지므로, 바로 연결 여부를 반환할 수 없음.
    // connected() 시그널을 통해 연결 성공 여부를 알림.
    return true; // 연결 시도 자체는 성공
}

void RaspbotClient::disconnectFromServer() {
    if (m_socket->state() == QTcpSocket::ConnectedState) {
        m_socket->disconnectFromHost();
        qDebug() << "서버에서 연결 해제 요청.";
    }
}

bool RaspbotClient::isConnected() const {
    return m_socket->state() == QTcpSocket::ConnectedState;
}

bool RaspbotClient::sendCommand(const QString &command) {
    if (!isConnected()) {
        qWarning() << "서버에 연결되어 있지 않습니다. 명령을 보낼 수 없습니다.";
        return false;
    }

    QByteArray data = command.toUtf8();
    // 줄바꿈 문자가 없을 때만 추가 (중복 방지)
    if (!data.endsWith('\n')) {
        data.append('\n');
    }

    qint64 bytesWritten = m_socket->write(data);
    if (bytesWritten == -1) {
        qWarning() << "데이터 쓰기 오류:" << m_socket->errorString();
        return false;
    }
    m_socket->flush(); // 버퍼 비우기
    qDebug() << "명령 전송:" << command;
    return true;
}

// 직접 제어 메소드 구현 (RESTful API 엔드포인트 사용)
bool RaspbotClient::controlMotor(MotorNumber motor, MotorDirection direction, int speed) {
    QString cmd = CommandBuilder::buildMotorCommand(motor, direction, speed);
    return sendCommand(cmd);
}

bool RaspbotClient::controlServo(int servoNumber, int angle) {
    QString cmd = CommandBuilder::buildServoCommand(servoNumber, angle);
    return sendCommand(cmd);
}

bool RaspbotClient::controlRgbAll(DeviceStatus status, RgbColor color) {
    QString cmd = CommandBuilder::buildRgbAllCommand(status, color);
    return sendCommand(cmd);
}

bool RaspbotClient::controlRgbIndividual(int ledNumber, DeviceStatus status, RgbColor color) {
    QString cmd = CommandBuilder::buildRgbIndividualCommand(ledNumber, status, color);
    return sendCommand(cmd);
}

bool RaspbotClient::setRgbAllBrightness(int r, int g, int b) {
    QString cmd = CommandBuilder::buildRgbAllBrightnessCommand(r, g, b);
    return sendCommand(cmd);
}

bool RaspbotClient::setRgbIndividualBrightness(int ledNumber, int r, int g, int b) {
    QString cmd = CommandBuilder::buildRgbIndividualBrightnessCommand(ledNumber, r, g, b);
    return sendCommand(cmd);
}

bool RaspbotClient::controlBuzzer(DeviceStatus status) {
    QString cmd = CommandBuilder::buildBuzzerCommand(status);
    return sendCommand(cmd);
}

bool RaspbotClient::controlUltrasonic(DeviceStatus status) {
    QString cmd = CommandBuilder::buildUltrasonicControlCommand(status);
    return sendCommand(cmd);
}

void RaspbotClient::requestUltrasonicDistance() {
    QString cmd = CommandBuilder::buildReadUltrasonicCommand();
    sendCommand(cmd);
}

void RaspbotClient::requestInfraredSensorData() {
    QString cmd = CommandBuilder::buildReadInfraredSensorCommand();
    sendCommand(cmd);
}

void RaspbotClient::requestInfraredCodeValue() {
    QString cmd = CommandBuilder::buildReadInfraredCodeCommand();
    sendCommand(cmd);
}

void RaspbotClient::onConnected() {
    qDebug() << "서버에 연결되었습니다.";
    emit connected();
}

void RaspbotClient::onDisconnected() {
    qDebug() << "서버와 연결이 끊겼습니다.";
    emit disconnected();
}

void RaspbotClient::onReadyRead() {
    m_readBuffer.append(m_socket->readAll());

    // 라인 피드('\n')를 기준으로 메시지를 처리합니다.
    while (m_readBuffer.contains('\n')) {
        int newlineIndex = m_readBuffer.indexOf('\n');
        QByteArray line = m_readBuffer.left(newlineIndex).trimmed();
        m_readBuffer.remove(0, newlineIndex + 1);

        QString response = QString::fromUtf8(line);
        qDebug() << "서버로부터 메시지 수신:" << response;
        emit messageReceived(response); // MainWindow로 전달
    }
}

void RaspbotClient::onErrorOccurred(QTcpSocket::SocketError socketError) {
    qWarning() << "소켓 오류 발생:" << socketError << "-" << m_socket->errorString();
    emit errorOccurred(socketError);
}
