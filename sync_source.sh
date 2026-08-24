#!/usr/bin/env bash
#
# 通用源码同步脚本：从远端 git 仓库的指定 commit 点同步文件到本地。
#
# 特性:
#   - 支持同步任意文件（相对仓库根目录的路径），可一次同步多个文件。
#   - 对 *.tar.gz / *.tgz 包，若其一级目录不等于「文件名去掉扩展名」，
#     会自动重新打包，使一级目录与文件名保持一致。
#     例如 ubs-mem-1.0.0.tar.gz -> 一级目录 ubs-mem-1.0.0。
#
# 用法:
#   ./sync_source.sh --repo <仓库URL> --commit <commit点> [--out <输出目录>] <文件> [<文件>...]
#
#   --repo    远端 git 仓库地址（必填）
#   --commit  commit 点，可为分支名 / 标签名 / commit SHA（必填）
#   --out     输出目录，默认为脚本所在目录（可选）
#   <文件>    要同步的文件路径，相对仓库根目录，可提供多个
#
# 示例:
#   ./sync_source.sh --repo https://gitcode.com/src-openeuler/ubs-mem.git \
#                    --commit openEuler-24.03-LTS-SP3 \
#                    ubs-mem-1.0.0.tar.gz ubs-mem.spec

set -euo pipefail

usage() {
    cat <<'EOF'
用法: ./sync_source.sh --repo <仓库URL> --commit <commit点> [--out <输出目录>] <文件> [<文件>...]

  --repo    远端 git 仓库地址（必填）
  --commit  commit 点，可为分支名 / 标签名 / commit SHA（必填）
  --out     输出目录，默认为脚本所在目录（可选）
  <文件>    要同步的文件路径，相对仓库根目录，可提供多个

示例:
  ./sync_source.sh --repo https://gitcode.com/src-openeuler/ubs-mem.git \
                   --commit openEuler-24.03-LTS-SP3 \
                   ubs-mem-1.0.0.tar.gz ubs-mem.spec
EOF
}

repo=""
commit=""
out_dir=""
files=()

while [ $# -gt 0 ]; do
    case "$1" in
        --repo)
            repo="${2:-}"
            [ -n "${repo}" ] || { echo "错误: --repo 需要参数" >&2; usage; exit 1; }
            shift 2
            ;;
        --commit)
            commit="${2:-}"
            [ -n "${commit}" ] || { echo "错误: --commit 需要参数" >&2; usage; exit 1; }
            shift 2
            ;;
        --out)
            out_dir="${2:-}"
            [ -n "${out_dir}" ] || { echo "错误: --out 需要参数" >&2; usage; exit 1; }
            shift 2
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        --)
            shift
            while [ $# -gt 0 ]; do files+=("$1"); shift; done
            ;;
        -*)
            echo "错误: 未知选项 $1" >&2
            usage
            exit 1
            ;;
        *)
            files+=("$1")
            shift
            ;;
    esac
done

[ -n "${repo}" ]    || { echo "错误: 缺少 --repo" >&2;   usage; exit 1; }
[ -n "${commit}" ]  || { echo "错误: 缺少 --commit" >&2; usage; exit 1; }
[ "${#files[@]}" -gt 0 ] || { echo "错误: 缺少要同步的文件" >&2; usage; exit 1; }

script_dir="$(cd "$(dirname "$0")" && pwd)"
out_dir="${out_dir:-${script_dir}}"
mkdir -p "${out_dir}"
out_dir="$(cd "${out_dir}" && pwd)"

# 将 gzip tar 包一级目录规范化为「文件名去掉扩展名」
normalize_tarball() {
    local path="$1" name top top_dirs repack
    name="$(basename "${path}")"
    case "${name}" in
        *.tar.gz) top="${name%.tar.gz}" ;;
        *.tgz)    top="${name%.tgz}" ;;
        *)        return 0 ;;
    esac

    top_dirs="$(tar -tzf "${path}" | awk -F/ '{print $1}' | sort -u)"
    if [ "$(printf '%s\n' "${top_dirs}" | wc -l)" -eq 1 ] && [ "${top_dirs}" = "${top}" ]; then
        return 0
    fi

    echo ">>> ${name} 一级目录不为 ${top}（当前: $(printf '%s' "${top_dirs}" | tr '\n' ' ')），重新打包 ..."
    repack="$(mktemp -d)"
    mkdir -p "${repack}/${top}"
    tar -xzf "${path}" -C "${repack}/${top}"
    tar -czf "${path}" --owner=root --group=root -C "${repack}" "${top}"
    rm -rf "${repack}"
    echo ">>> ${name} 重新打包后一级目录: $(tar -tzf "${path}" | awk -F/ '{print $1}' | sort -u)"
}

workdir="$(mktemp -d)"
trap 'rm -rf "${workdir}"' EXIT

echo ">>> 拉取 ${repo} @ ${commit} ..."
git init -q "${workdir}/upstream"
(
    cd "${workdir}/upstream"
    git remote add origin "${repo}"
    # 优先浅拉取指定 ref；失败则回退为完整拉取
    if ! git fetch --depth 1 origin "${commit}"; then
        echo ">>> 浅拉取失败，改为完整拉取 ${commit} ..."
        git fetch origin "${commit}"
    fi
)

cd "${workdir}/upstream"
rev="$(git rev-parse 'FETCH_HEAD^{commit}')"
echo ">>> 解析到 commit: ${rev}"

for f in "${files[@]}"; do
    echo ">>> 提取 ${f} ..."
    mkdir -p "${workdir}/out/$(dirname "${f}")"
    git show "${rev}:${f}" > "${workdir}/out/${f}"
done

for f in "${files[@]}"; do
    normalize_tarball "${workdir}/out/${f}"
done

for f in "${files[@]}"; do
    mkdir -p "${out_dir}/$(dirname "${f}")"
    cp -f "${workdir}/out/${f}" "${out_dir}/${f}"
done

echo ">>> 完成"
for f in "${files[@]}"; do
    ls -l "${out_dir}/${f}"
done