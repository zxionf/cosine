#pragma once

#include "vertex.hpp"
#include "shape_maker.hpp"

#include <vector>

namespace xel::graphics
{
    class Triangle
    {
    public:
        Triangle() {
            vertices = ShapeMaker::makePolygonVertices(64, 0.4f, 0);
            indices = ShapeMaker::makeFanIndices(vertices.size());
            // vertices.push_back(Vertex{glm::vec2{0.f, 0.f}, glm::vec4{1.0f, 0.0f, 0.0f, 1.0f}, glm::vec2{0.5f, 1.0f}});
            // vertices.push_back(Vertex{glm::vec2{20.f, 0.f}, glm::vec4{0.0f, 1.0f, 0.0f, 1.0f}, glm::vec2{1.0f, 0.0f}});
            // vertices.push_back(Vertex{glm::vec2{0.f, 20.f}, glm::vec4{0.0f, 0.0f, 1.0f, 1.0f}, glm::vec2{0.0f, 0.0f}});
            // indices = {0, 1, 2};
        }
        ~Triangle() {
            vertices.clear();
            indices.clear();
        }

        std::vector<Vertex> get_vertices() const { return vertices; }
        std::vector<uint16_t> get_indices() const { return indices; }
    private:
        std::vector<Vertex> vertices;
        std::vector<uint16_t> indices;
    };

} // namespace xel::graphics
