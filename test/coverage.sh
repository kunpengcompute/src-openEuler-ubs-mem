#!/bin/bash
set -e
#  sh coverage.sh SRC_PATH CURRENT_PATH
SRC_PATH=$1
CURRENT_PATH=$2

LCOV=$(which lcov)
GEN_HTML=$(which genhtml)

gcno_dirs=$(find $SRC_PATH -name "*.gcno" -exec dirname {} \; | sort -u)
gcda_dirs=$(find $SRC_PATH -name "*.gcda" -exec dirname {} \; | sort -u)

mkdir -p $CURRENT_PATH/build/coverage
rm $CURRENT_PATH/build/coverage/* -rf

generate_coverage_info() {
    local dir=$1
    local output_file=$2
    local init_flag=$3

    if [ "$init_flag" == "true" ]; then
        ${LCOV} -c -q -i --no-recursion -d "$dir" -o "$output_file" --rc lcov_branch_coverage=1 \
        --rc lcov_excl_br_line="LCOV_EXCL_BR_LINE|DAGGER_*|DBG_*|ULOG_*|TP_TRACE*";
    else
        ${LCOV} -c -q --no-recursion -d "$dir" -o "$output_file" --rc lcov_branch_coverage=1 \
        --rc lcov_excl_br_line="LCOV_EXCL_BR_LINE|DAGGER_*|DBG_*|ULOG_*|TP_TRACE*";
    fi
}

echo "[INFO] Generate coverage info list..."
for dir in $gcno_dirs; do
    output_file="$CURRENT_PATH/build/coverage/$(echo "$dir" | tr '/' '_').init.info"
    generate_coverage_info "$dir" "$output_file" true &
done

for dir in $gcda_dirs; do
    output_file="$CURRENT_PATH/build/coverage/$(echo "$dir" | tr '/' '_'|tr . _).run.info"
    generate_coverage_info "$dir" "$output_file" false &
done

wait

echo "[INFO] Combining init info and run info..."

init_output="$CURRENT_PATH/build/coverage/init.info"
find $CURRENT_PATH/build/coverage/ -name "*.init.info" -size +0 | xargs printf '-a %s ' | xargs \
${LCOV} -q -o "$init_output" --rc lcov_branch_coverage=1 &

run_output="$CURRENT_PATH/build/coverage/run.info"
find $CURRENT_PATH/build/coverage/ -name "*.run.info" -size +0 | xargs printf '-a %s ' | xargs \
${LCOV} -q -o "$run_output" --rc lcov_branch_coverage=1 &

wait

rm -rf  $CURRENT_PATH/build/coverage/*.run.info  $CURRENT_PATH/build/coverage/*.init.info

test_output="$CURRENT_PATH/build/coverage/test.info"
${LCOV} -q -o "$test_output" -a "$init_output" -a "$run_output"  --rc lcov_branch_coverage=1
echo "[INFO] Combining coverage.info..."

#####################################################################################################################
##########################################    需要屏蔽的目录	   ########################################################
#####################################################################################################################
coverage_output="$CURRENT_PATH/build/coverage.info"

${LCOV} -q -e $test_output */ubs-mem/src/* -output-file $coverage_output --rc lcov_branch_coverage=1;

${LCOV} -q --remove $coverage_output */ubs-mem/src/htracer* -o $coverage_output --rc lcov_branch_coverage=1;
${LCOV} -q --remove $coverage_output */ubs-mem/src/uc* -o $coverage_output --rc lcov_branch_coverage=1;

${LCOV} --summary $coverage_output --rc lcov_branch_coverage=1;
#####################################################################################################################
#####################################################################################################################
#####################################################################################################################

${GEN_HTML} -q -o $CURRENT_PATH/build/gcovr_report $CURRENT_PATH/build/coverage.info --show-details --legend --rc lcov_branch_coverage=1