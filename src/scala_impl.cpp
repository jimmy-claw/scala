#include "scala_impl.h"

#include <chrono>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <QObject>
#include <QUrl>
#include <QUrlQuery>

// nlohmann/json must be included before transport header (templated on it)
#include <nlohmann/json.hpp>

// Transport integration — include AFTER nlohmann and generated types
#include "logos_transport.hpp"

// ── Minimal JSON helpers (replace QJsonDocument for universal pattern) ───────
// TODO: Replace with nlohmann/json or LogosMap/LogosList from logos_json.h
// when available. For now, pass JSON strings through — the wire format is
// identical to what the old Qt code produced.

static std::string jsonEscape(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 16);
    for (char c : s) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:   out += c; break;
        }
    }
    return out;
}

static std::string generateUuid() {
    // Simple UUID v4 generator (no Qt QUuid dependency)
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (int i = 0; i < 32; i++) {
        unsigned char b = static_cast<unsigned char>(std::rand() % 256);
        if (i == 8 || i == 13 || i == 18 || i == 23)
            oss << '-';
        // Version 4 at position 14-15
        if (i == 14) b = (b & 0x0f) | 0x40;
        // Variant at position 19
        if (i == 19) b = (b & 0x3f) | 0x80;
        oss << std::setw(2) << static_cast<int>(b);
    }
    return oss.str();
}

static int64_t currentMs() {
    auto tp = std::chrono::system_clock::now().time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(tp).count();
}

static std::string generateStableIdentityImpl() {
    // Try to read /etc/machine-id for a stable identity
    std::ifstream idFile("/etc/machine-id");
    std::string machineId;
    if (idFile.is_open()) {
        std::getline(idFile, machineId);
    }
    if (machineId.empty()) {
        machineId = "scala-fallback-id";
    }

    // SHA-256 hash of machine-id
    // TODO: Replace with proper crypto when available in universal pattern
    // For now, use a simple hash that's stable per-machine
    unsigned char hash[32];
    // Fallback: just use the machine-id hex-encoded if no crypto available
    std::ostringstream oss;
    for (char c : machineId) {
        oss << std::hex << std::setw(2) << std::setfill('0')
            << static_cast<int>(static_cast<unsigned char>(c));
    }
    return oss.str();
}

// ── Forward declarations for Qt-based components (adapted later) ──────────────
// These are the existing CalendarStore/CalendarSync classes that still use Qt.
// The universal pattern requires pure C++, so we wrap them here with type
// conversion at the boundary. Full port to std types is a follow-up task.

#include "calendar_store.h"
#include "calendar_sync.h"
#include "types.h"
#include "qr_generator.h"

// ── Construction / Destruction ───────────────────────────────────────────────

ScalaImpl::ScalaImpl() {
    m_store = new CalendarStore();
    m_sync = new CalendarSync();  // no parent in universal pattern

    // Connect sync message handler (pure C++ callback, no Qt signals)
    m_sync->setMessageHandler(
        [this](const std::string& calendarId, const SyncMessage& msg) {
            onSyncMessageReceived(calendarId, msg.toBytes());
        });
    m_sync->setStatusHandler(
        [this](const std::string& calendarId, const std::string& status) {
            syncStatusChanged(calendarId, status);
        });
}

ScalaImpl::~ScalaImpl() {
    delete m_sync;
    delete m_store;
}

// ── Context lifecycle ────────────────────────────────────────────────────────

