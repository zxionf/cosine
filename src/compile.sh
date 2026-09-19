for f in shaders/*.vert shaders/*.frag; do
    glslc $f -o $f.spv
done
cp -r ./shaders ../build/src/shaders