#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

// Forward declarations
namespace logos_transport {
template <class LogosMap> class Transport;
}

// ── Sync message types ──────────────────────────────────────────────────────

enum class SyncMessageType : int {
    CreateEvent = 0,
    UpdateEvent,
    DeleteEvent,
    SyncEvents,          // bulk sync on join
    UnshareCalendar,
    CalendarReconnected
};

// ── SyncMessage (wire envelope) ────────────────────────────────────────────

struct SyncMessage {
    SyncMessageType type = SyncMessageType::CreateEvent;
    std::string calendarId;
    std::string payload;     // JSON-encoded CalendarEvent or empty
    std::string senderId;    // Logos identity pubkey
    int64_t timestamp = 0;
    std::string signature;

    nlohmann::json toJson() const;
    static SyncMessage fromJson(const nlohmann::json &obj);
    std::string toBytes() const;
    static SyncMessage fromBytes(const std::string &data);

    static std::string typeToString(SyncMessageType t);
    static SyncMessageType typeFromString(const std::string &s);

    // HMAC-SHA256 signature
    static std::string sign(const std::string &payload, const std::string &key);
    static bool verify(const std::string &payload, const std::string &signature, const std::string &key);
};

// ── CalendarSync (transport-backed) ────────────────────────────────────────

using LogosMap = nlohmann::json;

class CalendarSync {
public:
    using OnMessageReceived = std::function<void(const std::string &calendarId, const SyncMessage &msg)>;
    using OnSyncStatus = std::function<void(const std::string &calendarId, const std::string &status)>;

    CalendarSync();
    ~CalendarSync();

    /// Set callbacks (called from ScalaImpl)
    void setMessageHandler(OnMessageReceived h);
    void setStatusHandler(OnSyncStatus h);

    /// Start syncing a calendar (subscribe to its topic via transport).
    void startSync(const std::string &calendarId, const std::string &encryptionKey);

    /// Stop syncing a calendar.
    void stopSync(const std::string &calendarId);

    /// Send a sync message for a calendar (seals + broadcasts via transport).
    void sendMessage(const std::string &calendarId, const SyncMessage &msg);

    /// Whether a calendar is currently syncing.
    bool isSyncing(const std::string &calendarId) const;

    /// Topic format: /scala/1/<calendarId>/json
    static std::string topicForCalendar(const std::string &calendarId);

    /// Build and initialize the transport (called from ScalaImpl::onContextReady).
    /// Requires: delivery_module methods available via modules().
    void initTransport();

    /// Set the constructed transport (called from ScalaImpl after Ops wiring).
    void setTransport(std::optional<logos_transport::Transport<LogosMap>> tx);

    /// Bootstrap the transport (bring up node, join topics).
    void bootstrap();

    /// Check if transport is ready.
    bool ready() const;

    /// Handle incoming sealed message from transport (called by ScalaImpl callback).
    void handleReceive(const std::string &topic, const std::string &sealedOnceDecoded);

private:
    // calendarId -> encryption key
    std::map<std::string, std::string> m_activeTopics;

    // Transport (optional — constructed in initTransport)
    std::optional<logos_transport::Transport<LogosMap>> m_tx;

    // Callbacks
    OnMessageReceived m_onMessage;
    OnSyncStatus m_onStatus;

    // Device identity (set from ScalaImpl)
    std::string m_deviceId;

    /// Seal bytes with calendar encryption key + random nonce.
    std::string seal(const std::string &calendarId, const std::string &plaintext);

    /// Open sealed bytes with calendar encryption key.
    std::optional<std::string> open(const std::string &calendarId, const std::string &sealedBytes);
};
