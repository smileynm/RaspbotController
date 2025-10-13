#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "raspbotclient.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_connectButton_clicked();
    void on_disconnectButton_clicked();

    // -- 모터 제어 슬롯 --
    void on_forwardButton_pressed();
    void on_forwardButton_released();
    void on_backwardButton_pressed();
    void on_backwardButton_released();
    void on_leftButton_pressed();
    void on_leftButton_released();
    void on_rightButton_pressed();
    void on_rightButton_released();

    void on_speedSlider_valueChanged(int value); // 속도 슬라이더 변경 시

    // -- 기타 제어 버튼 --
    void on_rgbOnBtn_clicked();
    void on_buzzerOnBtn_clicked();
    void on_requestUltrasonicBtn_clicked();

    // RaspbotClient 시그널을 받는 슬롯
    void onClientConnected();
    void onClientDisconnected();
    void onClientError(QTcpSocket::SocketError socketError);
    void onClientMessageReceived(const QString &message);

private:
    Ui::MainWindow *ui;
    RaspbotClient *m_raspbotClient;
    void updateConnectionStatus(bool connected); // 연결 상태에 따라 UI 활성화/비활성화
    void stopAllMotors(); // 모든 모터를 정지시키는 헬퍼 함수
    unsigned char currentMotorSpeed; // 현재 설정된 모터 속도

    QList<std::function<void()>> m_motorCommandQueue; // 실행할 모터 명령 큐
    QTimer *m_commandTimer; // 명령 실행 간 딜레이를 위한 타이머
    void processNextQueuedMotorCommand(); // 큐에서 다음 명령 처리 함수
};
#endif // MAINWINDOW_H
