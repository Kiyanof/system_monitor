# System Resource Monitor

A lightweight system monitoring tool written in C for Linux systems that monitors system resources and Docker containers.

## Features

- CPU usage monitoring
- Memory usage tracking
- Disk I/O statistics
- Process listing
- Docker container monitoring

## Building the Project

### Prerequisites

- GCC compiler
- Make build system
- Linux system with procfs support
- Docker (for container monitoring)

### Build Instructions

```bash
cd build
make
```

## Usage

Basic usage:
```bash
./system_monitor [options]
```

Available options:
- `--cpu`: Monitor CPU usage
- `--memory`: Monitor memory usage
- `--disk`: Monitor disk I/O
- `--processes`: List active processes
- `--docker`: Monitor Docker containers
- `--interval=N`: Set update interval to N seconds

## Project Structure

```
system_monitor/
├── src/           # Source files
├── include/       # Header files
├── build/         # Build output
└── docs/          # Documentation
```

## Development Status

- [ ] Basic project setup
- [ ] CPU monitoring
- [ ] Memory monitoring
- [ ] Disk I/O monitoring
- [ ] Process listing
- [ ] Docker integration
- [ ] IPC implementation
- [ ] Testing and documentation
