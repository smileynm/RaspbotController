#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <QHostAddress>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QDebug> // 디버깅용
#include <QTimer>
#include <functional>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow) {
    ui->setupUi(this);

    m_raspbotClient = new RaspbotClient(this);
    currentMotorSpeed = 0; // 초기 속도

    m_commandTimer = new QTimer(this);
    connect(m_commandTimer, &QTimer::timeout, this, &MainWindow::processNextQueuedMotorCommand);

    // RaspbotClient의 시그널을 MainWindow의 슬롯에 연결
    connect(m_raspbotClient, &RaspbotClient::connected, this, &MainWindow::onClientConnected);
    connect(m_raspbotClient, &RaspbotClient::disconnected, this, &MainWindow::onClientDisconnected);
    connect(m_raspbotClient, &RaspbotClient::errorOccurred, this, &MainWindow::onClientError);
    connect(m_raspbotClient, &RaspbotClient::messageReceived, this, &MainWindow::onClientMessageReceived);

    // 모터 제어 버튼 pressed/released 시그널 연결
    connect(ui->forwardButton, &QPushButton::pressed, this, &MainWindow::on_forwardButton_pressed);
    connect(ui->forwardButton, &QPushButton::released, this, &MainWindow::on_forwardButton_released);
    connect(ui->backwardButton, &QPushButton::pressed, this, &MainWindow::on_backwardButton_pressed);
    connect(ui->backwardButton, &QPushButton::released, this, &MainWindow::on_backwardButton_released);
    connect(ui->leftButton, &QPushButton::pressed, this, &MainWindow::on_leftButton_pressed);
    connect(ui->leftButton, &QPushButton::released, this, &MainWindow::on_leftButton_released);
    connect(ui->rightButton, &QPushButton::pressed, this, &MainWindow::on_rightButton_pressed);
    connect(ui->rightButton, &QPushButton::released, this, &MainWindow::on_rightButton_released);

    // 속도 슬라이더 시그널 연결
    connect(ui->speedSlider, &QSlider::valueChanged, this, &MainWindow::on_speedSlider_valueChanged);

    // 초기 UI 상태 설정
    ui->hostLineEdit->setText("192.168.0.133"); // 현재 설정된 IP: 192.168.0.133
    ui->portLineEdit->setText("8080"); // 기본 포트: 8080
    ui->speedSlider->setMinimum(0);
    ui->speedSlider->setMaximum(255);
    ui->speedSlider->setValue(100); // 초기 속도 100
    on_speedSlider_valueChanged(100); // 초기 속도 라벨 업데이트
    updateConnectionStatus(false);
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::on_connectButton_clicked() {
    QString host = ui->hostLineEdit->text();
    int port = ui->portLineEdit->text().toInt();

    if (m_raspbotClient->connectToServer(host, port)) {
        ui->statusBar->showMessage(tr("서버 연결 시도 중..."), 2000);
    } else {
        QMessageBox::critical(this, tr("연결 오류"), tr("서버 연결 시도에 실패했습니다."));
    }
}

void MainWindow::on_disconnectButton_clicked() {
    m_raspbotClient->disconnectFromServer();
}

void MainWindow::onClientConnected() {
    updateConnectionStatus(true);
    ui->statusBar->showMessage(tr("서버에 연결되었습니다."), 3000);
}

void MainWindow::onClientDisconnected() {
    updateConnectionStatus(false);
    ui->statusBar->showMessage(tr("서버와 연결이 끊겼습니다."), 3000);
}

void MainWindow::onClientError(QTcpSocket::SocketError socketError) {
    updateConnectionStatus(false);
    Q_UNUSED(socketError);
    ui->statusBar->showMessage(tr("연결 오류: %1").arg(m_raspbotClient->errorString()), 5000);
    QMessageBox::critical(this, tr("연결 오류"), tr("소켓 오류 발생: %1").arg(m_raspbotClient->errorString()));
}

void MainWindow::onClientMessageReceived(const QString &message) {
    ui->logTextEdit->append(tr("서버 응답: %1").arg(message));

    QJsonParseError jsonError;
    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8(), &jsonError);

    if (jsonError.error == QJsonParseError::NoError && doc.isObject()) {
        QJsonObject obj = doc.object();
        if (obj.contains("command") && obj.value("command").toString() == "READ_ULTRASONIC" && obj.contains("distance")) {
            int distance = obj.value("distance").toInt();
            ui->logTextEdit->append(tr("-> 초음파 거리: %1 cm").arg(distance));
        }
        // 다른 센서 응답 처리
    }
}

// --- 모터 제어 슬롯 구현 ---

