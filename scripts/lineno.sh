#!/bin/sh
find ../faststdb -name "*.h" -or -name "*.cc" | xargs cat | wc  -l
