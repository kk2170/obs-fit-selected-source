# obs-fit-selected-source

[日本語版 README](README.ja.md)

Minimal OBS frontend plugin that fits the currently selected scene item to the canvas while keeping aspect ratio.

## What it does

- Adds a Tools menu item: **Fit Selected Source to Canvas**
- Uses the current scene, or the preview scene when Studio Mode is active
- Applies only to selected scene items
- Accounts for source size and crop, clears any bounding box, and centers the item on the canvas
- Skips locked items, selected groups, rotated items, and items inside non-translation-only groups for safety

## Build

You need OBS Studio development headers/libraries available to CMake.

```bash
cmake -S . -B build
cmake --build build
```

If your OBS installation exports `OBS::libobs` and `OBS::obs-frontend-api` (or legacy `OBS::frontend-api`), CMake will use them. Otherwise it falls back to `pkg-config` (`libobs`, `obs-frontend-api`).

## Docker build

This repository includes a Docker builder that source-builds OBS Studio first so the frontend API and related development files are aligned reliably.

The builder image is intended to be pinned to a specific OBS version. By default, the scripts use `OBS_REF=31.0.0` and derive the default image name from it (for example `obs-plugin-builder:31.0.0`).

Build the builder image:

```bash
bash scripts/docker-build-image.sh
```

You can pin the OBS version/tag used inside the image:

```bash
OBS_REF=31.0.0 bash scripts/docker-build-image.sh
```

You can also override the image name explicitly with `OBS_IMAGE_NAME` (or legacy `IMAGE_NAME`). The build and shell scripts automatically rebuild the image if it does not exist yet, or if the image label indicates a different `OBS_REF` than requested.

Build the plugin inside the container:

```bash
bash scripts/docker-build.sh
```

After the Docker build, the installed plugin artifacts are staged on the host under `.docker/out/opt/obs`.

Open a debug shell in the same builder image:

```bash
bash scripts/docker-shell.sh
```

Default host-side output locations:

- build tree: `.docker/build`
- install staging: `.docker/out`

Installed plugin artifacts end up under `.docker/out/opt/obs`, for example:

- `.docker/out/opt/obs/lib*/obs-plugins/`
- `.docker/out/opt/obs/share/obs/obs-plugins/obs-fit-selected-source/`

The build script bind-mounts this repository to `/src`, configures with `cmake -S /src -B /build`, then runs `cmake --build /build` and `DESTDIR=/out cmake --install /build`.

Useful environment overrides:

- `OBS_REF`, `OBS_IMAGE_NAME` (or compatibility alias `IMAGE_NAME`) for `bash scripts/docker-build-image.sh`
- `OBS_REF`, `OBS_IMAGE_NAME`, `BUILD_TYPE`, `CMAKE_GENERATOR`, `OBS_DOCKER_BUILD_DIR`, `OBS_DOCKER_OUT_DIR` for `bash scripts/docker-build.sh`
- `OBS_REF`, `OBS_IMAGE_NAME`, `OBS_DOCKER_BUILD_DIR`, `OBS_DOCKER_OUT_DIR` for `bash scripts/docker-shell.sh`
- `OBS_REF`, `OBS_DOCKER_OUT_DIR`, `OBS_DOCKER_DIST_DIR` for `bash scripts/package-zip.sh`

### Local OBS deployment after Docker build

After `bash scripts/docker-build.sh`, first check where your target OBS installation keeps its plugin directories, then copy the staged files into the corresponding `obs-plugins` locations.

- library side from `.docker/out/opt/obs/lib/obs-plugins/`
- data side from `.docker/out/opt/obs/share/obs/obs-plugins/obs-fit-selected-source/`

On Ubuntu/Debian-based systems, one way to inspect the packaged locations is:

```bash
dpkg -L obs-studio | grep obs-plugins
```

Then copy the built files into the matching directories for the OBS instance you actually run. Flatpak, Snap, custom builds, and locally installed OBS copies may use different paths, so do not assume `/usr` or `/opt/obs` without checking first.

### Packaging a distribution zip

You can package the staged install tree into a zip file with:

```bash
bash scripts/package-zip.sh
```

By default this reads from the staging root `.docker/out`, looks for installed files under `opt/obs`, writes to `.docker/dist`, and creates a zip named like `obs-fit-selected-source-linux-31.0.0.zip`.

For compatibility, if `OBS_DOCKER_OUT_DIR` already points directly at an `opt/obs` tree, the script accepts that too.

The archive layout is structured for inspection after extraction, for example:

- `obs-fit-selected-source-linux-31.0.0/lib/obs-plugins/...`
- `obs-fit-selected-source-linux-31.0.0/share/obs/obs-plugins/...`

You can override paths or the OBS version label via environment variables:

```bash
OBS_REF=31.0.0 OBS_DOCKER_OUT_DIR=.docker/out OBS_DOCKER_DIST_DIR=.docker/dist bash scripts/package-zip.sh
```

If `OBS_REF` contains `/` or `:`, the zip file name and extracted top-level directory use a safe slugged form.

Compatibility note: this builder is based on Ubuntu 24.04, so binaries produced inside the container may depend on a newer glibc than older Linux distributions provide.

## Usage

1. Build and install/copy the plugin into your OBS plugin directory.
2. In OBS, select one or more scene items in the target scene.
3. Open **Tools** → **Fit Selected Source to Canvas**.
4. Confirm the menu item appears under **Tools**.
5. Check the OBS log for applied/skipped counts, and search the log for `obs-fit-selected-source` if you want to verify the plugin was loaded.

## Notes

- In Studio Mode, the plugin prefers the preview scene and falls back to the current scene if needed.
- Selected items inside a group are processed when parent groups are translation-only (position offset only). If a parent group has non-default scale/rotation/bounds/crop/alignment or flip, those child items are skipped to avoid unsafe results.
