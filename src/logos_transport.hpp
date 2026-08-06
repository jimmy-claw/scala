// logos_transport.hpp — SHARED, crypto-agnostic sync transport for a Logos Basecamp
// module's C++ core (the desktop counterpart of the React-Native logos-transport.ts).
//
// Extracted verbatim-where-possible from KYM's kym_core (multi-topic "routes"), proven
// in KYM + qaku. It wraps the host's `delivery_module` (SDS Reliable Channels over an
// embedded Waku node) and moves OPAQUE sealed bytes on content topics. It knows nothing
// about your data or crypto: you seal bytes and call send(); on receive it hands you the
// once-decoded bytes and YOU open them with your key. Identity/crypto/envelope/state/
// reconcile stay in your core; this owns the wire.
//
// ── Why an `Ops` struct instead of templating on the delivery-module type ──────
// `modules().delivery_module`'s type is GENERATED per-module and is only complete inside
// your generated `.cpp` — it can't be named in a header, so a `Transport<DeliveryModule>`
// member won't declare in your core's `.h`. Instead this templates ONLY on LogosMap
// (a real, complete SDK header = nlohmann::json) and takes the ~8 delivery calls as
// std::function wrappers (`Ops`) that YOU build in your `.cpp` (where the type is
// complete). So `Transport<LogosMap>` is a clean by-value member of your core.
//
// ── What YOU supply ──────────────────────────────────────────────────────────
//   • Ops        → thin lambdas wrapping modules().delivery_module.* (see basecamp/README.md)
//   • Config     → deviceId (SDS senderId), useChannels (keep true), entryNodes (headless only)
//   • topics()   → the content topics to join (one per room/household)
//   • onReceive(topic, sealedOnceDecoded) → open with your key + ingest
//   • onReady()  → post-start hook (subscribe-time reconcile / summary / seed) — optional
//   • delay(ms,fn) → e.g. QTimer::singleShot, needed only for Config.hubMode
//
// ── Gotchas baked in (each cost real debugging) ──────────────────────────────
//   1. DOUBLE-base64 channel framing: send() b64-encodes your sealed bytes and hands
//      that as a byte ARRAY (bytesPayload); delivery base64s again on the wire. Receive
//      b64-decodes ONCE and hands you the inner-b64 bytes; your open() peels the last
//      layer. MUST match the mobile transport or desktop<->mobile stops interoperating.
//   2. Send payload rep (byte ARRAY vs string) differs by cpp-sdk build — send() probes
//      array->string once and caches the winner (newer builds throw "type must be array,
//      but is string" on a string → SIGABRT; the probe avoids it, no env flag).
//   3. Incoming payload is a JSON string, a byte array, or {"_bytes":"<b64>"} — toWire()
//      handles all three.
//   4. Register receive handlers BEFORE createNode. Headless (logoscore): the handler-
//      registration IPC must reach the delivery process before the node is built (else
//      "No external callbacks" and nothing is delivered) — set Config.hubMode + supply
//      delay() to postpone createNode ~1.5s. GUI host wires it synchronously.
//   5. In-progress guard: bootstrap() may be polled every snapshot; a 2nd createNode mid-
//      startup makes nwaku never finish. One startup at a time; a failure clears the guard.
//   6. useChannels: SDS Reliable Channels (interops with mobile). OFF = raw relay, will
//      NOT reconcile with channel peers (the "stuck on Checking peers" symptom). Default ON.
//   7. entryNodes: empty on the GUI host (it bootstraps the fleet). A HEADLESS hub has no
//      host, so it MUST pin the fleet entryNodes or it joins no shard mesh ("No peers for
//      topic"). Merge an env-JSON override so you can pin without a rebuild.
//   8. Cross-thread receive: under logoscore, delivery may emit messageReceived off the
//      source thread and Qt Remote Objects DROPS it — run the hub on a logoscore built on
//      cpp-sdk >= d77c3dd (PR #68). GUI host unaffected. If onReceive never fires despite
//      traffic, that's the cause (not your code).
//
#pragma once
#include <string>
#include <vector>
#include <functional>
#include <cstdio>

