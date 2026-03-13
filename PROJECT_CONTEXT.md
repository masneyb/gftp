# gFTP  Project — AI CLI Context

**Updated:** Feburary  27, 2026  
**Version:** (Pre-releasee)

---

## Project Overview

**gFTP.app** is a port of a legacy Linux gtk2/gtk3 application to macOS 

---

## Core Architecture

**gftp**
- Is a file transfer application that supports both GUI and console versions. In our case with focus on the macOS port

** Main gftp programs (C)**
- Currently runs on gtk3 in compiblity mode
- Application suppported gtk2 for a long time due to GtkList being deprecated

**Key Principle:**  
` - gFTP.app` Provide the user with an easy to use ftp and scp client for file transfers.

---

## Current Development Themes (Non-Authoritative)

This section provides **context only** and MUST NOT be used to infer active tasks,
task:w priority, or task selection.

The authoritative source of work is always:
`docs/current_task_documentation/**`

Themes:
- Match the macOS host styles well as s possible 
- Explore gtk3 on macOS support for taskbar intigration
- Explore support for running on hi-DPI screens using scaled icons

---

## Core Systems & Data Flow

1. User seelects the gFTP.app bundle 
2. Application Opem
3. User configures connction to client
    - User either inputs anew host and IP Adddress along with usernaem and n:
    - Enters the remote IP address or one selected from bookmarks
3. Detection:
4. Data transfer: File trasfer between hosts should be seemless if authenticated
5. Analysis:
    - 'bi-direction file transfer works``]

---

## Critical File Locations

### C
- `src/gtk/*`
- `src/text/*'`
- `src/uicommon'`
- 'po'

---

