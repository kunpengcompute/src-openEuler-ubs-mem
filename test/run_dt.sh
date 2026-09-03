#!/bin/bash
apply_mockcpp_patch() {
    local mockcpp_src_dir="$1"
    local patch_file="$2"
    local result=0

    # 检查参数是否完整
    if [ -z "${mockcpp_src_dir}" ] || [ -z "${patch_file}" ]; then
        echo "[ERROR] Usage: apply_mockcpp_patch <mockcpp_source_dir> <patch_file>"
        return 1
    fi

    # 检查是否已打过补丁（通过是否存在新增的 ARM64 文件）
    if [ -e "${mockcpp_src_dir}/src/JmpCodeAARCH64.h" ]; then
        echo "[STATUS] ARM64 patch already applied, skipping."
        return 0
    fi

    # 检查补丁文件是否存在
    if [ ! -e "${patch_file}" ]; then
        echo "[WARNING] Patch file not found: ${patch_file}"
        return 0  # 文件不存在不视为错误
    fi

    echo "[STATUS] Applying mockcpp patch: ${patch_file}"

    # 读取补丁文件内容，提取所有要修改的文件
    local patch_content
    patch_content=$(cat "${patch_file}")

    # 修复grep模式，确保能正确匹配文件路径
    local matches
    matches=$(echo "${patch_content}" | grep -o 'diff --git a/[^"]*')

    # Normalize line endings
    for match in ${matches}; do
        local file
        file=$(echo "${match}" | sed -E "s/diff --git a\/(.+)/\1/")
        if [ -e "${mockcpp_src_dir}/${file}" ]; then
            sed -i "s/\r\$//" "${mockcpp_src_dir}/${file}"
        fi
    done

    # Apply patch with better error handling
    echo "[INFO] Applying patch..."
    patch -p1 --verbose -d "${mockcpp_src_dir}" < "${patch_file}"
    local patch_result=$?

    if [ ${patch_result} -ne 0 ]; then
        echo "[ERROR] Failed to apply mockcpp patch!"
        echo "[ERROR] Patch file: ${patch_file}"
        echo "[ERROR] Exit code: ${patch_result}"
        return ${patch_result}
    else
        echo "[STATUS] Patch applied successfully."
        return 0
    fi
}

update_3rdparty() {
  git submodule update --init --recursive --depth 1
  cd ${CURRENT_PATH}/3rdparty
  apply_mockcpp_patch ./mockcpp ./mockcpp_support_arm64.patch
}


run_encrypt_tool() {
  cd $BUILD_PATH
  local output_file="$BUILD_PATH/keypass.txt"

  mkdir -p $BUILD_PATH/../config

  cat << EOF > $BUILD_PATH/../config/crypto_tool_config.json
{
    "algorithm": "AES256_GCM",
    "keyManagerType": "kmc",
    "thirdKeyManager": {
        "keyManagerPath": "",
        "keyManagerAddr": ""
    }
}
EOF
  result=$(echo "test123")
  echo "$result" | tr -d '\n'> "$output_file"
  echo "  Encrypted string saved to: $output_file"
}

compile_and_run() {
  set -e
  DEBUG_FUZZ=OFF
  cmake -DCMAKE_BUILD_TYPE=Debug -DASAN_BUILD=ON -DDEBUG_UT=ON -DDEBUG_FUZZ=${DEBUG_FUZZ} -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -S $SRC_PATH -B $BUILD_PATH
  echo "end cmake."
  cd $BUILD_PATH
  pwd;
  N_CPUS=$(grep processor /proc/cpuinfo | wc -l)
  echo "$N_CPUS processors detected."
  make -j $N_CPUS install;
  echo "end make install."
  if [ "$SKIP_RUN_TESTS" = true ]; then
    return
  fi
  cp $BUILD_PATH/output/bin/* $BUILD_PATH
  cp $BUILD_PATH/output/lib/* $BUILD_PATH
  run_encrypt_tool
  ASAN_PATH=$(find /usr/lib64/ -name *asan* |head -n 1)
  echo "LD_PRELOAD=$ASAN_PATH:$LD_PRELOAD LD_LIBRARY_PATH=$BUILD_PATH HSECEASY_PATH=$BUILD_PATH ${BUILD_PATH}/mxmd_ut --gtest_break_on_failure --gtest_output=xml:gcover_report/test_detail.xml"
  LD_PRELOAD=$ASAN_PATH:$LD_PRELOAD LD_LIBRARY_PATH=$BUILD_PATH HSECEASY_PATH=$BUILD_PATH ${BUILD_PATH}/mxmd_ut --gtest_break_on_failure --gtest_output=xml:gcover_report/test_detail.xml
  if [ ${DEBUG_FUZZ} = ON ]; then
    LD_PRELOAD=$ASAN_PATH:$LD_PRELOAD LD_LIBRARY_PATH=$BUILD_PATH:$LD_LIBRARY_PATH HSECEASY_PATH=$BUILD_PATH ${BUILD_PATH}/mxmd_fuzz --gtest_output=xml:gcover_report/test_fuzz_detail.xml
  fi
}

SKIP_RUN_TESTS=false
for arg in "$@"; do
  case "$arg" in
    --skip-run-tests)
      SKIP_RUN_TESTS=true
      ;;
  esac
done

CURRENT_PATH=$(cd "$(dirname "$0")"; pwd)
BUILD_PATH="${CURRENT_PATH}/build"
SRC_PATH="${CURRENT_PATH}/../"
COVERAGE_PATH="${BUILD_PATH}/coverage"
update_3rdparty
rm -rf "$COVERAGE_PATH"

set -e
sh ${CURRENT_PATH}/cert.sh
start_compile=$(date +%s%3N)
compile_and_run
end_compile=$(date +%s%3N)

if [ "$SKIP_RUN_TESTS" = false ]; then
  start_coverage=$(date +%s%3N)
  sh  ${CURRENT_PATH}/coverage.sh ${SRC_PATH} ${CURRENT_PATH}
  end_coverage=$(date +%s%3N)
fi

echo "The time consumed by each step is as follows:"
echo "update_deps: $(((end_update_deps - start_update_deps)/1000)).$(((end_update_deps - start_update_deps)%1000))s"
echo "compile_and_run: $(((end_compile - start_compile)/1000)).$(((end_compile - start_compile)%1000))s"
if [ "$SKIP_RUN_TESTS" = false ]; then
  echo "coverage: $(((end_coverage - start_coverage)/1000)).$(((end_coverage - start_coverage)%1000))s"
fi