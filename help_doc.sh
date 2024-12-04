#!/bin/bash

cat << HEREDOC

### sexp_prettify_cli.py (python) (KiCad fixed styling)
\`\`\`
$(./sexp_prettify_cli.py -h)
\`\`\`

### sexp_prettify_cpp_cli (cpp)
\`\`\`
$(./sexp_prettify_cpp_cli -h)
\`\`\`

### sexp_prettify_cli (c)
\`\`\`
$(./sexp_prettify_cli -h)
\`\`\`
HEREDOC