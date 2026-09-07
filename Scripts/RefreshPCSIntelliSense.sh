#!/bin/zsh

set -euo pipefail

SCRIPT_DIRECTORY="${0:A:h}"
HOST_DIRECTORY="${SCRIPT_DIRECTORY:h}"
PLUGIN_DIRECTORY="${HOST_DIRECTORY:h}/pcs"
SOURCE_DATABASE="${HOST_DIRECTORY}/.vscode/compileCommands_PCSHost.json"
OUTPUT_DATABASE="${PLUGIN_DIRECTORY}/.vscode/compile_commands.json"
TEMP_DATABASE="${OUTPUT_DATABASE}.tmp"
XCODE_DEVELOPER_DIRECTORY="$(xcode-select -p)"
COMPILER_PATH="${XCODE_DEVELOPER_DIRECTORY}/Toolchains/XcodeDefault.xctoolchain/usr/bin/clang++"
SDK_PATH="$(xcrun --sdk macosx --show-sdk-path)"
LIBCXX_PATH="${SDK_PATH}/usr/include/c++/v1"

if [[ ! -f "${SOURCE_DATABASE}" ]]; then
	print -u2 "Missing generated compile database: ${SOURCE_DATABASE}"
	print -u2 "Run GenerateProjectFiles.sh for PCSHost first."
	exit 1
fi

HEADER_PATHS_JSON="$(
	find "${PLUGIN_DIRECTORY}/Source/PCS" -type f \( -name '*.h' -o -name '*.hpp' \) -print \
		| sort \
		| jq -R -s 'split("\n") | map(select(length > 0))'
)"

jq \
	--arg compiler "${COMPILER_PATH}" \
	--arg sdk "${SDK_PATH}" \
	--arg libcxx "${LIBCXX_PATH}" \
	--argjson headers "${HEADER_PATHS_JSON}" \
	'
		def with_toolchain_paths:
			.arguments = ([
				$compiler,
				"-isysroot", $sdk,
				"-isystem", $libcxx
			] + .arguments[1:]);

		(map(with_toolchain_paths)) as $sources
		| ($sources | map(select(.file | endswith("/PointCloudSequenceComponent.cpp"))) | first) as $header_template
		| if $header_template == null then
			error("PCS component compile command was not found")
		else
			$sources + [
				$headers[] as $header
				| ($header_template | .file = $header)
			]
		end
	' \
	"${SOURCE_DATABASE}" > "${TEMP_DATABASE}"

mv "${TEMP_DATABASE}" "${OUTPUT_DATABASE}"
print "Wrote ${OUTPUT_DATABASE}"
