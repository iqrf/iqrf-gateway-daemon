{
	// Base Docker image
	baseImage: 'iqrftech/debian-c-builder',
	// Directory for package deployment
	deployDir: '/data/nginx/dl/iqrf-gateway-daemon/${DIST}/${ARCH_PREFIX}${ARCH}/${STABILITY}/',
	// Commands for package build
	packageBuildCommands(distribution, stability): [
		'python3 src/start-IqrfDaemon/iqrfRepoCache/update-cache.py',
		'debuild' + (if stability == 'devel' then ' -e PIPELINE_ID="$CI_PIPELINE_ID"' else '') +
		' -e CCACHE_COMPILERCHECK -e CCACHE_COMPRESS -e CCACHE_BASEDIR -e CCACHE_DIR -e USE_CCACHE=TRUE -b -uc -us -tc'
	],
	// CCache support
	ccache: true,
}
