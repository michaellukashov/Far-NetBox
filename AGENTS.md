# AGENTS.md — NetBox Development Guide

> Version: 3.0.0 | Last updated: 2026-05-11

You are an AI coding assistant working on **NetBox** — a Far Manager plugin (SFTP/FTP/SCP/WebDAV/S3 client) built in C++17 on WinSCP, PuTTY, and FileZilla codebases. Far Manager is a text-mode file manager for Windows; NetBox loads as a plugin DLL via F11.

---

## What Is NetBox

NetBox is a **Far Manager 3.0 plugin** providing file transfer over multiple protocols:

| Protocol | Implementation Source |
|----------|----------------------|
| **SFTP/SCP** | PuTTY + WinSCP codebases |
| **FTP/FTPS** | FileZilla codebase |
| **WebDAV** | neon library |
| **S3** | libs3 library |

## At a Glance

| Attribute | Value |
|-----------|-------|
| **Language** | C++17 (no extensions) |
| **Compiler** | MSVC (Visual Studio 2022) |
| **Build System** | CMake 3.15+ / Ninja |
| **Platforms** | x86, x64, ARM64 |
| **IDE** | VS2022 (generate solution with `-G "Visual Studio 17 2022"`) |

---

## Documentation Index

This documentation is split into four files, each with a single responsibility. Start here, then follow the links below for the specific information you need.

| File | Responsibility |
|------|---------------|
| **[AGENTS.md](AGENTS.md)** (this file) | Project overview, protocols, at-a-glance, navigation index |
| **[AGENTS-Structure.md](.ai-factory/AGENTS-Structure.md)** | File and folder structure, CMake configuration, third-party libraries, versions, OpenSSL patches, NASM assembly, language files |
| **[AGENTS-Standards.md](.ai-factory/AGENTS-Standards.md)** | Code style, naming conventions, formatting, threading, memory management, error handling, anti-patterns, quality gates, AI behavioral principles |
| **[AGENTS-Workflows.md](.ai-factory/AGENTS-Workflows.md)** | Build, run, and test commands, development workflow, tool selection, git workflow, shell rules, CI, critical notes for agents, compact instructions |

Additionally, the **architecture** document provides layered dependency rules, protocol interface design, and module communication patterns:

| File | Purpose |
|------|---------|
| [ARCHITECTURE.md](.ai-factory/ARCHITECTURE.md) | Layered plugin architecture, dependency rules, protocol interface |

---

## Documentation

| Document | Path | Description |
|----------|------|-------------|
| README | README.md | Project landing page |
| Getting Started | docs/getting-started.md | Build, install, first steps |
| User Guide | docs/user-guide.md | Protocols, features, i18n |
| Architecture | docs/architecture.md | Project structure and patterns |
| Contributing | docs/contributing.md | Code conventions and workflow |
| Testing | docs/testing.md | Manual regression testing |
| OpenSSL Sync Report | docs/openssl_sync_cleanup_report.md | OpenSSL 3 synchronization details |
