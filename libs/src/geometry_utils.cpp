#include "geometry_utils.h"
// #include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <cmath>
#include <vector>
#include <map>
#include <iostream>
#include <utility>
#include <algorithm>

// #define DATA_PATH "C:/WorkSpace/11_17/codes/CGAL-test/data/"
constexpr auto PI = 3.1415926536;

struct Vector_3;

struct Point_3 {
    double _x, _y, _z;
    Point_3() : _x(0), _y(0), _z(0) {}
    Point_3(double x, double y, double z) : _x(x), _y(y), _z(z) {}

    double x() const { return _x; }
    double y() const { return _y; }
    double z() const { return _z; }
};

// 简单向量类型
struct Vector_3 {
    double x, y, z;
    Vector_3() : x(0), y(0), z(0) {}
    Vector_3(double xx, double yy, double zz) : x(xx), y(yy), z(zz) {}
	// 模长平方
    double squared_length() const { return x * x + y * y + z * z; }
    // 模长
    double length() const { return std::sqrt(squared_length()); }
};

// 点-点 得到向量
inline Vector_3 operator-(const Point_3& a, const Point_3& b) {
    return Vector_3(a.x() - b.x(), a.y() - b.y(), a.z() - b.z());
}

// 向量加向量
inline Vector_3 operator+(const Vector_3& a, const Vector_3& b) {
    return Vector_3(a.x + b.x, a.y + b.y, a.z + b.z);
}

// 向量减向量
inline Vector_3 operator-(const Vector_3& a, const Vector_3& b) {
    return Vector_3(a.x - b.x, a.y - b.y, a.z - b.z);
}

// 向量 × 标量
inline Vector_3 operator*(const Vector_3& v, double s) {
    return Vector_3(v.x * s, v.y * s, v.z * s);
}
inline Vector_3 operator*(double s, const Vector_3& v) {
    return v * s;
}

// 向量 / 标量
inline Vector_3 operator/(const Vector_3& v, double s) {
    return Vector_3(v.x / s, v.y / s, v.z / s);
}

// 点 + 向量 -> 点
inline Point_3 operator+(const Point_3& p, const Vector_3& v) {
    return Point_3(p.x() + v.x, p.y() + v.y, p.z() + v.z);
}

//typedef CGAL::Exact_predicates_inexact_constructions_kernel Kernel;
//typedef Kernel::Point_3 Point_3;
//typedef Kernel::Vector_3 Vector_3;
// typedef CGAL::Surface_mesh<Point_3> Mesh;

// 储存边信息，包括点索引、是否弧线，第三点坐标
struct Edge { 
    int point1, point2; 
	int is_arc = 0; // 0 直线，1 弧线
	Point_3 arc_center;
	// double arc_angle = 0.0; // 弧度
    // 构造直线边
    Edge(int p1, int p2)
        : point1(p1), point2(p2),
        is_arc(0), arc_center(0, 0, 0) {
    }

    // 构造弧线边
    Edge(int p1, int p2, int isarc, const Point_3& center)
        : point1(p1), point2(p2),
        is_arc(isarc), arc_center(center){
    }
};

static constexpr double EPS = 1e-6;

// 向量运算
static Vector_3 vecSub(const Point_3& a, const Point_3& b) { return a - b; }

static Vector_3 vecCross(const Vector_3& a, const Vector_3& b) {
    return Vector_3(
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    );
}
static Vector_3 vecAdd(const Vector_3& a, const Vector_3& b) {
    return Vector_3(
        a.x + b.x,
        a.y + b.y,
		a.z + b.z
    );
}

