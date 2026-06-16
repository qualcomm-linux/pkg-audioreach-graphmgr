# AudioReach Graph Manager

## Introduction

This repository hosts following software components for Linux:
- Audio Graph Manager (AGM)
- IPC client & server wrapper on top of DBus or Android HW binder
- TinyALSA PCM and Mixer plugins
- card-def parser

## Documentation

Refer AudioReach docs [here](https://audioreach.github.io/)

## Build instructions

Graph Manager repository supports various build systems: Android, Autotools.
Refer meta-audioreach [README](https://github.com/Audioreach/meta-audioreach?tab=readme-ov-file#openembedded-build--development-process)
for instructions to use AudioReach Graph Manager on OpenEmbedded system.

##### Dependency on AudioReach Graph Service
Refer to instruction from [AudioReach Graph Service repository](https://github.com/Audioreach/audioreach-graphservices) to pull in Graph Service dependency

##### Configuration Options
- --with-syslog:  Use syslog message logging utliity. If target device is not Android, enable this option
- --with-glib:  Graph Manager uses string utilities which are not available in default C library on the target system. In such case, enable this option
- --with-no-ipc: If AGM is not running as a service, disable IPC communication to AGM
- --with-are-on-apps: Enable ARE (SPF) on APPS support with ARE running in same process context as graph service libraries.

## License

Graph Manager is licensed under the BSD-3-Clause. Check out the [LICENSE](LICENSE) for more details
