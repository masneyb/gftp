# SECURITY AUDIT — <Task Name / ID>
**Authoritative Location**: `{task_dir}/SECURITY_AUDIT.md`

> This document is the **authoritative security evaluation** of the task implementation.
> It is produced by the Security Warden and MUST be reviewed before final completion.

---

## 1) Audit Metadata

- **Date**: <YYYY-MM-DD>
- **Auditor**: Security Warden
- **Target Task Directory**: `{task_dir}`
- **Implementation Commit / Diff**: <Hash or Range>

---

## 2) Scope of Audit

- [ ] New Source Code
- [ ] Dependency Changes
- [ ] Infrastructure / Config Modifications
- [ ] Data Persistence / Schema Changes

---

## 3) Core Security Checklist

| Category | Status | Notes / Findings |
| :--- | :--- | :--- |
| **Secrets / Credentials** | ☐ Pass ☐ Fail | No hardcoded keys, tokens, or passwords found. |
| **PII / Privacy** | ☐ Pass ☐ Fail | No leakage of sensitive user data. |
| **Input Validation** | ☐ Pass ☐ Fail | Sanitization of entries, defense against injection. |
| **Dependencies** | ☐ Pass ☐ Fail | No new insecure packages or known CVEs. |
| **Permissions / Access** | ☐ Pass ☐ Fail | Adherence to least-privilege principles. |

---

## 4) Detailed Findings

### [ID-01] <Title of Finding>
- **Severity**: Critical / High / Medium / Low
- **Description**: Detailed explanation of the issue.
- **Impact**: What happens if this is exploited?
- **Recommended Fix**: Specific code or config change.
- **Status**: [ ] Open [ ] Resolved

---

## 5) Threat Model Update (If Applicable)

Does this implementation introduce a new attack vector or change the project's threat profile?
(e.g., new public API endpoint, new storage bucket).

---

## 6) Verdict

- [ ] **PASS**: No security concerns found.
- [ ] **PASS WITH ADVISORY**: Minor improvements suggested, not blocking.
- [ ] **BLOCK**: High/Critical severity issues MUST be fixed before implementation.

---

**Auditor Signature**: ________________
**Date of Verdict**: <YYYY-MM-DD>
