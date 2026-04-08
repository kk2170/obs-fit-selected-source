#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
REPO_ROOT=$(cd -- "${SCRIPT_DIR}/.." && pwd)

PLUGIN_NAME=obs-fit-selected-source
OBS_REF=${OBS_REF:-31.0.0}
OBS_DOCKER_OUT_DIR=${OBS_DOCKER_OUT_DIR:-${REPO_ROOT}/.docker/out}
OBS_DOCKER_DIST_DIR=${OBS_DOCKER_DIST_DIR:-${REPO_ROOT}/.docker/dist}

slugify_ref() {
	local ref slug
	ref=$1
	slug=${ref//\//-}
	slug=${slug//:/-}
	slug=${slug//[^[:alnum:]_.-]/-}
	printf '%s\n' "$slug"
}

resolve_path() {
	case "$1" in
		/*) printf '%s\n' "$1" ;;
		*) printf '%s\n' "${REPO_ROOT}/$1" ;;
	esac
}

OUT_BASE_DIR=$(resolve_path "${OBS_DOCKER_OUT_DIR}")
DIST_DIR=$(resolve_path "${OBS_DOCKER_DIST_DIR}")
OBS_REF_SLUG=$(slugify_ref "${OBS_REF}")
PACKAGE_ROOT="${PLUGIN_NAME}-linux-${OBS_REF_SLUG}"
PACKAGE_PATH="${DIST_DIR}/${PACKAGE_ROOT}.zip"

if [ -d "${OUT_BASE_DIR}/opt/obs" ]; then
	OUT_DIR="${OUT_BASE_DIR}/opt/obs"
else
	OUT_DIR="${OUT_BASE_DIR}"
fi

PLUGIN_LIB_DIR="${OUT_DIR}/lib/obs-plugins"
PLUGIN_SHARE_DIR="${OUT_DIR}/share/obs/obs-plugins/${PLUGIN_NAME}"
PLUGIN_LIB_PATH="${PLUGIN_LIB_DIR}/${PLUGIN_NAME}.so"

if [ ! -f "${PLUGIN_LIB_PATH}" ]; then
	printf 'error: required plugin library was not found: %s\n' "${PLUGIN_LIB_PATH}" >&2
	exit 1
fi

if [ ! -d "${PLUGIN_SHARE_DIR}" ]; then
	printf 'error: required plugin data directory was not found: %s\n' "${PLUGIN_SHARE_DIR}" >&2
	exit 1
fi

mkdir -p "${DIST_DIR}"

if command -v python3 >/dev/null 2>&1; then
	python3 - "${OUT_DIR}" "${PACKAGE_PATH}" "${PACKAGE_ROOT}" "${PLUGIN_NAME}" <<'PY'
import pathlib
import sys
import zipfile

out_dir = pathlib.Path(sys.argv[1])
package_path = pathlib.Path(sys.argv[2])
package_root = pathlib.Path(sys.argv[3])
plugin_name = sys.argv[4]

required = [
    out_dir / "lib" / "obs-plugins" / f"{plugin_name}.so",
    out_dir / "share" / "obs" / "obs-plugins" / plugin_name,
]

for path in required:
    if not path.exists():
        raise SystemExit(f"error: required artifact was not found: {path}")

with zipfile.ZipFile(package_path, "w", compression=zipfile.ZIP_DEFLATED) as zf:
    for relative in [
        pathlib.Path("lib") / "obs-plugins" / f"{plugin_name}.so",
        pathlib.Path("share") / "obs" / "obs-plugins" / plugin_name,
    ]:
        source = out_dir / relative
        if source.is_dir():
            for child in sorted(source.rglob("*")):
                if child.is_file():
                    zf.write(child, package_root / relative / child.relative_to(source))
        else:
            zf.write(source, package_root / relative)
PY
else
	printf 'error: python3 is required to create the zip archive\n' >&2
	exit 1
fi

printf 'created %s\n' "${PACKAGE_PATH}"