void ScalaImpl::onContextReady() {
    // Called when the module context is fully initialized.
    // At this point, modules().delivery_module is available for inter-module calls.

    // Load stored identity or generate stable fallback
    std::string stored = m_store->kvGet("identity").toStdString();
    if (!stored.empty()) {
        m_identity = stored;
    } else {
        m_identity = generateStableIdentityImpl();
        m_store->kvSet("identity", QString::fromStdString(m_identity));
    }

    // ── Wire transport (logos-transport integration) ──────────────────────
    using LogosMap = nlohmann::json;
    using Tx = logos_transport::Transport<LogosMap>;

    // Build Ops — thin wrappers around modules().delivery_module.*
    // Delivery module uses SYNC methods; we adapt them to the transport's callback API
    Tx::Ops ops;
    ops.createNode = [this](const std::string& cfg, Tx::Cb cb) {
        auto r = modules().delivery_module.createNode(cfg);
        cb(r.success, r.error);
    };
    ops.start = [this](Tx::Cb cb) {
        auto r = modules().delivery_module.start();
        cb(r.success, r.error);
    };
    ops.subscribe = [this](const std::string& topic, Tx::Cb cb) {
        auto r = modules().delivery_module.subscribe(topic);
        cb(r.success, r.error);
    };
    ops.channelCreate = [this](const std::string& id, const std::string& ct, const std::string& sid, Tx::Cb cb) {
        auto r = modules().delivery_module.channelCreate(id, ct, sid);
        cb(r.success, r.error);
    };
    ops.channelSend = [this](const std::string& id, const LogosMap& payload, Tx::Cb cb) {
        // Convert LogosMap to byte vector for channelSend
        std::string data = payload.dump();
        std::vector<uint8_t> bytes(data.begin(), data.end());
        auto r = modules().delivery_module.channelSend(id, bytes);
        cb(r.success, r.error);
    };

    // Receive handlers — connect delivery_module events to transport callbacks
    // The builder generates typed event subscription methods from logos_events:
    ops.onMessage = [this](Tx::RecvCb handler) {
        modules().delivery_module.onMessageReceived(
            [handler](const std::string& hash, const std::string& topic,
                      const std::vector<uint8_t>& payload, int64_t ts) {
                std::string s(payload.begin(), payload.end());
                LogosMap p = LogosMap::parse(s, nullptr, false);
                if (p.is_discarded()) p = s;
                handler(topic, p);
            });
    };
    ops.onChannelMessage = [this](Tx::RecvCb handler) {
        modules().delivery_module.onChannelMessageReceived(
            [handler](const std::string& channelId, const std::string& senderId,
                      const std::vector<uint8_t>& payload, int64_t ts) {
                std::string s(payload.begin(), payload.end());
                LogosMap p = LogosMap::parse(s, nullptr, false);
                if (p.is_discarded()) p = s;
                handler(channelId, p);
            });
    };

    // Construct transport
    std::string deviceId = m_identity.empty() ? "scala-default" : m_identity;
    Tx tx(std::move(ops),
          Tx::Config{
              .logLevel = "INFO",
              .preset = "logos.dev",
              .entryNodes = {},  // empty on GUI host
              .useChannels = true,
              .hubMode = false,
              .deviceId = deviceId
          },
          [this]{ std::vector<std::string> topics;
                  auto calIds = m_store->listCalendars();
                  for (const auto& cal : calIds) {
                      if (cal.isShared)
                          topics.push_back(CalendarSync::topicForCalendar(cal.id.toStdString()));
                  }
                  return topics; },
          [this](const std::string& topic, const std::string& sealedOnceDecoded) {
              m_sync->handleReceive(topic, sealedOnceDecoded);
          });

    m_sync->setTransport(std::move(tx));
    m_sync->bootstrap();
}

// ── Namespace API ────────────────────────────────────────────────────────────

void ScalaImpl::setNamespace(const std::string& ns) {
    m_namespace = ns;
    m_store->setNamespace(QString::fromStdString(ns));
}

// ── Identity API ─────────────────────────────────────────────────────────────

std::string ScalaImpl::getIdentity() const {
    return m_identity;
}

void ScalaImpl::setIdentity(const std::string& pubkeyHex) {
    if (m_identity != pubkeyHex) {
        m_identity = pubkeyHex;
        m_store->kvSet("identity", QString::fromStdString(m_identity));
        identityChanged();  // emit event (generated by logos_events:)
    }
}

// ── Calendar CRUD ────────────────────────────────────────────────────────────

