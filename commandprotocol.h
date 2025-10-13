#ifndef COMMANDPROTOCOL_H
#define COMMANDPROTOCOL_H

#include <QString>
#include <QJsonObject>
#include <QJsonDocument>

/**
 * 클라이언트가 서버에 전송할 JSON 명령어를 만드는 헬퍼 클래스로,
 * 서버 RESTful API의 엔드포인트 경로와 JSON 키에 맞춰 명령어를 생성합니다.
 */

enum class MotorNumber {
    L1 = 0x00,
    L2 = 0x01,
    R1 = 0x02,
    R2 = 0x03
};

enum class MotorDirection {
    FORWARD = 0x00,
    BACKWARD = 0x01
};

enum class DeviceStatus {
    OFF = 0x00,
    ON = 0x01
};

enum class RgbColor {
    RED = 0x0,
    GREEN,
    BLUE,
    YELLOW,
    PURPLE,
    INDIGO,
    WHITE,
    OFF
};

class CommandBuilder {
public:

    // 모터 제어 (/motor 엔드포인트)
    static QString buildMotorCommand(MotorNumber motor_number, MotorDirection direction, int speed) {
        QJsonObject cmd;
        cmd["endpoint"] = "/motor";                  // 반드시 서버가 기대하는 endpoint 문자열
        cmd["motor_number"] = static_cast<int>(motor_number);
        cmd["direction"] = static_cast<int>(direction);
        cmd["speed"] = speed;
        return QJsonDocument(cmd).toJson(QJsonDocument::Compact);
    }

    // 서보 제어 (/servo 엔드포인트)
    static QString buildServoCommand(int servo_number, int angle) {
        QJsonObject cmd;
        cmd["endpoint"] = "/servo";
        cmd["servo_number"] = servo_number;
        cmd["angle"] = angle;
        return QJsonDocument(cmd).toJson(QJsonDocument::Compact);
    }

    // 전체 RGB 제어 (/rgb/all 엔드포인트)
    static QString buildRgbAllCommand(DeviceStatus status, RgbColor color) {
        QJsonObject cmd;
        cmd["endpoint"] = "/rgb/all";
        cmd["status"] = static_cast<int>(status);
        cmd["color"] = static_cast<int>(color);
        return QJsonDocument(cmd).toJson(QJsonDocument::Compact);
    }

    // 개별 RGB 제어 (/rgb/individual 엔드포인트)
    static QString buildRgbIndividualCommand(int led_number, DeviceStatus status, RgbColor color) {
        QJsonObject cmd;
        cmd["endpoint"] = "/rgb/individual";
        cmd["led_number"] = led_number;
        cmd["status"] = static_cast<int>(status);
        cmd["color"] = static_cast<int>(color);
        return QJsonDocument(cmd).toJson(QJsonDocument::Compact);
    }

    // 전체 RGB 밝기 제어 (/rgb/brightness/all 엔드포인트)
    static QString buildRgbAllBrightnessCommand(int r, int g, int b) {
        QJsonObject cmd;
        cmd["endpoint"] = "/rgb/brightness/all";
        cmd["r"] = r;
        cmd["g"] = g;
        cmd["b"] = b;
        return QJsonDocument(cmd).toJson(QJsonDocument::Compact);
    }

    // 개별 RGB 밝기 제어 (/rgb/brightness/individual 엔드포인트)
    static QString buildRgbIndividualBrightnessCommand(int led_number, int r, int g, int b) {
        QJsonObject cmd;
        cmd["endpoint"] = "/rgb/brightness/individual";
        cmd["led_number"] = led_number;
        cmd["r"] = r;
        cmd["g"] = g;
        cmd["b"] = b;
        return QJsonDocument(cmd).toJson(QJsonDocument::Compact);
    }

    // 부저 제어 (/buzzer 엔드포인트)
    static QString buildBuzzerCommand(DeviceStatus status) {
        QJsonObject cmd;
        cmd["endpoint"] = "/buzzer";
        cmd["status"] = static_cast<int>(status);
        return QJsonDocument(cmd).toJson(QJsonDocument::Compact);
    }

    // 초음파 센서 제어 (/ultrasonic 엔드포인트)
    static QString buildUltrasonicControlCommand(DeviceStatus status) {
        QJsonObject cmd;
        cmd["endpoint"] = "/ultrasonic";
        cmd["status"] = static_cast<int>(status);
        return QJsonDocument(cmd).toJson(QJsonDocument::Compact);
    }

    // 초음파 거리 읽기 (/ultrasonic/read 엔드포인트)
    static QString buildReadUltrasonicCommand() {
        QJsonObject cmd;
        cmd["endpoint"] = "/ultrasonic/read";
        return QJsonDocument(cmd).toJson(QJsonDocument::Compact);
    }

    // 적외선 센서 읽기 (/ir/sensor 엔드포인트)
    static QString buildReadInfraredSensorCommand() {
        QJsonObject cmd;
        cmd["endpoint"] = "/ir/sensor";
        return QJsonDocument(cmd).toJson(QJsonDocument::Compact);
    }

    // 적외선 코드 읽기 (/ir/code 엔드포인트)
    static QString buildReadInfraredCodeCommand() {
        QJsonObject cmd;
        cmd["endpoint"] = "/ir/code";
        return QJsonDocument(cmd).toJson(QJsonDocument::Compact);
    }
};

#endif // COMMANDPROTOCOL_H
