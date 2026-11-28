# 开发期工具（不参与运行时、不进打包）。
#
# mtsdf_gen：把内置几何烘焙成 MTSDF 图标图集（PNG + JSON）。
# 生成物提交在 assets/sdf/ 下，构建时不需要再跑它。
add_executable(mtsdf_gen mtsdf_gen/main.cpp)
target_link_libraries(mtsdf_gen PRIVATE msdfgen-core)
target_include_directories(mtsdf_gen PRIVATE ${STB_INCLUDE_DIR})