void MainWindow::stopAllMotors() {
    if (!m_raspbotClient->isConnected()) return;

    // 모터 명령 큐 비우고 타이머 중지
    m_motorCommandQueue.clear();


    // 모든 모터를 속도 0으로 설정하여 정지
    m_raspbotClient->controlMotor(MotorNumber::L1, MotorDirection::FORWARD, 0);
    m_raspbotClient->controlMotor(MotorNumber::L2, MotorDirection::FORWARD, 0);
    m_raspbotClient->controlMotor(MotorNumber::R1, MotorDirection::FORWARD, 0);
    m_raspbotClient->controlMotor(MotorNumber::R2, MotorDirection::FORWARD, 0);

    if (!m_commandTimer->isActive()) {
        m_commandTimer->start(10); // 10ms 딜레이 후 다음 명령 실행
        processNextQueuedMotorCommand(); // 첫 번째 명령 즉시 실행
    }

    m_commandTimer->stop();
    qDebug() << "모터 정지";
}

void MainWindow::processNextQueuedMotorCommand() {
    if (m_motorCommandQueue.isEmpty()) {
        m_commandTimer->stop();
        return;
    }

    std::function<void()> cmd = m_motorCommandQueue.takeFirst();
    cmd(); // 저장된 명령 실행
}

void MainWindow::on_forwardButton_pressed() {
    if (!m_raspbotClient->isConnected()) return;
    qDebug() << "앞으로 이동 - 속도:" << currentMotorSpeed;

    m_motorCommandQueue.clear(); // 기존 명령 큐 비우기

    // 각 모터 제어 명령을 람다 함수로 큐에 추가
    m_motorCommandQueue.append([this]() {
        m_raspbotClient->controlMotor(MotorNumber::L1, MotorDirection::FORWARD, currentMotorSpeed);
    });
    m_motorCommandQueue.append([this]() {
        m_raspbotClient->controlMotor(MotorNumber::L2, MotorDirection::FORWARD, currentMotorSpeed);
    });
    m_motorCommandQueue.append([this]() {
        m_raspbotClient->controlMotor(MotorNumber::R1, MotorDirection::FORWARD, currentMotorSpeed);
    });
    m_motorCommandQueue.append([this]() {
        m_raspbotClient->controlMotor(MotorNumber::R2, MotorDirection::FORWARD, currentMotorSpeed);
    });

    // 타이머가 동작 중이 아니면 시작
    if (!m_commandTimer->isActive()) {
        m_commandTimer->start(10); // 10ms 딜레이 후 다음 명령 실행 (적절한 값으로 조절)
        processNextQueuedMotorCommand(); // 첫 번째 명령 즉시 실행
    }

    // m_raspbotClient->controlMotor(MotorNumber::L1, MotorDirection::FORWARD, currentMotorSpeed);
    // m_raspbotClient->controlMotor(MotorNumber::L2, MotorDirection::FORWARD, currentMotorSpeed);
    // m_raspbotClient->controlMotor(MotorNumber::R1, MotorDirection::FORWARD, currentMotorSpeed);
    // m_raspbotClient->controlMotor(MotorNumber::R2, MotorDirection::FORWARD, currentMotorSpeed);
}

void MainWindow::on_forwardButton_released() {
    stopAllMotors();
}

void MainWindow::on_backwardButton_pressed() {
    if (!m_raspbotClient->isConnected()) return;

    qDebug() << "뒤로 이동 명령 큐에 추가 - 속도:" << currentMotorSpeed;

    m_motorCommandQueue.clear();

    m_motorCommandQueue.append([this]() {
        m_raspbotClient->controlMotor(MotorNumber::L1, MotorDirection::BACKWARD, currentMotorSpeed);
    });
    m_motorCommandQueue.append([this]() {
        m_raspbotClient->controlMotor(MotorNumber::L2, MotorDirection::BACKWARD, currentMotorSpeed);
    });
    m_motorCommandQueue.append([this]() {
        m_raspbotClient->controlMotor(MotorNumber::R1, MotorDirection::BACKWARD, currentMotorSpeed);
    });
    m_motorCommandQueue.append([this]() {
        m_raspbotClient->controlMotor(MotorNumber::R2, MotorDirection::BACKWARD, currentMotorSpeed);
    });

    if (!m_commandTimer->isActive()) {
        m_commandTimer->start(10);
        processNextQueuedMotorCommand();
    }

    // qDebug() << "뒤로 이동 - 속도:" << currentMotorSpeed;
    // m_raspbotClient->controlMotor(MotorNumber::L1, MotorDirection::BACKWARD, currentMotorSpeed);
    // m_raspbotClient->controlMotor(MotorNumber::L2, MotorDirection::BACKWARD, currentMotorSpeed);
    // m_raspbotClient->controlMotor(MotorNumber::R1, MotorDirection::BACKWARD, currentMotorSpeed);
    // m_raspbotClient->controlMotor(MotorNumber::R2, MotorDirection::BACKWARD, currentMotorSpeed);
}

void MainWindow::on_backwardButton_released() {
    stopAllMotors();
}

