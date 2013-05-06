#! /bin/sh -e

./buildCore.sh
./buildMetrics.sh
./buildRules.sh
./buildReporters.sh
./buildClangTooling.sh
./buildClangPlugin.sh
./buildRelease.sh
