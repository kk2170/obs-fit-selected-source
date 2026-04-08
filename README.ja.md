# obs-fit-selected-source

[English README](README.md)

アスペクト比を維持したまま、現在選択中のシーンアイテムをキャンバスにフィットさせるための最小構成の OBS フロントエンドプラグインです。

## 概要

このプラグインは、OBS 上で現在選択しているソースをキャンバス全体に収まるよう自動調整します。ソースの縦横比は維持され、必要な位置合わせも自動で行われます。

## できること

- **Tools** メニューに **Fit Selected Source to Canvas** を追加します
- 通常時は現在のシーン、スタジオモード時はプレビューシーンを使用します
- 選択されているシーンアイテムにのみ適用します
- ソースサイズとクロップを考慮し、バウンディングボックスを解除して、キャンバス中央に配置します
- 安全のため、ロックされたアイテム、選択されたグループ、回転されたアイテム、変換が平行移動のみではないグループ内のアイテムはスキップします

## ビルド

CMake から参照できる OBS Studio の開発用ヘッダー／ライブラリが必要です。

```bash
cmake -S . -B build
cmake --build build
```

お使いの OBS インストールが `OBS::libobs` と `OBS::obs-frontend-api`（または旧名称の `OBS::frontend-api`）をエクスポートしていれば、CMake はそれらを使用します。そうでない場合は `pkg-config`（`libobs`, `obs-frontend-api`）にフォールバックします。

## Docker ビルド

このリポジトリには Docker ビルダーが含まれており、まず OBS Studio をソースからビルドすることで、frontend API および関連する開発用ファイルの整合性を安定して確保します。

ビルダーイメージは特定の OBS バージョンに固定して使うことを想定しています。デフォルトではスクリプトは `OBS_REF=31.0.0` を使用し、そこからデフォルトのイメージ名（例: `obs-plugin-builder:31.0.0`）を決定します。

ビルダーイメージを作成するには:

```bash
bash scripts/docker-build-image.sh
```

イメージ内で使用する OBS のバージョン／タグは次のように固定できます。

```bash
OBS_REF=31.0.0 bash scripts/docker-build-image.sh
```

また、`OBS_IMAGE_NAME`（または旧互換の `IMAGE_NAME`）でイメージ名を明示的に上書きすることもできます。build / shell スクリプトは、イメージがまだ存在しない場合や、イメージラベルが要求した `OBS_REF` と異なる場合に自動で再ビルドします。

コンテナ内でプラグインをビルドするには:

```bash
bash scripts/docker-build.sh
```

Docker ビルド後、インストール済みプラグインの成果物はホスト側の `.docker/out/opt/obs` 配下に配置されます。

同じビルダーイメージでデバッグ用シェルを開くには:

```bash
bash scripts/docker-shell.sh
```

ホスト側のデフォルト出力先:

- build ツリー: `.docker/build`
- インストール用ステージング: `.docker/out`

インストール済みプラグイン成果物の配置先例:

- `.docker/out/opt/obs/lib*/obs-plugins/`
- `.docker/out/opt/obs/share/obs/obs-plugins/obs-fit-selected-source/`

ビルドスクリプトは、このリポジトリを `/src` に bind mount し、`cmake -S /src -B /build` で設定した後、`cmake --build /build` と `DESTDIR=/out cmake --install /build` を実行します。

便利な環境変数上書き:

- `bash scripts/docker-build-image.sh` 用: `OBS_REF`, `OBS_IMAGE_NAME`（または互換エイリアス `IMAGE_NAME`）
- `bash scripts/docker-build.sh` 用: `OBS_REF`, `OBS_IMAGE_NAME`, `BUILD_TYPE`, `CMAKE_GENERATOR`, `OBS_DOCKER_BUILD_DIR`, `OBS_DOCKER_OUT_DIR`
- `bash scripts/docker-shell.sh` 用: `OBS_REF`, `OBS_IMAGE_NAME`, `OBS_DOCKER_BUILD_DIR`, `OBS_DOCKER_OUT_DIR`
- `bash scripts/package-zip.sh` 用: `OBS_REF`, `OBS_DOCKER_OUT_DIR`, `OBS_DOCKER_DIST_DIR`

## Dockerビルド後のローカルOBSへの配置

`bash scripts/docker-build.sh` 実行後は、まず対象の OBS インストール先がプラグインディレクトリをどこに持っているか確認し、その後ステージングされたファイルを対応する `obs-plugins` の場所へコピーしてください。

- ライブラリ側: `.docker/out/opt/obs/lib/obs-plugins/`
- データ側: `.docker/out/opt/obs/share/obs/obs-plugins/obs-fit-selected-source/`

Ubuntu / Debian 系では、パッケージ版の配置先を確認する方法のひとつとして次が使えます。

```bash
dpkg -L obs-studio | grep obs-plugins
```

そのうえで、実際に使用している OBS 環境に対応するディレクトリへビルド済みファイルをコピーしてください。Flatpak、Snap、独自ビルド、ローカル配置の OBS ではパスが異なる場合があるため、事前確認なしに `/usr` や `/opt/obs` を前提にしないでください。

## 配布zip作成

ステージング済みのインストールツリーは、次のコマンドで zip 化できます。

```bash
bash scripts/package-zip.sh
```

デフォルトでは、ステージングルート `.docker/out` を読み取り、`opt/obs` 配下のインストール済みファイルを対象に `.docker/dist` へ出力し、`obs-fit-selected-source-linux-31.0.0.zip` のような名前の zip を作成します。

互換性のため、`OBS_DOCKER_OUT_DIR` がすでに `opt/obs` ツリー自体を指している場合でも、このスクリプトは受け付けます。

展開後に確認しやすいよう、アーカイブ内のレイアウトは次のようになります。

- `obs-fit-selected-source-linux-31.0.0/lib/obs-plugins/...`
- `obs-fit-selected-source-linux-31.0.0/share/obs/obs-plugins/...`

パスや OBS バージョンラベルは環境変数で上書きできます。

```bash
OBS_REF=31.0.0 OBS_DOCKER_OUT_DIR=.docker/out OBS_DOCKER_DIST_DIR=.docker/dist bash scripts/package-zip.sh
```

`OBS_REF` に `/` や `:` が含まれている場合、zip ファイル名と展開後トップレベルディレクトリ名には安全な slug 化された文字列が使われます。

互換性に関する注意: このビルダーは Ubuntu 24.04 ベースのため、コンテナ内で生成されたバイナリは、古い Linux ディストリビューションでは利用できない新しめの glibc に依存する可能性があります。

## 使い方

1. プラグインをビルドし、OBS のプラグインディレクトリへインストールまたはコピーします。
2. OBS 上で、対象シーン内のシーンアイテムを 1 つ以上選択します。
3. **Tools** → **Fit Selected Source to Canvas** を開きます。
4. メニュー項目が **Tools** 配下に表示されていることを確認します。
5. 適用件数／スキップ件数は OBS ログで確認できます。プラグインが読み込まれているか確認したい場合は、ログ内で `obs-fit-selected-source` を検索してください。

## 注意点

- スタジオモードでは、プラグインはプレビューシーンを優先し、必要に応じて現在のシーンへフォールバックします。
- グループ内で選択されたアイテムも、親グループの変換が平行移動（位置オフセット）のみであれば処理対象になります。親グループにデフォルト以外のスケール／回転／バウンズ／クロップ／アラインメント、または反転がある場合、それらの子アイテムは安全のためスキップされます。
