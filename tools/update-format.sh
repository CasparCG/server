#!/bin/bash

# args needed to format inline. can be overridden to use script for otehr purposes
CLANG_ARGS=${CLANG_ARGS:--i}

SOURCE_FILES=`find ./src/ \( -name \*.cpp -type f -or -name \*.h -type f \) -not -path "./src/packages/*" -not -path "*interop*" -not -path "./src/cmake*"`
BAD_FILES=0
for SOURCE_FILE in $SOURCE_FILES
do
  export FORMATTING_ISSUE_COUNT=`clang-format $CLANG_ARGS $SOURCE_FILE | grep offset | wc -l`
  if [ "$FORMATTING_ISSUE_COUNT" -gt "0" ]; then
    echo "Source file $SOURCE_FILE contains formatting issues."
    BAD_FILES=1
  fi
done

if [ $BAD_FILES ]; then 
  exit 1
fi
