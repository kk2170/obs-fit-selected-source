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

OBS_REF=${OBS_REF:-31.0.0}
OBS_IMAGE_NAME=${OBS_IMAGE_NAME:-${IMAGE_NAME:-$(default_obs_image_name "${OBS_REF}")}}

docker build \
	-f "${REPO_ROOT}/docker/Dockerfile.obs-builder" \
	--build-arg "OBS_REF=${OBS_REF}" \
	-t "${OBS_IMAGE_NAME}" \
	"${REPO_ROOT}"
