configure_file(package.yaml.in package.yaml)
install(FILES package.yaml readme.md DESTINATION meta/)