namespace logos_transport {

// ── base64 (RFC 4648, no newlines). Swap for your core's if you already have one. ──
namespace b64detail {
inline const char *tbl() { return "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"; }
inline std::string encode(const std::string &in) {
    std::string out; out.reserve(((in.size() + 2) / 3) * 4);
    int val = 0, bits = -6; const unsigned char *p = (const unsigned char *)in.data();
    for (size_t i = 0; i < in.size(); ++i) { val = (val << 8) + p[i]; bits += 8;
        while (bits >= 0) { out.push_back(tbl()[(val >> bits) & 0x3F]); bits -= 6; } }
    if (bits > -6) out.push_back(tbl()[((val << 8) >> (bits + 8)) & 0x3F]);
    while (out.size() % 4) out.push_back('=');
    return out;
}
inline std::string decode(const std::string &in) {
    std::vector<int> T(256, -1); for (int i = 0; i < 64; i++) T[(unsigned char)tbl()[i]] = i;
    std::string out; int val = 0, bits = -8;
    for (unsigned char c : in) { if (c == '=' || T[c] == -1) break;
        val = (val << 6) + T[c]; bits += 6;
        if (bits >= 0) { out.push_back(char((val >> bits) & 0xFF)); bits -= 8; } }
    return out;
}
} // namespace b64detail

// Templated ONLY on LogosMap (nlohmann::json, a complete SDK header). Delivery calls
// come in via `Ops` std::functions so the generated delivery-module type never appears
// here — making Transport<LogosMap> a normal by-value member of your core.
template <class LogosMap>
class Transport {
public:
    using Cb      = std::function<void(bool ok, const std::string &err)>;              // delivery result → simple
    using RecvCb  = std::function<void(const std::string &topic, const LogosMap &payload)>;

    // Thin wrappers around modules().delivery_module.*, built in YOUR .cpp. Adapt the
    // SDK's StdLogosResult callbacks to Cb (ok,err) there. See basecamp/README.md.
    struct Ops {
        std::function<void(const std::string &cfgJson, Cb)> createNode;
        std::function<void(Cb)> start;
        std::function<void(const std::string &contentTopic, Cb)> subscribe;
        std::function<void(const std::string &channelId, const std::string &contentTopic,
                           const std::string &senderId, Cb)> channelCreate;
        std::function<void(const std::string &channelId, const LogosMap &payload, Cb)> channelSend;
        std::function<void(const std::string &contentTopic, const LogosMap &payload, Cb)> sendRaw; // raw relay (optional)
        std::function<void(RecvCb)> onMessage;         // raw relay receive
        std::function<void(RecvCb)> onChannelMessage;  // SDS channel receive
    };
    struct Config {
        std::string logLevel = "INFO";
        std::string preset = "logos.dev";
        std::vector<std::string> entryNodes;   // empty on GUI host; pin for headless hub
        bool useChannels = true;
        bool hubMode = false;                  // headless: delay createNode ~1.5s
        std::string deviceId;                  // SDS senderId
    };
    using TopicList = std::function<std::vector<std::string>()>;
    using OnReceive = std::function<void(const std::string &topic, const std::string &sealedOnceDecoded)>;
    using OnReady   = std::function<void()>;
    using SetStatus = std::function<void(const std::string &)>;
    using Delay     = std::function<void(int ms, std::function<void()>)>;

    Transport(Ops ops, Config cfg, TopicList topics, OnReceive onReceive,
              OnReady onReady = {}, SetStatus setStatus = {}, Delay delay = {})
        : m_ops(std::move(ops)), m_cfg(std::move(cfg)), m_topics(std::move(topics)),
          m_onReceive(std::move(onReceive)), m_onReady(std::move(onReady)),
          m_setStatus(std::move(setStatus)), m_delay(std::move(delay)) {}

    bool ready() const { return m_nodeReady; }

