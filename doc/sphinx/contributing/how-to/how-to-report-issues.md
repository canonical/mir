---
myst:
  html_meta:
    description: How to report Mir bugs and request features with actionable details, reproducible steps, and the right channels.
---

(contributing-reporting-issues)=

# How to report Mir issues and request features

Good issue reports make fixes faster and reduce back-and-forth.
Use this guide when reporting bugs, regressions, or feature requests for Mir.

## Pick the right channel

- Use [GitHub issues](https://github.com/canonical/mir/issues) for actionable bugs and feature requests.
- Use [Mir Discourse](https://discourse.ubuntu.com/c/project/mir/15) for early discussion, design ideas, and broader questions.
- Use [security reporting guidance](https://github.com/canonical/mir/blob/main/SECURITY.md) for vulnerabilities.

## Include the essential information

A high-quality issue usually includes:

1. What you expected to happen.
1. What actually happened.
1. Exact steps to reproduce.
1. Mir version and install source (for example package, snap channel, or build from source).
1. Environment details (distribution and version, graphics stack, hardware where relevant).
1. Relevant logs, error output, and screenshots if they clarify behavior.

If the issue is intermittent, describe frequency and any patterns you have observed.

## Share reproduction details clearly

- Prefer minimal reproduction steps over full system histories.
- Include command lines exactly as run.
- Call out whether behavior differs between hosted and native sessions, if applicable.

## For feature requests

For new features, include:

- the user problem you are trying to solve,
- current workarounds and their limitations,
- why this should live in Mir (instead of shell-specific policy).

Concrete use cases help maintainers assess scope and priority.

## Follow-up after filing

- Watch for maintainer questions and provide additional details promptly.
- If you discover a better reproduction method, edit the top issue description so new readers see the latest steps.
- If fixed, link the fixing PR or release note for future traceability.

## Related resources

- [Getting involved in Mir](getting-involved-in-mir)
- [Good first issues](https://github.com/canonical/mir/issues?q=is%3Aissue%20state%3Aopen%20label%3A%22Good%20first%20issue%22)
- [How to test Mir for a release](how-to-test-mir-for-a-release)
