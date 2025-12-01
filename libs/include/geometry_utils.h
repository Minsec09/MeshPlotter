#pragma once
#include <vector>
#include <utility>
#include <array>

struct FaceProperties {
    double area;
    double center_x;
    double center_y;
    double center_z;
};

namespace cgal_tools {
	/// <summary>
	/// 输入点边信息，返回mesh网格
	/// </summary>
	/// <param name="points">二维数组[num_points, 3]，表示num_points个点的三维坐标</param>
	/// <param name="edges">二维数组[num_edges, 3]，表示num_edges个边的组成点，以及该边是不是弧线</param>
	/// <param name="edges_info">二维数组[num_edges, 3]，这条弧线边上第三点的坐标（直线全为0）</param>
	/// <returns>pair (faces[num_faces, n], properties) properties 字段为area, center_x, center_y, center_z</returns>
	std::pair<std::vector<std::vector<int>>, std::vector<FaceProperties>>  
		reconstruct_meshes(const std::vector<std::array<double, 3>>& points,
							const std::vector<std::array<int, 3>>& edges,
							const std::vector<std::array<double, 3>>& edges_info);
}