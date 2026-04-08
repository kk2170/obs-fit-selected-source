#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
REPO_ROOT=$(cd -- "${SCRIPT_DIR}/.." && pwd)

default_obs_image_name() {
	local ref slug
	ref=$1
	slug=${ref//\//-}
	slug=${slug//:/-}
	slug=${slug//[^[:alnum:]_.-]/-}
	printf 'obs-plugin-builder:%s\n' "$slug"
}

ensure_builder_image() {
	local current_ref

	if ! docker image inspect "${OBS_IMAGE_NAME}" >/dev/null 2>&1; then
		OBS_IMAGE_NAME="${OBS_IMAGE_NAME}" IMAGE_NAME="${OBS_IMAGE_NAME}" OBS_REF="${OBS_REF}" bash "${SCRIPT_DIR}/docker-build-image.sh"
		return
	fi

	current_ref=$(docker image inspect --format '{{ index .Config.Labels "io.obsproject.obs-ref" }}' "${OBS_IMAGE_NAME}" 2>/dev/null || true)
	if [ "${current_ref}" != "${OBS_REF}" ]; then
		OBS_IMAGE_NAME="${OBS_IMAGE_NAME}" IMAGE_NAME="${OBS_IMAGE_NAME}" OBS_REF="${OBS_REF}" bash "${SCRIPT_DIR}/docker-build-image.sh"
	fi
}

OBS_REF=${OBS_REF:-31.0.0}
OBS_IMAGE_NAME=${OBS_IMAGE_NAME:-${IMAGE_NAME:-$(default_obs_image_name "${OBS_REF}")}}
BUILD_TYPE=${BUILD_TYPE:-Release}
CMAKE_GENERATOR=${CMAKE_GENERATOR:-Ninja}
OBS_DOCKER_BUILD_DIR=${OBS_DOCKER_BUILD_DIR:-${REPO_ROOT}/.docker/build}
OBS_DOCKER_OUT_DIR=${OBS_DOCKER_OUT_DIR:-${REPO_ROOT}/.docker/out}

resolve_path() {
	case "$1" in
		/*) printf '%s\n' "$1" ;;
		*) printf '%s\n' "${REPO_ROOT}/$1" ;;
	esac
}

BUILD_DIR=$(resolve_path "${OBS_DOCKER_BUILD_DIR}")
OUT_DIR=$(resolve_path "${OBS_DOCKER_OUT_DIR}")

mkdir -p "${BUILD_DIR}" "${OUT_DIR}"

ensure_builder_image

docker run --rm \
	--user "$(id -u):$(id -g)" \
	-e HOME=/tmp \
	-v "${REPO_ROOT}:/src" \
	-v "${BUILD_DIR}:/build" \
	-v "${OUT_DIR}:/out" \
	"${OBS_IMAGE_NAME}" \
	bash -lc "cmake -S /src -B /build -G \"${CMAKE_GENERATOR}\" -DCMAKE_BUILD_TYPE=\"${BUILD_TYPE}\" -DCMAKE_INSTALL_PREFIX=/opt/obs && cmake --build /build && DESTDIR=/out cmake --install /build"
