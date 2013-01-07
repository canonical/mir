This directory contains cucumber (http://cukes.info/) feature descriptions for mir windowing functionality (features/) and a cucumber test bridge (session_*.cpp).

The current feature description language is kept documented with intention at:
<TODO> Link to Google Document with feature language <TODO>

This directory builds a session-management-cucumber-steps executable which communicates with the cucumber executable. Thus the tests may be run as follows:

$ mir / : build/bin/session-management-cucumber-steps & cucumber tests/behavior-tests/features