static double vecDot(const Vector_3& a, const Vector_3& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

static double vecNorm(const Vector_3& a) {
    return std::sqrt(a.squared_length());
}

std::map<std::pair<int, int>, int> buildEdgeMap(const std::vector<Edge>& cgal_edges) {
    std::map<std::pair<int, int>, int> edge_map;
    for (size_t i = 0; i < cgal_edges.size(); ++i) {
        int p1 = cgal_edges[i].point1;
        int p2 = cgal_edges[i].point2;
        if (p1 > p2) std::swap(p1, p2); // 保证无向查找
        edge_map[{p1, p2}] = i;
    }
    return edge_map;
}


// 属性计算
// 计算三角形面积
static double computeTriangleArea(const Point_3& p1, const Point_3& p2, const Point_3& p3) {
    Vector_3 v1 = p2 - p1;
    Vector_3 v2 = p3 - p1;
    Vector_3 cross = vecCross(v1, v2);
    return 0.5 * std::sqrt(cross.squared_length());
}

// 计算多边形中心坐标
static Point_3 computeCentroid(const std::vector<Point_3>& points) {
    double sum_x = 0.0, sum_y = 0.0, sum_z = 0.0;
    int n = points.size();

    for (const auto& p : points) {
        sum_x += p.x();
        sum_y += p.y();
        sum_z += p.z();
    }

    return Point_3(sum_x / n, sum_y / n, sum_z / n);
}

// 计算两向量夹角 (0 到 PI)
static double getVecAngle(const Vector_3& a, const Vector_3& b) {
    double n_a = a.length();
    double n_b = b.length();
    if (n_a < 1e-12 || n_b < 1e-12) return 0.0;

    double cos_theta = vecDot(a, b) / (n_a * n_b);
    // 防止浮点误差导致 acos 越界
    if (cos_theta > 1.0) cos_theta = 1.0;
    if (cos_theta < -1.0) cos_theta = -1.0;

    return std::acos(cos_theta);
}

Point_3 getCircleCenter(const Point_3& p1, const Point_3& p2, const Point_3& M) {
    Vector_3 v1 = p2 - p1;
    Vector_3 v2 = M - p1;

    // 法向量检查：三点是否共线
    Vector_3 n = vecCross(v1, v2);
    double area2 = vecNorm(n);
    if (area2 < 1e-12) {
        // 三点几乎共线，无圆心
        return Point_3(std::numeric_limits<double>::quiet_NaN(),
            std::numeric_limits<double>::quiet_NaN(),
            std::numeric_limits<double>::quiet_NaN());
    }

    // 三边长度
    double a = vecNorm(p2 - M);
    double b = vecNorm(p1 - M);
    double c = vecNorm(p1 - p2);

    double a2 = a * a;
    double b2 = b * b;
    double c2 = c * c;

    // 外心 barycentric 系数
    double alpha = a2 * (b2 + c2 - a2);
    double beta = b2 * (c2 + a2 - b2);
    double gamma = c2 * (a2 + b2 - c2);

    double sum = alpha + beta + gamma;

    // 防止除0
    if (std::abs(sum) < 1e-12) {
        return Point_3(std::numeric_limits<double>::quiet_NaN(),
            std::numeric_limits<double>::quiet_NaN(),
            std::numeric_limits<double>::quiet_NaN());
    }

    // 得到圆心
    Point_3 C(
        (alpha * p1.x() + beta * p2.x() + gamma * M.x()) / sum,
        (alpha * p1.y() + beta * p2.y() + gamma * M.y()) / sum,
        (alpha * p1.z() + beta * p2.z() + gamma * M.z()) / sum
    );

    return C;
}
 

// 计算多边形面积(直线)，鞋带公式
static Vector_3 computePolygonArea3D_line_vector(const std::vector<Point_3>& pts)
{
    int n = pts.size();
    if (n < 3) return Vector_3(0, 0, 0);

    Vector_3 sum(0, 0, 0);

    // 获取第一个点作为参考点 (Reference Point)
    const Point_3& p_ref = pts[0];

    for (int i = 0; i < n; ++i) {
        const Point_3& p1 = pts[i];
        const Point_3& p2 = pts[(i + 1) % n];

        // 关键：构建向量时，减去参考点 p_ref
        // 这在数学上等价于原点计算，但能避免大坐标带来的精度丢失
        Vector_3 v1(p1.x() - p_ref.x(), p1.y() - p_ref.y(), p1.z() - p_ref.z());
        Vector_3 v2(p2.x() - p_ref.x(), p2.y() - p_ref.y(), p2.z() - p_ref.z());

        // 累加叉乘结果
        sum = sum + vecCross(v1, v2);
    }

    // 注意：这里直接返回 sum 向量，不要在这里做 0.5 * sqrt
    // 因为我们需要这个向量的方向（xyz）来判断法线
    return sum;
}


static std::pair<std::vector<std::vector<int>>, std::vector<FaceProperties>>
caculate_properties(const std::vector<std::vector<int>>& faces,
    const std::vector<Point_3>& cgal_points,
    const std::vector<Edge>& cgal_edges) {

	// 建立边查找表
    auto edge_lookup = buildEdgeMap(cgal_edges);
    // 创建面属性数组（目前是计算面积和中心点）
    std::vector<FaceProperties> face_props;
    for (const auto& face_indices : faces) {
        if (face_indices.size() < 3) {
            // 无效面默认属性
            face_props.push_back({ 0.0, 0.0, 0.0, 0.0 });
            continue;
        }

        // 将索引转换为实际坐标点
        std::vector<Point_3> face_points;
        for (int idx : face_indices) {
            if (idx >= 0 && idx < cgal_points.size()) {
                face_points.push_back(cgal_points[idx]);
            }
        }

        if (face_points.size() < 3) {
            face_props.push_back({ 0.0, 0.0, 0.0, 0.0 });
            continue;
        }

        // --- 步骤 1: 计算直线多边形的面积向量 ---
        Vector_3 area_vec_double = computePolygonArea3D_line_vector(face_points);
        double area_line_double = area_vec_double.length();
        double area_line = 0.5 * area_line_double;

        // 获取该面的法向量 (用于判断凹凸)
        // 如果面积极小，法向量不可靠，这里做个保护
        Vector_3 face_normal(0, 0, 1);
        if (area_line_double > 1e-9) {
            face_normal = area_vec_double / area_line_double;
        }

        // --- 步骤 2: 计算弧线修正面积 ---
        double area_correction = 0.0;
        int n_pts = face_indices.size();

        for (int i = 0; i < n_pts; ++i) {
            int idx1 = face_indices[i];
            int idx2 = face_indices[(i + 1) % n_pts]; // 下一点，形成闭环

            // 在 lookup 中查找边 (无向)
            int p1_key = idx1;
            int p2_key = idx2;
            if (p1_key > p2_key) std::swap(p1_key, p2_key);

            auto it = edge_lookup.find({ p1_key, p2_key });
            if (it != edge_lookup.end()) {
                const Edge& e = cgal_edges[it->second];

                // 如果是弧线
                if (e.is_arc) {
                    Point_3 P1 = cgal_points[idx1];
                    Point_3 P2 = cgal_points[idx2];
                    // 注意：Edge 结构中 arc_center 实际上存的是弧上一点 M
                    Point_3 M = e.arc_center;

                    // A. 计算圆心
                    Point_3 C = getCircleCenter(P1, P2, M);

                    // 检查圆心是否有效 (NaN check)
                    if (std::isnan(C.x())) {
                        continue; // 三点共线或无效，当做直线处理，无修正
                    }

                    // B. 计算半径
                    double R = (P1 - C).length();

                    // C. 计算圆心角 (Total Angle)
                    // 必须分为 P1->M 和 M->P2 两段计算，以正确处理 > 180 度的优弧
                    Vector_3 v_C_P1 = P1 - C;
                    Vector_3 v_C_M = M - C;
                    Vector_3 v_C_P2 = P2 - C;

                    double angle1 = getVecAngle(v_C_P1, v_C_M);
                    double angle2 = getVecAngle(v_C_M, v_C_P2);
                    double total_angle = angle1 + angle2;

                    // D. 计算弓形面积 (Area Segment)
                    // 扇形面积
                    double area_sector = 0.5 * R * R * total_angle;

                    // 三角形(C, P1, P2) 面积
                    // 使用叉乘模长计算，结果恒为正
                    double area_tri_cp1p2 = 0.5 * vecCross(v_C_P1, v_C_P2).length();

                    // 几何修正：
                    // 如果角度 < 180 (M_PI)，弓形面积 = 扇形 - 三角形
                    // 如果角度 > 180 (M_PI)，弓形面积 = 扇形 + 三角形 (因为三角形面积计算结果是正的，但此时弦将圆切成了两部分，优弧部分包含了圆心)
                    double area_segment = 0.0;
                    if (total_angle > PI) {
                        area_segment = area_sector + area_tri_cp1p2;
                    }
                    else {
                        area_segment = area_sector - area_tri_cp1p2;
                    }

                    // E. 判断正负号 (加还是减)
                    // 依据：弧线是向内凹(减) 还是 向外凸(加)
                    // 方法：计算 (P2-P1) x (M-P1) 与 面法向 的点乘
                    // P1->P2 是当前多边形的边方向
                    Vector_3 v_chord = P2 - P1;
                    Vector_3 v_mid_vec = M - P1;
                    Vector_3 cross_check = vecCross(v_chord, v_mid_vec);

                    double dir = vecDot(cross_check, face_normal);

                    // 逻辑：
                    // 标准逆时针(CCW)多边形，法向朝上。
                    // 向量叉积 (P2-P1)x(M-P1) 服从右手定则。
                    // 如果 M 在 P1->P2 左侧（多边形内部），叉积向上，Dot > 0。
                    // -> 弧线内凹 -> 面积减小。
                    // 如果 M 在 P1->P2 右侧（多边形外部），叉积向下，Dot < 0。
                    // -> 弧线外凸 -> 面积增加。

                    if (dir > 0) {
                        area_correction -= area_segment;
                    }
                    else {
                        area_correction += area_segment;
                    }
                }
            }
        }

        // 3. 汇总结果
        double total_area = area_line + area_correction;
        // 理论上不应小于0，除非几何体自相交严重或数据错误
        if (total_area < 0) total_area = 0.0;

        Point_3 centroid = computeCentroid(face_points);
        // 注意：这里的 centroid 仅基于多边形顶点计算。
        // 如果需要极其精确的物理重心（包含弧线质量分布），计算会复杂得多（需要加权合成弓形重心），
        // 通常几何应用中用顶点平均值已足够。

        face_props.push_back({ total_area, centroid.x(), centroid.y(), centroid.z() });
    }

    return std::make_pair(faces, face_props);
}


namespace cgal_tools {
    std::pair<std::vector<std::vector<int>>, std::vector<FaceProperties>> 
        reconstruct_meshes(const std::vector<std::array<double, 3>>& points,
            const std::vector<std::array<int, 3>>& edges, 
            const std::vector<std::array<double, 3>>& edges_info) {

		int num_points = points.size();
		int num_edges = edges.size();
		std::vector<Edge>     cagl_edges;
		std::vector<Point_3>  cgal_points;

		// 转化为CGAL point3
		for (const auto& point : points) {
			cgal_points.emplace_back(Point_3(point[0], point[1], point[2]));
		}
		// 获取边
        for (int i = 0; i < num_edges; i++) {
			cagl_edges.emplace_back(Edge{
                edges[i][0], edges[i][1], edges[i][2], // 
                Point_3(edges_info[i][0], edges_info[i][1], edges_info[i][2]),
                });
		}
		// 建立每个点的邻接表
		std::vector<std::vector<int>> adj(num_points);
		for (auto& e : cagl_edges) {
			adj[e.point1].push_back(e.point2);
			adj[e.point2].push_back(e.point1); // 无向
		}
        // std::cout << "111" << std::endl;
		// 计算模型中心，以确定后续各点法向量
        Point_3 center = computeCentroid(cgal_points);
		/*Point_3 center(0.0, 0.0, 0.0);
		for (int i = 0; i < num_points; ++i) {
			center = Point_3(
				center.x() + cgal_points[i].x(),
				center.y() + cgal_points[i].y(),
				center.z() + cgal_points[i].z()
			);
		}
		center = Point_3(
			center.x() / num_points,
			center.y() / num_points,
			center.z() / num_points
		);*/

        // 遍历每个点，为其相邻点按 从内到外的法向 逆时针排序
        std::vector<std::vector<int>> sorted_ring(num_points);

        for (int v = 0; v < num_points; ++v) {
            const auto& neis = adj[v];
            int deg = static_cast<int>(neis.size());
            if (deg == 0) continue;      // 孤立点，忽略
            if (deg == 1) {              // 只有一条边，可忽略
                // sorted_ring[v] = neis; 
                continue;
            }
            // 设置模型中心为参考 计算该点向量
            Vector_3 n_v = cgal_points[v] - center;
            if (n_v.squared_length() < EPS) {
                // 退化：顶点刚好等于中心，给个默认方向
                n_v = Vector_3(0.0, 0.0, 1.0);
            }
            n_v = n_v / std::sqrt(n_v.squared_length());

            // 在 n_v 垂直平面上建立局部坐标系 (e1, e2)
            // 先选一条邻接边方向做初始切向方向
            Vector_3 e1 = cgal_points[neis[0]] - cgal_points[v];
            // 投影到切平面
            double proj = vecDot(e1, n_v);
            // std::cout << "dot" << proj << std::endl;
            e1 = e1 - proj * n_v;
            if (e1.squared_length() < EPS && deg >= 2) {
                e1 = cgal_points[neis[1]] - cgal_points[v];
                proj = vecDot(e1, n_v);
                e1 = e1 - proj * n_v;
            }
            if (e1.squared_length() < EPS) {
                // 再退一步给个固定方向并投影
                e1 = Vector_3(1.0, 0.0, 0.0);
                proj = vecDot(e1, n_v);
                e1 = e1 - proj * n_v;
            }
            e1 = e1 / std::sqrt(e1.squared_length());
            Vector_3 e2 = vecCross(n_v, e1);
            e2 = e2 / std::sqrt(e2.squared_length());

            // 计算每个邻居的极角（在以 n_v 为法向的切平面中）
            struct NeighborAngle {
                int idx;
                double angle;
            };
            std::vector<NeighborAngle> tmp;
            tmp.reserve(deg);

            for (int u : neis) {
                Vector_3 d = cgal_points[u] - cgal_points[v];
                // 投影到切平面
                double proj_n = vecDot(d, n_v);
                Vector_3 d_tan = d - proj_n * n_v;
                double x = vecDot(d_tan, e1);
                double y = vecDot(d_tan, e2);
                double angle = std::atan2(y, x);

                tmp.push_back({ u, angle });
            }

            std::sort(tmp.begin(), tmp.end(),
                [](const NeighborAngle& a, const NeighborAngle& b) {
                    return a.angle < b.angle;
                });

            sorted_ring[v].resize(deg);
            for (int i = 0; i < deg; ++i) {
                sorted_ring[v][i] = tmp[i].idx;
            }
        }

       for (int idx = 0; idx < num_points; ++idx) {
            std::cout << "\n点" << idx << "的排序环: ";
            for (int nb : sorted_ring[idx]) {
                std::cout << nb << " ";
            }
        }
        std::cout << std::endl;

        // 构造半边
        int num_halfedges = num_edges * 2;
        std::vector<int> he_tail(num_halfedges);
        std::vector<int> he_head(num_halfedges);
        std::vector<int> he_twin(num_halfedges);
        // (u,v) -> 半边 id
        std::map<std::pair<int, int>, int> halfedge_id;

        for (int ei = 0; ei < num_edges; ++ei) {
            int u = cagl_edges[ei].point1;
            int v = cagl_edges[ei].point2;

            int h_uv = 2 * ei;      // 有向边 u->v
            int h_vu = 2 * ei + 1;  // 有向边 v->u

            he_tail[h_uv] = u;
            he_head[h_uv] = v;
            he_tail[h_vu] = v;
            he_head[h_vu] = u;

            he_twin[h_uv] = h_vu;
            he_twin[h_vu] = h_uv;

            halfedge_id[{u, v}] = h_uv;
            halfedge_id[{v, u}] = h_vu;
        }

        // 把 sorted_ring[v] 转为“按顺序的出半边”
        std::vector<std::vector<int>> out_halfedges(num_points);
        for (int v = 0; v < num_points; ++v) {
            for (int u : sorted_ring[v]) {
                int h = halfedge_id[{v, u}];     // v->u 这条半边
                out_halfedges[v].push_back(h);   // 以 v 为 tail 的半边，按环顺序
            }
        }


        // 记录在每个顶点 v 上，每个邻居 u 在 sorted_ring[v] 中的位置
        std::vector<std::map<int, int>> pos_in_ring(num_points);
        for (int v = 0; v < num_points; ++v) {
            const auto& ring = sorted_ring[v];
            for (int i = 0; i < (int)ring.size(); ++i) {
                int u = ring[i];
                pos_in_ring[v][u] = i;
            }
        }
        // 记录每条有向边是否已被用于某个面： (u,v) -> visited
        std::map<std::pair<int, int>, bool> visited_directed_edges;

        // 存所有找到的面
        std::vector<std::vector<int>> faces;

        // 设置最大面边数
        int max_face_edges = num_edges * 2;

        // 遍历每条有向边 u->v 和 v->u都要走一次
        for (const auto& e : cagl_edges) {
            int a = e.point1;
            int b = e.point2;

            int dirs[2][2] = { {a, b}, {b, a} };

            for (int di = 0; di < 2; ++di) {
                int start_u = dirs[di][0];
                int start_v = dirs[di][1];

                // 如果这条有向边已经属于某个面了，就跳过
                if (visited_directed_edges[{start_u, start_v}]) {
                    continue;
                }

                // 点没有邻接环（孤立或度数过小），跳过
                if (sorted_ring[start_v].empty()) {
                    continue;
                }

                std::vector<int> face;
                face.reserve(16);

                int prev = start_u;
                int cur = start_v;

                face.push_back(start_u);
                bool closed = false;
                bool failed = false;

                while (true) {
                    // 加入当前点
                    face.push_back(cur);

                    // 标记当前有向边 prev->cur 已被使用
                    visited_directed_edges[{prev, cur}] = true;

                    // 在 cur 的环里找到 prev 所在的位置
                    auto it_pos = pos_in_ring[cur].find(prev);
                    if (it_pos == pos_in_ring[cur].end()) {
                        failed = true;
                        break;
                    }

                    const auto& ring = sorted_ring[cur];
                    int deg = (int)ring.size();
                    int idx = it_pos->second;

                    // 一直逆时针walk：取 prev 在环中的前一个邻居
                    int next_idx = (idx - 1 + deg) % deg;
                    int next = ring[next_idx];

                    // 如果下一条边的终点回到起始点 start_u，则闭合
                    if (next == start_u) {
                        // 最后这条边 cur->start_u 也属于这个面，标记已访问
                        visited_directed_edges[{cur, start_u}] = true;
                        closed = true;
                        break;
                    }

                    // 如果下一条有向边已经被用在别的面里了，这条 walk 放弃
                    if (visited_directed_edges[{cur, next}]) {
                        failed = true;
                        break;
                    }
                    // 超过限定的边数
                    if ((int)face.size() > max_face_edges) {
                        failed = true;
                        break;
                    }

                    prev = cur;
                    cur = next;
                }

                if (closed && !failed && (int)face.size() >= 3) {
                    faces.push_back(face);
                }
            }
        }
        //std::cout << "\n找到的面数量: " << faces.size() << "\n";
        //for (int i = 0; i < (int)faces.size(); ++i) {
        //    const auto& f = faces[i];
        //    std::cout << "面-" << i << " (" << f.size() << "个顶点): ";
        //    for (int v : f) {
        //        std::cout << v << " ";
        //    }
        //    std::cout << "\n";
        //}
        // faces [n_meshes, n] n是点索引
        // 计算面积和中心点坐标并返回
       
        return caculate_properties(faces, cgal_points, cagl_edges);
	}
}
