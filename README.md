# Scala — Secure CALendar App

A privacy-first shared calendar app built on [Logos Core](https://logos.co).

**Scala** = **S**ecure **CAL**endar **A**pp

## Background

Scala was originally built as a demo/prototype using React + Waku JS SDK ([vpavlin/scala](https://github.com/vpavlin/scala)), serving as a real-world test for `ReliableChannel` in js-waku (based on SDS — Scalable Data Sync).

This repo is a rewrite on Logos Core — native C++/QML module with Logos Messaging for P2P sync, local storage, and Logos identity.

## Features (planned)

- Multiple calendars per user
- Full event model: title, date/time, all-day, location, attendees, reminders, custom fields
- Calendar sharing via invite link / QR code
- Per-calendar encryption (each shared calendar has its own key)
- P2P sync via Logos Messaging (no central server)
- Logos Core identity as sender/attendee identity
- Offline-capable (local-first)
- System notifications

## Architecture

```
QML UI (Logos Core IComponent)
  ↓
C++ Module (LogosCalendar)
  ├── Local KV/SQLite     — calendars, events, local state
  ├── Logos Messaging     — P2P sync, per-calendar topic + encryption
  ├── Logos Core Identity — sender identity, attendee accounts
  └── Logos Storage       — attachments (future)
```

## Status

🚧 Early design phase — see [issues](https://github.com/jimmy-claw/scala/issues) for roadmap.

## Related

- Original prototype: [vpavlin/scala](https://github.com/vpavlin/scala)
- [Lope](https://github.com/jimmy-claw/lope) — notes app, same stack
- [Logos Core](https://github.com/logos-co/logos-app)
- [logos-co/ideas#20](https://github.com/logos-co/ideas/issues/20) — logos-kv-module proposal
