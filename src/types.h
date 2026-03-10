#ifndef SCALA_TYPES_H
#define SCALA_TYPES_H

#include <QDateTime>
#include <QJsonArray>
#include <QJsonObject>
#include <QString>
#include <QStringList>
#include <QUuid>

namespace scala {

struct CalendarEvent {
    QString id;
    QString calendarId;
    QString title;
    QDateTime startTime;
    QDateTime endTime;
    bool allDay = false;
    QString description;
    QString location;
    QStringList attendees;
    QString creatorId;
    qint64 createdAt = 0;
    qint64 updatedAt = 0;

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["id"] = id;
        obj["calendarId"] = calendarId;
        obj["title"] = title;
        obj["startTime"] = startTime.toMSecsSinceEpoch();
        obj["endTime"] = endTime.toMSecsSinceEpoch();
        obj["allDay"] = allDay;
        obj["description"] = description;
        obj["location"] = location;
        QJsonArray att;
        for (const auto &a : attendees)
            att.append(a);
        obj["attendees"] = att;
        obj["creatorId"] = creatorId;
        obj["createdAt"] = createdAt;
        obj["updatedAt"] = updatedAt;
        return obj;
    }

    static CalendarEvent fromJson(const QJsonObject &obj) {
        CalendarEvent ev;
        ev.id = obj["id"].toString();
        ev.calendarId = obj["calendarId"].toString();
        ev.title = obj["title"].toString();
        ev.startTime = QDateTime::fromMSecsSinceEpoch(obj["startTime"].toDouble());
        ev.endTime = QDateTime::fromMSecsSinceEpoch(obj["endTime"].toDouble());
        ev.allDay = obj["allDay"].toBool();
        ev.description = obj["description"].toString();
        ev.location = obj["location"].toString();
        QJsonArray att = obj["attendees"].toArray();
        for (const auto &a : att)
            ev.attendees.append(a.toString());
        ev.creatorId = obj["creatorId"].toString();
        ev.createdAt = static_cast<qint64>(obj["createdAt"].toDouble());
        ev.updatedAt = static_cast<qint64>(obj["updatedAt"].toDouble());
        return ev;
    }
};

struct Calendar {
    QString id;
    QString name;
    QString color;
    bool isShared = false;
    QString encryptionKey;
    qint64 createdAt = 0;
    qint64 updatedAt = 0;

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["id"] = id;
        obj["name"] = name;
        obj["color"] = color;
        obj["isShared"] = isShared;
        obj["encryptionKey"] = encryptionKey;
        obj["createdAt"] = createdAt;
        obj["updatedAt"] = updatedAt;
        return obj;
    }

    static Calendar fromJson(const QJsonObject &obj) {
        Calendar cal;
        cal.id = obj["id"].toString();
        cal.name = obj["name"].toString();
        cal.color = obj["color"].toString();
        cal.isShared = obj["isShared"].toBool();
        cal.encryptionKey = obj["encryptionKey"].toString();
        cal.createdAt = static_cast<qint64>(obj["createdAt"].toDouble());
        cal.updatedAt = static_cast<qint64>(obj["updatedAt"].toDouble());
        return cal;
    }
};

} // namespace scala

#endif // SCALA_TYPES_H