    // Bring the node up once (idempotent, poll-safe): register receive handlers →
    // createNode → start → (join every topic) → onReady().
    void bootstrap() {
        if (m_nodeReady || m_starting) return;
        auto topics = m_topics();
        if (topics.empty()) return;                    // unpaired — nothing to sync yet
        m_starting = true;

        auto toWire = [](const LogosMap &v) -> std::string {
            if (v.is_string()) return v.template get<std::string>();
            if (v.is_array()) { std::string s; s.reserve(v.size());
                for (const auto &c : v) if (c.is_number_integer()) s.push_back((char)c.template get<int>());
                return s; }
            if (v.is_object() && v.contains("_bytes") && v["_bytes"].is_string())
                return b64detail::decode(v["_bytes"].template get<std::string>());
            return std::string();
        };
        auto handle = [this, toWire](const std::string &topic, const LogosMap &payload) {
            std::string b64 = toWire(payload);
            if (b64.empty() && payload.is_object() && payload.contains("payload")) b64 = toWire(payload["payload"]);
            if (!b64.empty() && m_onReceive) m_onReceive(topic, b64detail::decode(b64)); // one decode; app peels the rest
        };
        if (m_ops.onMessage)        m_ops.onMessage(handle);
        if (m_ops.onChannelMessage) m_ops.onChannelMessage(handle);

        if (m_setStatus) m_setStatus("Connecting...");
        LogosMap cfg = LogosMap::object();
        cfg["logLevel"] = m_cfg.logLevel; cfg["mode"] = "Core"; cfg["preset"] = m_cfg.preset;
        if (!m_cfg.entryNodes.empty()) { cfg["relay"] = true;
            LogosMap arr = LogosMap::array(); for (auto &e : m_cfg.entryNodes) arr.push_back(e); cfg["entryNodes"] = arr; }
        const std::string cfgStr = cfg.dump();
        fprintf(stderr, "logos_transport bootstrap cfg=%s\n", cfgStr.c_str());

        auto startNode = [this, cfgStr]() {
            m_ops.createNode(cfgStr, [this](bool ok, const std::string &err) {
                if (!ok) { m_starting = false; if (m_setStatus) m_setStatus("Delivery error (createNode): " + err); return; }
                m_ops.start([this](bool ok2, const std::string &err2) {
                    if (!ok2) { m_starting = false; if (m_setStatus) m_setStatus("Delivery error (start): " + err2); return; }
                    m_nodeReady = true;
                    if (m_setStatus) m_setStatus("Connected");
                    for (const auto &t : m_topics()) join(t);   // subscribe + channelCreate every topic
                    if (m_onReady) m_onReady();                  // your post-start reconcile/summary/seed
                });
            });
        };
        if (m_cfg.hubMode && m_delay) m_delay(1500, startNode); else startNode();
    }

    // Join one topic (subscribe + create the SDS channel, or a plain subscription in raw
    // mode). channelId == contentTopic == the topic.
    void join(const std::string &topic) {
        if (m_cfg.useChannels) { if (m_ops.channelCreate) m_ops.channelCreate(topic, topic, m_cfg.deviceId, noop()); }
        else                   { if (m_ops.subscribe)     m_ops.subscribe(topic, noop()); }
    }

    // Publish sealed bytes. Encapsulates the double-base64 framing + the array/string
    // representation probe. Fire-and-forget (async) — a sync send would block the event
    // loop through a stalling lightpush and freeze the module.
    void send(const std::string &topic, const std::string &sealedBytes) {
        if (!m_nodeReady) return;
        const std::string b64 = b64detail::encode(sealedBytes);   // inner layer; delivery adds the outer
        auto attempt = [&](int repr) -> bool {
            try {
                LogosMap p = (repr == 1) ? bytesPayload(b64) : LogosMap(b64);
                if (m_cfg.useChannels) { if (m_ops.channelSend) m_ops.channelSend(topic, p, noop()); }
                else                   { if (m_ops.sendRaw)     m_ops.sendRaw(topic, p, noop()); }
                return true;
            } catch (...) { return false; }
        };
        if (m_sendRepr == 1 || m_sendRepr == 2) { if (attempt(m_sendRepr)) return; m_sendRepr = 0; }
        if (attempt(1)) { m_sendRepr = 1; return; }   // newer builds: byte-array payload
        if (attempt(2)) { m_sendRepr = 2; return; }   // older builds: string payload
        fprintf(stderr, "logos_transport send: no working payload representation\n");
    }

private:
    static Cb noop() { return [](bool, const std::string &) {}; }
    // Byte ARRAY payload — the current cpp-sdk throws on a JSON string. Same wire bytes.
    static LogosMap bytesPayload(const std::string &s) {
        LogosMap a = LogosMap::array();
        for (unsigned char c : s) a.push_back((unsigned)c);
        return a;
    }

    Ops m_ops; Config m_cfg; TopicList m_topics; OnReceive m_onReceive;
    OnReady m_onReady; SetStatus m_setStatus; Delay m_delay;
    bool m_nodeReady = false, m_starting = false;
    int m_sendRepr = 0;   // 0 unprobed, 1 byte array, 2 string
};

} // namespace logos_transport
