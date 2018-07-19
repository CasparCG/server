#!/bin/bash

# override args to diff changes
export CLANG_ARGS=-output-replacements-xml

SCRIPT_ROOT=$(dirname "${0}")/..
"${SCRIPT_ROOT}/tools/update-format.sh"
