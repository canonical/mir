target_sources(mirplatformwayland-graphics PRIVATE
    xdg-shell-client.c                      xdg-shell-client.h
)

mir_disable_tsan_for_c_target(mirplatformwayland-graphics)