std::string ScalaImpl::createCalendar(const std::string& name, const std::string& color) {
    scala::Calendar cal;
    cal.id = QString::fromStdString(generateUuid());
    cal.name = QString::fromStdString(name);
    cal.color = QString::fromStdString(color);
    cal.creatorId = QString::fromStdString(m_identity);
    cal.isShared = false;
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    cal.createdAt = now;
    cal.updatedAt = now;

    m_store->saveCalendar(cal);
    return cal.id.toStdString();
}

std::string ScalaImpl::listCalendars() {
    auto calendars = m_store->listCalendars();
    QJsonArray arr;
    for (const auto& cal : calendars)
        arr.append(cal.toJson());
    return QString::fromUtf8(QJsonDocument(arr).toJson(QJsonDocument::Compact)).toStdString();
}

bool ScalaImpl::deleteCalendar(const std::string& id) {
    // Stop sync if running before deleting
    if (m_sync->isSyncing(id))
        m_sync->stopSync(id);

    return m_store->deleteCalendar(QString::fromStdString(id));
}

// ── Event CRUD ───────────────────────────────────────────────────────────────

std::string ScalaImpl::createEvent(const std::string& calendarId, const std::string& eventJson) {
    QJsonDocument doc = QJsonDocument::fromJson(QString::fromStdString(eventJson).toUtf8());
    QJsonObject obj = doc.object();

    scala::CalendarEvent ev = scala::CalendarEvent::fromJson(obj);
    ev.id = QString::fromStdString(generateUuid());
    ev.calendarId = QString::fromStdString(calendarId);
    ev.creatorId = QString::fromStdString(m_identity);
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    ev.createdAt = now;
    ev.updatedAt = now;

    m_store->saveEvent(ev);

    // Broadcast to peers if calendar is syncing
    if (m_sync->isSyncing(ev.calendarId.toStdString())) {
        SyncMessage msg;
        msg.type = SyncMessageType::CreateEvent;
        msg.calendarId = ev.calendarId.toStdString();
        msg.senderId = m_identity;
        msg.payload = QString::fromUtf8(
            QJsonDocument(ev.toJson()).toJson(QJsonDocument::Compact)).toStdString();
        msg.timestamp = now;
        m_sync->sendMessage(ev.calendarId.toStdString(), msg);
    }

    return ev.id.toStdString();
}

std::string ScalaImpl::updateEvent(const std::string& eventJson) {
    QJsonDocument doc = QJsonDocument::fromJson(QString::fromStdString(eventJson).toUtf8());
    QJsonObject obj = doc.object();

    scala::CalendarEvent ev = scala::CalendarEvent::fromJson(obj);

    // Ownership check: only creator can update
    auto existing = m_store->getEvent(ev.id);
    if (!existing.id.isEmpty() && !existing.creatorId.isEmpty()
        && existing.creatorId != QString::fromStdString(m_identity)) {
        QJsonObject err;
        err["error"] = "not_authorized";
        return QString::fromUtf8(QJsonDocument(err).toJson(QJsonDocument::Compact)).toStdString();
    }

    ev.creatorId = existing.creatorId;
    ev.updatedAt = QDateTime::currentMSecsSinceEpoch();

    bool ok = m_store->updateEvent(ev);
    if (ok) {
        // Broadcast to peers if calendar is syncing
        if (m_sync->isSyncing(ev.calendarId.toStdString())) {
            SyncMessage msg;
            msg.type = SyncMessageType::UpdateEvent;
            msg.calendarId = ev.calendarId.toStdString();
            msg.senderId = m_identity;
            msg.payload = QString::fromUtf8(
                QJsonDocument(ev.toJson()).toJson(QJsonDocument::Compact)).toStdString();
            msg.timestamp = ev.updatedAt;
            m_sync->sendMessage(ev.calendarId.toStdString(), msg);
        }
        return ev.id.toStdString();
    }
    return {};
}

