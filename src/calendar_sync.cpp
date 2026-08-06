#include "calendar_sync.h"
#include "logos_transport.hpp"

#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/hmac.h>

#include <cstdio>
#include <sstream>
#include <iomanip>

using LogosMap = nlohmann::json;
using Tx = logos_transport::Transport<LogosMap>;

// Forward declaration — ScalaImpl builds Ops and constructs Transport
extern "C" void scala_init_transport(CalendarSync *sync, const std::string &deviceId);

// ── SyncMessage helpers ─────────────────────────────────────────────────────

std::string SyncMessage::typeToString(SyncMessageType t) {
    switch (t) {
    case SyncMessageType::CreateEvent:         return "CreateEvent";
    case SyncMessageType::UpdateEvent:         return "UpdateEvent";
    case SyncMessageType::DeleteEvent:         return "DeleteEvent";
    case SyncMessageType::SyncEvents:          return "SyncEvents";
    case SyncMessageType::UnshareCalendar:     return "UnshareCalendar";
    case SyncMessageType::CalendarReconnected: return "CalendarReconnected";
    }
    return "CreateEvent";
}

SyncMessageType SyncMessage::typeFromString(const std::string &s) {
    if (s == "CreateEvent")         return SyncMessageType::CreateEvent;
    if (s == "UpdateEvent")         return SyncMessageType::UpdateEvent;
    if (s == "DeleteEvent")         return SyncMessageType::DeleteEvent;
    if (s == "SyncEvents")          return SyncMessageType::SyncEvents;
    if (s == "UnshareCalendar")     return SyncMessageType::UnshareCalendar;
    if (s == "CalendarReconnected") return SyncMessageType::CalendarReconnected;
    return SyncMessageType::CreateEvent;
}

nlohmann::json SyncMessage::toJson() const {
    nlohmann::json obj;
    obj["type"]       = typeToString(type);
    obj["calendarId"] = calendarId;
    obj["payload"]    = payload;
    obj["senderId"]   = senderId;
    obj["timestamp"]  = timestamp;
    if (!signature.empty())
        obj["signature"] = signature;
    return obj;
}

SyncMessage SyncMessage::fromJson(const nlohmann::json &obj) {
    SyncMessage msg;
    msg.type       = typeFromString(obj.value("type", std::string("CreateEvent")));
    msg.calendarId = obj.value("calendarId", std::string());
    msg.payload    = obj.value("payload", std::string());
    msg.senderId   = obj.value("senderId", std::string());
    msg.timestamp  = obj.value("timestamp", int64_t(0));
    msg.signature  = obj.value("signature", std::string());
    return msg;
}

std::string SyncMessage::toBytes() const {
    return toJson().dump();
}

SyncMessage SyncMessage::fromBytes(const std::string &data) {
    return fromJson(nlohmann::json::parse(data, nullptr, false));
}

static std::string hexEncode(const unsigned char *data, size_t len) {
    std::ostringstream oss;
    for (size_t i = 0; i < len; i++)
        oss << std::hex << std::setfill('0') << std::setw(2) << (int)data[i];
    return oss.str();
}

std::string SyncMessage::sign(const std::string &payload, const std::string &key) {
    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int digestLen = 0;
    unsigned char *mac = HMAC(EVP_sha256(), key.c_str(), key.size(),
                              (unsigned char*)payload.c_str(), payload.size(),
                              digest, &digestLen);
    return mac ? hexEncode(digest, digestLen) : std::string();
}

bool SyncMessage::verify(const std::string &payload, const std::string &signature, const std::string &key) {
    return sign(payload, key) == signature;
}

// ── CalendarSync ────────────────────────────────────────────────────────────

CalendarSync::CalendarSync() {}
CalendarSync::~CalendarSync() {}

void CalendarSync::setMessageHandler(OnMessageReceived h) { m_onMessage = std::move(h); }
void CalendarSync::setStatusHandler(OnSyncStatus h) { m_onStatus = std::move(h); }

std::string CalendarSync::topicForCalendar(const std::string &calendarId) {
    return "/scala/1/" + calendarId + "/json";
}

bool CalendarSync::isSyncing(const std::string &calendarId) const {
    return m_activeTopics.count(calendarId) > 0;
}

void CalendarSync::startSync(const std::string &calendarId, const std::string &encryptionKey) {
    if (m_activeTopics.count(calendarId)) {
        fprintf(stderr, "CalendarSync: already syncing %s\n", calendarId.c_str());
        return;
    }

    m_activeTopics[calendarId] = encryptionKey;

    // If transport is ready, join the topic immediately
    if (m_tx && m_tx->ready()) {
        std::string topic = topicForCalendar(calendarId);
        m_tx->join(topic);
        fprintf(stderr, "CalendarSync: joined topic %s\n", topic.c_str());
    }

    if (m_onStatus) m_onStatus(calendarId, "syncing");
}

void CalendarSync::stopSync(const std::string &calendarId) {
    if (!m_activeTopics.count(calendarId)) return;
    m_activeTopics.erase(calendarId);
    if (m_onStatus) m_onStatus(calendarId, "stopped");
}

