# iBowl Project — AI CLI Context

**Updated:** January 22, 2026  
**Version:** Post-v1.0.4 (motion-core + bowling-engine architecture)

---

## Project Overview

**iBowl** is an on-device ML bowling analysis app using MediaPipe GPU acceleration
for pose and hand detection across five phases of the bowling approach.

---

## Core Architecture

**Flutter App Layer**
- UI, state management (Riverpod), video selection and presentation

**BowlingEngine (Dart)**
- Bowling-specific logic (phases, metrics, scoring)
- Location: `lib/bowling_engine/`

**motion-core (Kotlin)**
- Sports-agnostic video processing
- MediaPipe pose + hand detection
- Ball detection (OpenCV + Kalman)
- Biomechanical correction
- Location: `android/motion-core/`

**motion-core-common (KMP)**
- Shared algorithms (RTS smoothing, utilities)
- Location: `android/motion-core-common/`

**Key Principle:**  
`motion-core` produces raw, corrected data.  
`bowling-engine` interprets that data into bowling-specific meaning.

---

## Current Development Themes (Non-Authoritative)

This section provides **context only** and MUST NOT be used to infer active tasks,
task priority, or task selection.

The authoritative source of work is always:
`docs/current_task_documentation/**`

Themes:
- Results visualization alignment for coaching tracks
- Metric correctness and presentation consistency
- Pose / landmark stability and downstream effects
- Performance optimization in native processing and data transfer
- Clarity of coaching feedback language for beginner users

---

## Core Systems & Data Flow

1. User selects video → `MotionCoreService` starts processing
2. `TieredPipeline` selects HIGH / MID / LOW tier
3. Detection:
    - Pose: MediaPipe VIDEO mode (33 landmarks)
    - Hand: MediaPipe VIDEO mode + ROI validation
    - Ball: OpenCV HoughCircles + Kalman filter
    - Smoothing: RTS (ball), biomechanical constraints (pose)
4. Data transfer:
    - `MotionCoreResult` via MethodChannel to Dart
5. Analysis:
    - `FrameDataConverter` → `FrameData`
    - `BowlingAnalyzer` orchestration
    - `BowlingPhaseDetector` (5 phases)
    - `MetricsRouter` (Side / Behind)

---

## Critical File Locations

### Dart
- `lib/bowling_engine/bowling_analyzer.dart`
- `lib/bowling_engine/phase/bowling_phase_detector.dart`
- `lib/bowling_engine/metrics/metrics_router.dart`
- `lib/presentation/analysis/results_presenter.dart`
- `lib/widgets/analysis_results/metric_gradient_card.dart`
- `lib/widgets/analysis_results/collapsible_phase_card.dart`
- `lib/services/analysis/video_analysis_service.dart`
- `lib/screens/video_analysis_screen.dart`

### Kotlin
- `android/motion-core/.../TieredPipeline.kt`
- `android/motion-core/.../DetectionCoordinator.kt`
- `android/motion-core/.../BallDetector.kt`
- `android/motion-core/.../PoseDetectorWrapper.kt`
- `android/motion-core/.../HandDetectorWrapper.kt`

---

## Coding Patterns & Conventions

- Logging: sparse, performance-aware
- Error handling: graceful degradation
- Comments: explain WHY, not WHAT
- Commits: `<type>(<scope>): <subject>`

---

## Quick Commands

### Flutter
```bash
flutter run --flavor dev --device-user 0
flutter clean && flutter pub get
flutter analyze
```

### Android
```bash
cd android && ./gradlew assembleDevDebug
cd android && ./gradlew compileDebugKotlin
```

### Git
```bash
git status
git diff HEAD
```
