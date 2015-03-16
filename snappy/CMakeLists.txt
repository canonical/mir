configure_file(package.yaml.in package.yaml)
install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/package.yaml
  readme.md
  DESTINATION meta/
)