void MainWindow::on_leftButton_pressed() {
    if (!m_raspbotClient->isConnected()) return;

    qDebug() << "좌회전 명령 큐에 추가 - 속도:" << currentMotorSpeed;

    m_motorCommandQueue.clear();

    m_motorCommandQueue.append([this]() {
        m_raspbotClient->controlMotor(MotorNumber::L1, MotorDirection::BACKWARD, currentMotorSpeed);
    });
    m_motorCommandQueue.append([this]() {
        m_raspbotClient->controlMotor(MotorNumber::L2, MotorDirection::BACKWARD, currentMotorSpeed);
    });
    m_motorCommandQueue.append([this]() {
        m_raspbotClient->controlMotor(MotorNumber::R1, MotorDirection::FORWARD, currentMotorSpeed);
    });
    m_motorCommandQueue.append([this]() {
        m_raspbotClient->controlMotor(MotorNumber::R2, MotorDirection::FORWARD, currentMotorSpeed);
    });

    if (!m_commandTimer->isActive()) {
        m_commandTimer->start(10);
        processNextQueuedMotorCommand();
    }

    // qDebug() << "좌회전 - 속도:" << currentMotorSpeed;
    // // 제자리 좌회전: 왼쪽 모터 뒤로, 오른쪽 모터 앞으로
    // m_raspbotClient->controlMotor(MotorNumber::L1, MotorDirection::BACKWARD, currentMotorSpeed);
    // m_raspbotClient->controlMotor(MotorNumber::L2, MotorDirection::BACKWARD, currentMotorSpeed);
    // m_raspbotClient->controlMotor(MotorNumber::R1, MotorDirection::FORWARD, currentMotorSpeed);
    // m_raspbotClient->controlMotor(MotorNumber::R2, MotorDirection::FORWARD, currentMotorSpeed);
}

void MainWindow::on_leftButton_released() {
    stopAllMotors();
}

void MainWindow::on_rightButton_pressed() {
    if (!m_raspbotClient->isConnected()) return;

    qDebug() << "우회전 명령 큐에 추가 - 속도:" << currentMotorSpeed;

    m_motorCommandQueue.clear();

    m_motorCommandQueue.append([this]() {
        m_raspbotClient->controlMotor(MotorNumber::L1, MotorDirection::FORWARD, currentMotorSpeed);
    });
    m_motorCommandQueue.append([this]() {
        m_raspbotClient->controlMotor(MotorNumber::L2, MotorDirection::FORWARD, currentMotorSpeed);
    });
    m_motorCommandQueue.append([this]() {
        m_raspbotClient->controlMotor(MotorNumber::R1, MotorDirection::BACKWARD, currentMotorSpeed);
    });
    m_motorCommandQueue.append([this]() {
        m_raspbotClient->controlMotor(MotorNumber::R2, MotorDirection::BACKWARD, currentMotorSpeed);
    });

    if (!m_commandTimer->isActive()) {
        m_commandTimer->start(10);
        processNextQueuedMotorCommand();
    }

    // qDebug() << "우회전 - 속도:" << currentMotorSpeed;
    // // 제자리 우회전: 왼쪽 모터 앞으로, 오른쪽 모터 뒤로
    // m_raspbotClient->controlMotor(MotorNumber::L1, MotorDirection::FORWARD, currentMotorSpeed);
    // m_raspbotClient->controlMotor(MotorNumber::L2, MotorDirection::FORWARD, currentMotorSpeed);
    // m_raspbotClient->controlMotor(MotorNumber::R1, MotorDirection::BACKWARD, currentMotorSpeed);
    // m_raspbotClient->controlMotor(MotorNumber::R2, MotorDirection::BACKWARD, currentMotorSpeed);
}

void MainWindow::on_rightButton_released() {
    stopAllMotors();
}

void MainWindow::on_speedSlider_valueChanged(int value) {
    currentMotorSpeed = static_cast<unsigned char>(value);
    ui->speedLabel->setText(QString::number(value)); // 속도 라벨 업데이트
    qDebug() << "모터 속도 변경:" << currentMotorSpeed;
}

// --- 기타 제어 버튼 구현 ---
void MainWindow::on_rgbOnBtn_clicked() {
    m_raspbotClient->controlRgbAll(DeviceStatus::ON, RgbColor::RED);
}

void MainWindow::on_buzzerOnBtn_clicked() {
    m_raspbotClient->controlBuzzer(DeviceStatus::ON);
}

void MainWindow::on_requestUltrasonicBtn_clicked() {
    m_raspbotClient->requestUltrasonicDistance();
}

void MainWindow::updateConnectionStatus(bool connected) {
    ui->connectButton->setEnabled(!connected);
    ui->disconnectButton->setEnabled(connected);

    // 모터 제어 버튼 활성화/비활성화
    ui->forwardButton->setEnabled(connected);
    ui->backwardButton->setEnabled(connected);
    ui->leftButton->setEnabled(connected);
    ui->rightButton->setEnabled(connected);
    ui->speedSlider->setEnabled(connected);

    // 기타 제어 버튼들 활성화/비활성화
    //ui->rgbOnBtn->setEnabled(connected);
    //ui->buzzerOnBtn->setEnabled(connected);
    //ui->requestUltrasonicBtn->setEnabled(connected);
    // 필요한 다른 제어 버튼들도 여기에 추가
}
