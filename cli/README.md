# Scala CLI

A C++ client that connects to a running logoscore instance via QtRemoteObjects (LogosAPIClient).

## How it works

1. Reads the auth token from `/tmp/logos_scala_module` (written by logoscore on startup)
2. Connects to the logoscore QtRO registry using `LogosAPIClient`
3. Invokes the requested method on `scala_module`
4. Prints the result to stdout

This is the same pattern the QML UI uses to connect to logoscore.

## Prerequisites

- logoscore running with `scala_module` loaded (`make run-module`)

## Build

```bash
make build-cli
```

## Usage

### Direct binary

```bash
./build-module/scala_cli <method> [args...]
```

### Shell wrapper

```bash
make install-cli
scala-cli <command> [args]
```

### Commands

| Shell wrapper | Binary call | Description |
|---------------|-------------|-------------|
| `list-calendars` | `listCalendars` | List all calendars |
| `list-events <calId>` | `listEvents <calId>` | List events for a calendar |
| `create-calendar <name> <color>` | `createCalendar <name> <color>` | Create a new calendar |
| `share <calId>` | `generateShareLink <calId>` | Generate a share link |
| `join <link>` | `handleShareLink <link>` | Join a shared calendar |
| `identity` | `getIdentity` | Show current identity |

### Examples

```bash
# List all calendars
./build-module/scala_cli listCalendars

# Create a new calendar
./build-module/scala_cli createCalendar Work '#3b82f6'

# List events for a calendar
./build-module/scala_cli listEvents <calendar-id>
```

## Environment Variables

| Variable | Default | Description |
|----------|---------|-------------|
| `SCALA_CLI_BIN` | `../build-module/scala_cli` | Path to the C++ binary (for scala-cli.sh) |
