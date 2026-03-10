# Scala CLI

A thin shell wrapper around `logoscore --call` for headless use of the Scala calendar module.

## Prerequisites

- `logoscore` binary (from [logos-liblogos](https://github.com/logos-co/logos-liblogos))
- `kv_module` and `scala_module` plugins installed in the modules directory

## Installation

```bash
make install-cli
```

This installs `scala-cli` to `~/.local/bin/`.

## Usage

```bash
scala-cli <command> [args]
```

### Commands

| Command | Description |
|---------|-------------|
| `list-calendars` | List all calendars |
| `list-events <calId>` | List events for a calendar |
| `create-calendar <name> <color>` | Create a new calendar |
| `share <calId>` | Generate a share link |
| `join <link>` | Join a shared calendar |
| `identity` | Show current identity |

### Examples

```bash
# List all calendars
scala-cli list-calendars

# Create a new calendar
scala-cli create-calendar Work '#3b82f6'

# List events for a calendar
scala-cli list-events <calendar-id>

# Share a calendar
scala-cli share <calendar-id>
```

## Environment Variables

| Variable | Default | Description |
|----------|---------|-------------|
| `SCALA_LOGOSCORE` | auto-detect | Path to `logoscore` binary |
| `SCALA_MODULES_DIR` | `~/.local/share/logos/modules` | Modules directory |
| `SCALA_NAMESPACE` | `default` | Namespace for data isolation |
