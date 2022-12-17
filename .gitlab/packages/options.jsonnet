{
	// Base Docker image
	baseImage: 'iqrftech/debian-c-builder',
	// Directory for package deployment
	deployDir: '/data/nginx/dl/iqrf-gateway-daemon/${DIST}/${ARCH_PREFIX}${ARCH}/${STABILITY}/',
	// Commands for package build
	packageBuildCommands(distribution, stability): [
		'python3 src/start-IqrfDaemon/iqrfRepoCache/update-cache.py',
		'export DAEMON_VERSION=' + (if stability == 'devel' then '"v2.4.0-beta~${CI_PIPELINE_ID}"' else '"$CI_COMMIT_TAG"'),
		'debuild' + (if stability == 'devel' then ' -e PIPELINE_ID="$CI_PIPELINE_ID"' else '') +
		' -e CCACHE_COMPILERCHECK -e CCACHE_COMPRESS -e CCACHE_BASEDIR -e CCACHE_DIR -e USE_CCACHE=TRUE -e DAEMON_VERSION -b -uc -us -tc'
	],
	// CCache support
	ccache: true,
}