bool ScalaImpl::deleteEvent(const std::string& id) {
    auto ev = m_store->getEvent(QString::fromStdString(id));

    // Ownership check: only creator can delete
    if (!ev.id.isEmpty() && !ev.creatorId.isEmpty()
        && ev.creatorId != QString::fromStdString(m_identity)) {
        return false;  // not authorized
    }

    return m_store->deleteEvent(QString::fromStdString(id));
}

std::string ScalaImpl::listEvents(const std::string& calendarId) {
    auto events = m_store->listEvents(QString::fromStdString(calendarId));
    QJsonArray arr;
    for (const auto& ev : events)
        arr.append(ev.toJson());
    return QString::fromUtf8(QJsonDocument(arr).toJson(QJsonDocument::Compact)).toStdString();
}

std::string ScalaImpl::getEvent(const std::string& id) {
    auto ev = m_store->getEvent(QString::fromStdString(id));
    if (ev.id.isEmpty())
        return {};
    return QString::fromUtf8(
        QJsonDocument(ev.toJson()).toJson(QJsonDocument::Compact).toStdString();
}

// ── Sync API ─────────────────────────────────────────────────────────────────

std::string ScalaImpl::shareCalendar(const std::string& calendarId) {
    auto cal = m_store->getCalendar(QString::fromStdString(calendarId));
    if (cal.id.isEmpty())
        return {};

    // Generate encryption key if not already shared
    if (cal.encryptionKey.isEmpty()) {
        cal.encryptionKey = QString::fromStdString(generateUuid())
                          + QString::fromStdString(generateUuid());
    }
    cal.isShared = true;
    cal.updatedAt = QDateTime::currentMSecsSinceEpoch();
    m_store->saveCalendar(cal);

    m_sync->startSync(cal.id.toStdString(), cal.encryptionKey.toStdString());
    return cal.encryptionKey.toStdString();
}

bool ScalaImpl::joinSharedCalendar(const std::string& calendarId,
                                   const std::string& encryptionKey) {
    if (calendarId.empty() || encryptionKey.empty())
        return false;

    QString qCalId = QString::fromStdString(calendarId);
    QString qKey = QString::fromStdString(encryptionKey);

    // Create local calendar entry if it doesn't exist
    auto cal = m_store->getCalendar(qCalId);
    if (cal.id.isEmpty()) {
        cal.id = qCalId;
        cal.name = "Shared Calendar";
        cal.color = "#9C27B0";
        cal.isShared = true;
        cal.encryptionKey = qKey;
        qint64 now = QDateTime::currentMSecsSinceEpoch();
        cal.createdAt = now;
        cal.updatedAt = now;
        m_store->saveCalendar(cal);
    } else {
        cal.isShared = true;
        cal.encryptionKey = qKey;
        cal.updatedAt = QDateTime::currentMSecsSinceEpoch();
        m_store->saveCalendar(cal);
    }

    m_sync->startSync(qCalId.toStdString(), qKey.toStdString());

    // Request bulk sync from peers
    SyncMessage msg;
    msg.type = SyncMessageType::SyncEvents;
    msg.calendarId = qCalId.toStdString();
    msg.senderId = m_identity;
    msg.timestamp = QDateTime::currentMSecsSinceEpoch();
    m_sync->sendMessage(qCalId.toStdString(), msg);

    return true;
}

std::string ScalaImpl::getSyncStatus(const std::string& calendarId) {
    auto cal = m_store->getCalendar(QString::fromStdString(calendarId));
    if (cal.id.isEmpty() || !cal.isShared)
        return "not_shared";

    if (m_sync->isSyncing(cal.id.toStdString()))
        return "syncing";

    return "offline";
}

// ── Share link API ───────────────────────────────────────────────────────────

std::string ScalaImpl::generateShareLink(const std::string& calendarId) {
    auto cal = m_store->getCalendar(QString::fromStdString(calendarId));
    if (cal.id.isEmpty())
        return {};

    // Ensure calendar is shared (generates key if needed)
    if (!cal.isShared || cal.encryptionKey.isEmpty())
        shareCalendar(calendarId);

    // Re-read after shareCalendar may have updated the key
    cal = m_store->getCalendar(cal.id);
    if (cal.encryptionKey.isEmpty())
        return {};

    QByteArray base64Key = cal.encryptionKey.toUtf8().toBase64(
        QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);

    QUrl url;
    url.setScheme("scala");
    url.setHost("join");

    QUrlQuery query;
    query.addQueryItem("id", cal.id);
    query.addQueryItem("key", QString::fromLatin1(base64Key));
    query.addQueryItem("name", cal.name);
    url.setQuery(query);

    return url.toString().toStdString();
}

std::string ScalaImpl::parseShareLink(const std::string& link) {
    QUrl url(QString::fromStdString(link));
    if (!url.isValid() || url.scheme() != "scala" || url.host() != "join")
        return {};

    QUrlQuery query(url);
    QString id = query.queryItemValue("id");
    QString base64Key = query.queryItemValue("key");
    QString name = query.queryItemValue("name");

    if (id.isEmpty() || base64Key.isEmpty())
        return {};

    QByteArray key = QByteArray::fromBase64(base64Key.toLatin1(),
                                            QByteArray::Base64UrlEncoding
                                            | QByteArray::OmitTrailingEquals);

    QJsonObject result;
    result["id"] = id;
    result["key"] = QString::fromUtf8(key);
    result["name"] = name;

    return QString::fromUtf8(QJsonDocument(result).toJson(QJsonDocument::Compact)).toStdString();
}

bool ScalaImpl::handleShareLink(const std::string& link) {
    std::string parsed = parseShareLink(link);
    if (parsed.empty())
        return false;

    QJsonDocument doc = QJsonDocument::fromJson(QString::fromStdString(parsed).toUtf8());
    QJsonObject obj = doc.object();

    QString id = obj["id"].toString();
    QString key = obj["key"].toString();
    QString name = obj["name"].toString();

    if (id.isEmpty() || key.isEmpty())
        return false;

    // If calendar doesn't exist yet, create with the shared name
    auto cal = m_store->getCalendar(id);
    if (cal.id.isEmpty() && !name.isEmpty()) {
        cal.id = id;
        cal.name = name;
        cal.color = "#9C27B0";
        cal.isShared = true;
        cal.encryptionKey = key;
        qint64 now = QDateTime::currentMSecsSinceEpoch();
        cal.createdAt = now;
        cal.updatedAt = now;
        m_store->saveCalendar(cal);
    }

    return joinSharedCalendar(id.toStdString(), key.toStdString());
}

// ── Search API ───────────────────────────────────────────────────────────────

std::string ScalaImpl::searchEvents(const std::string& query) {
    if (query.empty())
        return "[]";

    QString qQuery = QString::fromStdString(query).trimmed();
    if (qQuery.isEmpty())
        return "[]";

    QString lowerQuery = qQuery.toLower();
    QJsonArray results;

    auto calendars = m_store->listCalendars();
    for (const auto& cal : calendars) {
        auto events = m_store->listEvents(cal.id);
        for (const auto& ev : events) {
            if (ev.title.toLower().contains(lowerQuery)
                || ev.description.toLower().contains(lowerQuery)
                || ev.location.toLower().contains(lowerQuery)) {
                QJsonObject obj = ev.toJson();
                obj["calendarName"] = cal.name;
                obj["calendarColor"] = cal.color;
                results.append(obj);
            }
        }
    }

    return QString::fromUtf8(QJsonDocument(results).toJson(QJsonDocument::Compact)).toStdString();
}

// ── Reminders API ────────────────────────────────────────────────────────────

std::string ScalaImpl::getPendingReminders() {
    QJsonArray pending;
    QDateTime now = QDateTime::currentDateTime();

    auto calendars = m_store->listCalendars();
    for (const auto& cal : calendars) {
        auto events = m_store->listEvents(cal.id);
        for (const auto& ev : events) {
            if (ev.reminderMinutes < 0)
                continue;

            // Check if already reminded
            QString reminderKey = "reminder:" + ev.id;
            if (m_store->kvGet(reminderKey) == "fired")
                continue;

            // Check if event starts within the reminder window
            qint64 msBefore = static_cast<qint64>(ev.reminderMinutes) * 60 * 1000;
            QDateTime reminderTime = ev.startTime.addMSecs(-msBefore);
            if (now >= reminderTime && now < ev.startTime) {
                QJsonObject obj;
                obj["id"] = ev.id;
                obj["title"] = ev.title;
                obj["startTime"] = ev.startTime.toMSecsSinceEpoch();
                obj["calendarId"] = ev.calendarId;
                pending.append(obj);

                // Mark as fired
                m_store->kvSet(reminderKey, "fired");
            }
        }
    }

    return QString::fromUtf8(QJsonDocument(pending).toJson(QJsonDocument::Compact)).toStdString();
}

// ── Settings API ─────────────────────────────────────────────────────────────

void ScalaImpl::setSetting(const std::string& key, const std::string& value) {
    m_store->kvSet("setting_" + QString::fromStdString(key),
                   QString::fromStdString(value));
}

std::string ScalaImpl::getSetting(const std::string& key, const std::string& defaultValue) {
    QString val = m_store->kvGet("setting_" + QString::fromStdString(key));
    if (val.isEmpty())
        return defaultValue;
    return val.toStdString();
}

// ── Incoming sync messages ───────────────────────────────────────────────────

void ScalaImpl::onSyncMessageReceived(const std::string& calendarId, const std::string& msgJson) {
    // Parse the SyncMessage from JSON string
    nlohmann::json obj = nlohmann::json::parse(msgJson);
    SyncMessage msg = SyncMessage::fromJson(obj);

    QString qCalId = QString::fromStdString(calendarId);

    switch (msg.type) {
    case SyncMessageType::CreateEvent: {
        QJsonDocument evDoc = QJsonDocument::fromJson(QString::fromStdString(msg.payload).toUtf8());
        scala::CalendarEvent ev = scala::CalendarEvent::fromJson(evDoc.object());
        if (!ev.id.isEmpty()) {
            m_store->saveEvent(ev);
        }
        break;
    }
    case SyncMessageType::UpdateEvent: {
        QJsonDocument evDoc = QJsonDocument::fromJson(QString::fromStdString(msg.payload).toUtf8());
        scala::CalendarEvent ev = scala::CalendarEvent::fromJson(evDoc.object());
        if (!ev.id.isEmpty()) {
            m_store->updateEvent(ev);
        }
        break;
    }
    case SyncMessageType::DeleteEvent: {
        std::string eventId = msg.payload;
        if (!eventId.empty()) {
            m_store->deleteEvent(QString::fromStdString(eventId));
        }
        break;
    }
    case SyncMessageType::SyncEvents: {
        // Peer is requesting bulk sync — send all events for this calendar
        auto events = m_store->listEvents(qCalId);
        for (const auto& ev : events) {
            SyncMessage reply;
            reply.type = SyncMessageType::CreateEvent;
            reply.calendarId = qCalId.toStdString();
            reply.senderId = m_identity;
            reply.payload = QString::fromUtf8(
                QJsonDocument(ev.toJson()).toJson(QJsonDocument::Compact)).toStdString();
            reply.timestamp = QDateTime::currentMSecsSinceEpoch();
            m_sync->sendMessage(qCalId.toStdString(), reply);
        }
        break;
    }
    case SyncMessageType::UnshareCalendar: {
        m_sync->stopSync(qCalId.toStdString());
        auto cal = m_store->getCalendar(qCalId);
        if (!cal.id.isEmpty()) {
            cal.isShared = false;
            cal.encryptionKey.clear();
            cal.updatedAt = QDateTime::currentMSecsSinceEpoch();
            m_store->saveCalendar(cal);
        }
        break;
    }
    case SyncMessageType::CalendarReconnected: {
        // Peer reconnected — no action needed
        break;
    }
    }
}
