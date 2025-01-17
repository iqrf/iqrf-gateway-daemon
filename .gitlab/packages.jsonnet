local distributions = import 'packages/distributions.jsonnet';
local options = import 'packages/options.jsonnet';

// Docker image suffix
local imageSuffix(distribution, arch=null) = distribution.name + '-' + distribution.version + if arch != null then if arch != 'all' then '-' + arch else '' else if distribution.arch != 'all' then '-' + distribution.arch else '';

// Machine architecture
local machineArch(arch) = if arch == 'all' then 'amd64' else arch;

// Export environment variable for given stability
local stabilityEnvironmentVars(stability) = (
if stability == 'devel' then
	'export STABILITY="iqaros-devel"'
else
	'[[ "$CI_COMMIT_TAG" =~ ^.*-(alpha|beta|rc)[0-9]*$ ]] && export STABILITY="iqaros-testing" || export STABILITY="iqaros-stable"'
);

local image(distribution) = {
	image: options.baseImage + ':' + imageSuffix(distribution, machineArch(distribution.arch)),
	variables: {
		ARCH: distribution.arch,
		DIST: distribution.version,
	} + if distribution.name == 'raspbian' then {
		ARCH_PREFIX: 'rpi-',
		CFLAGS: '-marm -march=armv6zk -mcpu=arm1176jzf-s -mfloat-abi=hard -mfpu=vfp',
		CXXFLAGS: '-marm -march=armv6zk -mcpu=arm1176jzf-s -mfloat-abi=hard -mfpu=vfp',
	} else {},
	tags: ['linux', machineArch(distribution.arch)],
};

local buildPackageJob(distribution, stability) = {
	stage: 'build',
	before_script: [
		'git checkout -B "$CI_COMMIT_REF_NAME" "$CI_COMMIT_SHA"',
		'git submodule init',
		'git submodule update',
		stabilityEnvironmentVars(stability),
	] + if options.ccache then [
		'export CCACHE_COMPILERCHECK="content"',
		'export CCACHE_COMPRESS="true"',
		'export CCACHE_BASEDIR="$PWD"',
		'export CCACHE_DIR="${CCACHE_BASEDIR}/.ccache"',
		'ccache --zero-stats --show-stats',
		'echo "CCACHEDIR=~/.ccache" > ~/.pbuilderrc',
	] else [],
	script: [
		'rm -rf ../*.buildinfo ../*.changes ../*.deb ../*.ddeb',
		'export DATE=`git show --no-patch --format=%cD ${CI_COMMIT_SHA}`',
	] + (
	if stability == 'devel' then
		['gbp dch -a -S --snapshot-number="$CI_PIPELINE_ID" --ignore-branch']
	else []
	) + [
		'dch-add-distro -d "${DIST}" -D "${DATE}"',
	] + options.packageBuildCommands(distribution, stability) + [
		'rm -rf packageDeploy && mkdir packageDeploy',
		'mv ../*.buildinfo ../*.changes ../*.deb packageDeploy',
		'mv ../*.ddeb packageDeploy 2>/dev/null || :',
		'sed -i "s/.ddeb/.deb/g" packageDeploy/*.changes',
		'for file in $(ls packageDeploy/*.ddeb) ; do mv "${file}" "${file%.ddeb}.deb"; done',
	],
	artifacts: {
		paths: [
			'packageDeploy',
		],
		expire_in: '2 weeks',
	},
} + image(distribution) + (
if stability == 'devel' then {
	except: ['tags'],
} else {
	only: ['tags'],
}
) + (
if options.ccache then {
	after_script:
		[
			'export CCACHE_DIR="${PWD}/.ccache"',
			'ccache --show-stats',
		],
	cache:
		{
			key: '$CI_JOB_NAME',
			paths: ['.ccache/'],
		},
} else {}
);

local deployPackageJob(distribution, stability) = {
	stage: 'deploy',
	before_script: [
		'mkdir -p ~/.ssh',
		'chmod 700 ~/.ssh',
		'eval $(ssh-agent -s)',
		'echo "$SSH_PRIVATE_KEY" | tr -d \'\r\' | ssh-add - > /dev/null',
		'echo "$SSH_KNOWN_HOSTS" > ~/.ssh/known_hosts',
		'chmod 644 ~/.ssh/known_hosts',
	],
	needs: [
		{
			job: 'build-package_' + stability + '/' + imageSuffix(distribution),
			artifacts: true,
		},
	],
} + image(distribution) + (
if stability == 'devel' then {
	except: ['tags'],
	only: {
		refs: ['master', 'v2.99.x'],
	},
	script: [
		stabilityEnvironmentVars(stability),
		'ssh www-deploy@icinga.iqrf.org "rm -f ' + options.deployDir + '*"',
		'rsync -hrvz --delete -e ssh packageDeploy/* www-deploy@icinga.iqrf.org:' + options.deployDir + '',
	],
} else {
	only: ['tags'],
	script: [
		stabilityEnvironmentVars(stability),
		'ssh www-deploy@icinga.iqrf.org "mkdir -p ' + options.deployDir + 'old && find ' + options.deployDir + ' -maxdepth 1 -type f -exec mv {} ' + options.deployDir + 'old \\;"',
		'rsync -hrvz --delete -e ssh packageDeploy/* www-deploy@icinga.iqrf.org:' + options.deployDir + '',
	],
}
);

{
	['build-package_' + stability + '/' + imageSuffix(distribution)]: buildPackageJob(distribution, stability)
	for distribution in distributions
	for stability in ['devel', 'release']
} + {
	['deploy-package_' + stability + '/' + imageSuffix(distribution)]: deployPackageJob(distribution, stability)
	for distribution in distributions
	for stability in ['devel', 'release']
}