void CalendarSync::sendMessage(const std::string &calendarId, const SyncMessage &msg) {
    if (!m_activeTopics.count(calendarId)) {
        if (m_onStatus) m_onStatus(calendarId, "error: not syncing");
        return;
    }

    if (!m_tx || !m_tx->ready()) {
        fprintf(stderr, "CalendarSync: transport not ready, dropping message\n");
        return;
    }

    // Seal the message bytes
    std::string sealed = seal(calendarId, msg.toBytes());

    // Send via transport (it does double-base64 framing)
    std::string topic = topicForCalendar(calendarId);
    m_tx->send(topic, sealed);

    fprintf(stderr, "CalendarSync: sent %s to %s (%zu bytes)\n",
            SyncMessage::typeToString(msg.type).c_str(), topic.c_str(), msg.toBytes().size());
}

// ── Crypto helpers (seal/open) ─────────────────────────────────────────────

std::string CalendarSync::seal(const std::string &calendarId, const std::string &plaintext) {
    auto it = m_activeTopics.find(calendarId);
    if (it == m_activeTopics.end()) return plaintext; // fallback

    const std::string &keyHex = it->second;
    // Convert hex key to bytes
    std::vector<unsigned char> key(32);
    for (size_t i = 0; i < 32 && i * 2 < keyHex.size(); i++) {
        sscanf(keyHex.c_str() + i * 2, "%2hhx", &key[i]);
    }

    // Generate random nonce (12 bytes)
    std::vector<unsigned char> nonce(12);
    RAND_bytes(nonce.data(), 12);

    // AES-256-GCM encrypt
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    unsigned char ciphertext[plaintext.size() + 32];
    int len = 0, ciphertextLen = 0;

    EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, key.data(), nonce.data());
    EVP_EncryptUpdate(ctx, ciphertext, &len, (unsigned char*)plaintext.data(), plaintext.size());
    EVP_EncryptFinal_ex(ctx, ciphertext + len, &ciphertextLen);

    // Get tag
    unsigned char tag[16];
    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, 16, tag);
    EVP_CIPHER_CTX_free(ctx);

    // Pack: nonce(12) + tag(16) + ciphertext
    std::string result;
    result.resize(12 + 16 + len);
    memcpy(&result[0], nonce.data(), 12);
    memcpy(&result[12], tag, 16);
    memcpy(&result[28], ciphertext, len);

    return result;
}

std::optional<std::string> CalendarSync::open(const std::string &calendarId, const std::string &sealedBytes) {
    auto it = m_activeTopics.find(calendarId);
    if (it == m_activeTopics.end()) return std::nullopt;

    if (sealedBytes.size() < 28 + 16) return std::nullopt; // min: nonce+tag+16 bytes ciphertext

    const std::string &keyHex = it->second;
    std::vector<unsigned char> key(32);
    for (size_t i = 0; i < 32 && i * 2 < keyHex.size(); i++) {
        sscanf(keyHex.c_str() + i * 2, "%2hhx", &key[i]);
    }

    // Extract nonce, tag, ciphertext
    std::vector<unsigned char> nonce(12);
    unsigned char tag[16];
    memcpy(nonce.data(), sealedBytes.data(), 12);
    memcpy(tag, sealedBytes.data() + 12, 16);
    size_t ctLen = sealedBytes.size() - 28;
    const unsigned char *ct = (const unsigned char*)sealedBytes.data() + 28;

    // AES-256-GCM decrypt
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    std::string plaintext(ctLen, '\0');

    EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, key.data(), nonce.data());
    EVP_DecryptUpdate(ctx, (unsigned char*)plaintext.data(), (int*)(&ctLen), ct, (int)ctLen);

    // Set tag for verification
    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, 16, tag);

    int finalLen = 0;
    int ok = EVP_DecryptFinal_ex(ctx, (unsigned char*)plaintext.data() + ctLen, &finalLen);
    EVP_CIPHER_CTX_free(ctx);

    if (ok <= 0) return std::nullopt; // auth failure
    plaintext.resize(ctLen);
    return plaintext;
}

// ── Transport integration ───────────────────────────────────────────────────

void CalendarSync::handleReceive(const std::string &topic, const std::string &sealedOnceDecoded) {
    // Find which calendar this topic belongs to
    for (auto &[calendarId, _] : m_activeTopics) {
        if (topicForCalendar(calendarId) == topic) {
            // Open the sealed bytes
            auto plain = open(calendarId, sealedOnceDecoded);
            if (!plain) {
                fprintf(stderr, "CalendarSync: failed to open message for %s\n", calendarId.c_str());
                return;
            }

            // Parse and dispatch
            SyncMessage msg = SyncMessage::fromBytes(*plain);
            if (m_onMessage) m_onMessage(calendarId, msg);
            return;
        }
    }
    fprintf(stderr, "CalendarSync: received message for unknown topic %s\n", topic.c_str());
}

void CalendarSync::initTransport() {
    // Transport is constructed externally by ScalaImpl which has access to modules()
    // See scala_impl.cpp for the actual Ops construction
}

// Called from ScalaImpl after transport is constructed
void CalendarSync::setTransport(std::optional<Tx> tx) {
    m_tx = std::move(tx);
}

void CalendarSync::bootstrap() {
    if (m_tx && m_tx->ready()) return;
    if (m_tx) m_tx->bootstrap();
}

bool CalendarSync::ready() const {
    return m_tx.has_value() && m_tx->ready();
}
