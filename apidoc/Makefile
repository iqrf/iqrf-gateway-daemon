.PHONY: doc

clean:
	@echo "Cleaning up workspace"
	@rm -rf public

doc:
	@echo "Building documentation"
	@hugo
	@cd public/ && cp -r * /data/nginx/apidocs/iqrf-gateway-daemon
