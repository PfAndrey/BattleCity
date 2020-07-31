#pragma once

#ifndef TILEMAP_H
#define TILEMAP_H

#include "Geometry.h"
#include <vector>
#include <functional>
#include <fstream>
#include <mutex>

const static Vector directions[] = { Vector::zero,Vector::left, Vector::up,Vector::down,Vector::right };

template<typename T>
using AllowedCellPredicate = std::function<bool(const T&)>;

template<typename T>
class TileMap
{
public:
	TileMap(int width, int height)
	{
		m_width = width;
		m_height = height;
		m_map = new T*[width];
		for (int x = 0; x < width; ++x)
		{
			m_map[x] = new T[height];
		    
	     	for (int i = 0; i < height; ++i)
				m_map[x][i] = T();
		}
	}
	~TileMap()
	{
		for (int x = 0; x < m_width; ++x)
			delete m_map[x];
		delete m_map;
	}
	inline void setCell(int x, int y, T value)
	{
		assert(x < m_width && y < m_height && x >= 0 && y >= 0);
		m_map[x][y] = value;
	}
	inline const T& getCell(int x, int y) const
	{
		assert(x < m_width && y < m_height && x >= 0 && y >= 0);
		return m_map[x][y];
	}
	inline const T& getCell(const Vector& point) const
	{
		assert(point.x < m_width && point.y < m_height && point.x >= 0 && point.y >= 0);
		return m_map[(int)point.x][(int)point.y];
	}
	void clear(T value = T())
	{
		for (int x = 0; x < m_width; ++x)
			for (int y = 0; y < m_height; ++y)
				m_map[x][y] = value;
	}
	inline int width() const
	{
		return m_width;
	}
	inline int height() const
	{
		return m_height;
	}
	void fillRect(int x1, int y1, int width, int height, T value)
	{
		for (int x = x1; x < x1 + width; ++x)
			for (int y = y1; y < y1 + height; ++y)
				setCell(x, y, value);
	}
	void loadFromString(std::map<char, T> dictionary, const std::string& str)
	{
		clear();
		assert(width()*height() == str.length());
		int i = 0;
		for (size_t y = 0; y < height(); ++y)
			for (size_t x = 0; x < width(); ++x)
				setCell(x, y, dictionary[str[i++]]);
	}
	void loadFromString(std::function<T(char)> fabric, const std::string& str)
	{

		clear();

		assert(width()*height() == str.length());
		int i = 0;
		for (int y = 0; y < height(); ++y)
			for (int x = 0; x < width(); ++x)
				setCell(x, y, fabric(str[i++]));
	}
	void loadFromFile(std::map<char, T> dictionary, const std::string& FilePath)
	{
		std::ifstream file;

		file.open(FilePath);

		if (!file.is_open())
			throw std::runtime_error("Can't load file: " + FilePath);


		std::string str;
		for (int y = 0; y < height(); ++y)
		{
			std::getline(file, str);
			assert(str.length() == m_width);
			for (int x = 0; x < m_width; ++x)
			{
				assert(dictionary.find(str[x]) != dictionary.end());
				m_map[x][y] = dictionary[str[x]];
			}
		}
	}
	bool inBounds(const Vector& cell) const
	{
		return cell.x >= 0 && cell.y >= 0 && cell.x < m_width && cell.y < m_height;
	}
	std::vector<Vector> getCells(T cell_type)
	{
		std::vector<Vector> cells;

		for (int x = 0; x < m_width; ++x)
			for (int y = 0; y < m_height; ++y)
				if (m_map[x][y] == cell_type)
					cells.emplace_back(x, y);
		return 	cells;
	}
	std::vector<std::pair<Vector,T>> getCells(const Rect& rect)
	{
		std::vector<std::pair<Vector, T>> cells;
		for (int x = rect.left(); x < rect.right(); ++x)
			for (int y = rect.top(); y < rect.bottom(); ++y)
				cells.push_back(std::make_pair<>({ x,y }, getCell(x, y)));
		return cells;
	}
	Vector traceLine(const Vector& start_cell, const Vector& direction, const AllowedCellPredicate<T>& allowed_cell)
	{
		Vector curr_cell = floor(start_cell);
		if (!allowed_cell(getCell(curr_cell)))
			return curr_cell;
		
		//assert(allowed_cell(getCell(curr_cell)));

		if (direction == Vector::zero)
			return curr_cell;

		while (inBounds(curr_cell) && allowed_cell(getCell(curr_cell)))
			curr_cell += direction;

		curr_cell += -direction;
		assert(allowed_cell(getCell(curr_cell)));
		return curr_cell;
	}
	Vector getCell(const Vector& start_cell, const Vector& direction, int length)
	{
		Vector cur_cell = start_cell;
		for (int i = 0; i < length; ++i)
			cur_cell +=  direction;
		return cur_cell;
	}
	std::vector<Vector> getNeighborNodes(const Vector& start_cell, const T& allowedCellType)
	{
		Vector curr_cell;
		std::vector<Vector> nodes;

		for (int i = 1; i < 5; ++i)
		{
			curr_cell = start_cell;
			while ((curr_cell == start_cell || getCellDegree(curr_cell, allowedCellType) < 3) && inBounds(curr_cell + directions[i]) && getCell(curr_cell + directions[i]) == allowedCellType)
				curr_cell += directions[i];
			if (curr_cell != start_cell && curr_cell.x != 0 && curr_cell.x != m_width - 1)
				nodes.push_back(curr_cell);
		}

		return nodes;
	}
	TileMap& operator=(const TileMap& other_map)
	{
		for (int x = 0; x < m_width; ++x)
			for (int y = 0; y < m_height; ++y)
				setCell(x, y, other_map.getCell(x, y));
		m_width = other_map.m_width;
		m_height = other_map.m_height;
		return *this;
	}

	std::vector<Vector> findPath(const Vector& start_cell, const Vector& finish_cell, const AllowedCellPredicate<T>& is_allowed_cell, int unit_size = 1, const Rect& claster_rect = Rect())
	{
		Vector start = floor(start_cell);
		Vector finish = floor(finish_cell);

		assert(is_allowed_cell(getCell(start)));
		assert(is_allowed_cell(getCell(finish)));

		Rect bounds;
		if (claster_rect == Rect())
		{
			bounds = Rect(0, 0, m_width - unit_size, m_height - unit_size);
		}
		else
		{
			bounds = claster_rect;
		}

		auto getHeuristicValue = [&finish](Vector v) {return abs(v.x - finish.x) + abs(v.y - finish.y); };

		struct Info
		{
			Vector parent;
			int value = 0;
			int base_value = 0;
			bool in_closed_list = false;
		};

		std::unordered_map<Vector, Info> info_list;

		auto cmp = [&info_list](const Vector& a, const Vector& b) { return info_list[a].value > info_list[b].value; };
		std::priority_queue<Vector, std::vector<Vector>, decltype(cmp)> open_list(cmp);

		static const Vector deltas[] = { { 1,0 }, { 0,1 }, { -1,0 }, { 0,-1 } };

		open_list.push(start);

		bool finded = false;
		Vector current_cell;
		while (!open_list.empty())
		{
			current_cell = open_list.top();
			open_list.pop();
			info_list[current_cell].in_closed_list = true;


			if (current_cell == finish)
				break;

			for (auto& delta : deltas)
			{
				const Vector neighbor_cell = current_cell + delta;
				if (bounds.isContain(neighbor_cell) &&
					isEqualRect(neighbor_cell.x, neighbor_cell.y, unit_size, unit_size, is_allowed_cell) && !info_list[neighbor_cell].in_closed_list)
				{
					Info& neighbor = info_list[neighbor_cell];
					neighbor.parent = current_cell;
					neighbor.base_value = info_list[current_cell].base_value + 10;
					neighbor.value = neighbor.base_value + getHeuristicValue(neighbor_cell);
					open_list.push(neighbor_cell);
				}
			}
		}

		std::vector<Vector> path;
		if (current_cell == finish)
		{
			while (current_cell != start)
			{
				path.push_back(current_cell);
				current_cell = info_list[current_cell].parent;
			}

			path.push_back(start);

			for (int i = 0; i < path.size() / 2; ++i)
				std::swap(path[i], path[path.size() - i - 1]);

			std::vector<Vector> optimized_path;

			optimized_path.push_back(path.front());
			for (int i = 1; i < path.size() - 1; ++i)
			{
				if (path[i - 1].x != path[i + 1].x && path[i - 1].y != path[i + 1].y)
					optimized_path.push_back(path[i]);
			}
			optimized_path.push_back(path.back());
			return optimized_path;

		}
		return path;
	}

	bool isEqualRect(int x, int y, int w, int h, const AllowedCellPredicate<T>& is_allowed_cell) const
	{
		for (int Y = y; Y < y + h; ++Y)
			for (int X = x; X < x + w; ++X)
				if (!is_allowed_cell(getCell(X, Y)))
					return false;
		return true;
	}

private:
	T** m_map;
	int m_height,m_width;
};


template <typename T>
class Path_Finder
{
public:
	std::vector<Vector> findPath(const Vector& start_cell, const Vector& finish_cell, const AllowedCellPredicate<T>& is_allowed_cell, int unit_size = 1, const Rect& claster_rect = Rect())
	{
		Vector start = floor(start_cell);
		Vector finish = floor(finish_cell);

		assert(is_allowed_cell(getCell(start)));
		assert(is_allowed_cell(getCell(finish)));

		Rect bounds;
		if (claster_rect == Rect())
		{
			bounds = Rect(0, 0, m_width - unit_size, m_height - unit_size);
		}
		else
		{
			bounds = claster_rect;
		}

		auto getHeuristicValue = [&finish](Vector v) {return abs(v.x - finish.x) + abs(v.y - finish.y); };

		struct Info
		{
			Vector parent;
			int value = 0;
			int base_value = 0;
			bool in_closed_list = false;
		};

		std::unordered_map<Vector, Info> info_list;

		auto cmp = [&info_list](const Vector& a, const Vector& b) { return info_list[a].value > info_list[b].value; };
		std::priority_queue<Vector, std::vector<Vector>, decltype(cmp)> open_list(cmp);

		static const Vector deltas[] = { { 1,0 }, { 0,1 }, { -1,0 }, { 0,-1 } };

		open_list.push(start);

		bool finded = false;
		Vector current_cell;
		while (!open_list.empty())
		{
			current_cell = open_list.top();
			open_list.pop();
			info_list[current_cell].in_closed_list = true;


			if (current_cell == finish)
				break;

			for (auto& delta : deltas)
			{
				const Vector neighbor_cell = current_cell + delta;
				if (bounds.isContain(neighbor_cell) &&
					isEqualRect(neighbor_cell.x, neighbor_cell.y, unit_size, unit_size, is_allowed_cell) && !info_list[neighbor_cell].in_closed_list)
				{
					Info& neighbor = info_list[neighbor_cell];
					neighbor.parent = current_cell;
					neighbor.base_value = info_list[current_cell].base_value + 10;
					neighbor.value = neighbor.base_value + getHeuristicValue(neighbor_cell);
					open_list.push(neighbor_cell);
				}
			}
		}

		std::vector<Vector> path;
		if (current_cell == finish)
		{
			while (current_cell != start)
			{
				path.push_back(current_cell);
				current_cell = info_list[current_cell].parent;
			}

			path.push_back(start);

			for (int i = 0; i < path.size() / 2; ++i)
				std::swap(path[i], path[path.size() - i - 1]);

			std::vector<Vector> optimized_path;

			optimized_path.push_back(path.front());
			for (int i = 1; i < path.size() - 1; ++i)
			{
				if (path[i - 1].x != path[i + 1].x && path[i - 1].y != path[i + 1].y)
					optimized_path.push_back(path[i]);
			}
			optimized_path.push_back(path.back());
			return optimized_path;

		}
		return path;
	}
};


template <typename T>
class HPA_Finder
{
public:
	HPA_Finder(const AllowedCellPredicate<T>& allowed_cell) :
		allowed_cell_pred(allowed_cell)
	{

	}

	void build(TileMap<T>* map, int _claster_size, int _unit_size)
	{
		m_mutex.lock();

		m_map = map;
		claster_size = _claster_size;
		unit_size = _unit_size;

		m_abstract_graph.clear();
		m_clasters.clear();
		m_trans_points.clear();

		const int map_w = m_map->width();
		const int map_h = m_map->height();

		// I. DIVIDE MAP INTO CLASTERS
		for (int x = 0; x <= map_w; x += claster_size)
		{
			for (int y = 0; y <= map_h; y += claster_size)
			{
				m_clasters.emplace_back(x, y, std::min(claster_size, map_w - x), std::min(claster_size, map_h - y));
			}
		}

		// II. FIND ENTRANCES AND CREATE INTER-EDGES
		std::vector<Vector> buffer;
		enum { vertical = 0, horizontal = 1 };
		auto flush_buffer = [this, &buffer](const Vector& claster, int orientation)
		{
			if (!buffer.empty())
			{
				Vector center = floor((buffer.front() + buffer.back()) / 2);
				Vector& A = center;
				Vector  B = center + Vector(orientation, !orientation);

				m_trans_points[claster].push_back(A);
				m_trans_points[claster + Vector(orientation, !orientation) * claster_size].push_back(B);

				Verticle* a = m_abstract_graph.getVerticleByPos(A);
				Verticle* b = m_abstract_graph.getVerticleByPos(B);
				if (!a) a = m_abstract_graph.addVerticle(A);
				if (!b) b = m_abstract_graph.addVerticle(B);

				m_abstract_graph.addEdge(a, b, edge_cost);
				buffer.clear();
			}
		};

		for (auto& block : m_clasters)
		{
			//right side
			int x = block.right() - 1;
			if (x + 1 < map_w)
				for (int y = block.top(); y < block.bottom(); ++y)
					if (y < map_h && m_map->isEqualRect(x, y, unit_size, unit_size, allowed_cell_pred) &&
						m_map->isEqualRect(x + 1, y, unit_size, unit_size, allowed_cell_pred))
						buffer.emplace_back(x, y);
					else
						flush_buffer(block.leftTop(), horizontal);

			flush_buffer(block.leftTop(), horizontal);

			//bottom side
			int y = block.bottom() - 1;
			if (y + 1 < map_h)
				for (int x = block.left(); x < block.right(); ++x)
					if (x < map_w && m_map->isEqualRect(x, y, unit_size, unit_size, allowed_cell_pred) &&
						m_map->isEqualRect(x, y + 1, unit_size, unit_size, allowed_cell_pred))
						buffer.emplace_back(x, y);
					else
						flush_buffer(block.leftTop(), vertical);

			flush_buffer(block.leftTop(), vertical);
		}

		//III. FIND PATHS BETWEEN INTER-EDGES 
		auto findEdges = [this](const Rect& block) -> std::vector<Edge*>
		{
			std::vector<Edge*> edges;
			auto& ls = m_trans_points[block.leftTop()]; //get_s inter_edges verticles for each claster

			if (!ls.empty())
				for (auto it = ls.begin(); it != ls.end(); ++it)
				{
					for (auto it2 = it; it2 != ls.end(); ++it2)
					{
						if (it != it2)
						{
							auto a = m_abstract_graph.getVerticleByPos(*it);
							auto b = m_abstract_graph.getVerticleByPos(*it2);
							assert(a, b);

							auto path = m_map->findPath(*it, *it2, allowed_cell_pred, unit_size, block);

							Edge* edge = new Edge(a, b, getLength(path) * edge_cost);

							if (!path.empty())
								edges.push_back(edge);
						}
					}
				}
			return edges;
		};

		std::vector<std::future<std::vector<Edge*>>> futures;


		for (auto& block : m_clasters)
		{
			auto edges = std::async(findEdges, block);
			futures.push_back(std::move(edges));
		}

		for (auto& future : futures)
		{
			auto edges = future.get();
			for (auto& edge : edges)
			{
				m_abstract_graph.addEdge(edge);
			}
		}

		m_mutex.unlock();
	}

	std::vector<Vector> search(Vector start, Vector finish)
	{
		//IV. Inject Finish and Start verticles into abstract graph  
		const int arr_size = 2;
		bool need_remove[] = { false,false };
		const Vector injection_verticles_pos[] = { start,finish };
		for (int i = 0; i < arr_size; ++i)
		{
			auto& verticle_pos = injection_verticles_pos[i];
			const Vector claster = floor(verticle_pos / claster_size) * claster_size;
			if (!m_abstract_graph.getVerticleByPos(verticle_pos)) //already exsist (equal inter_edge's vertricle)
			{
				need_remove[i] = true;
				Verticle* ptr = m_abstract_graph.addVerticle(verticle_pos);
				Rect block(claster.x, claster.y, claster_size, claster_size);
				const auto& verticles_on_claster = m_trans_points[claster];
				for (auto& v : verticles_on_claster)
				{
					auto path = m_map->findPath(verticle_pos, v, allowed_cell_pred, unit_size, block);
					if (!path.empty())
					{
						const auto& verticle = m_abstract_graph.getVerticleByPos(v);
						m_abstract_graph.addEdge(ptr, verticle, getLength(path) * edge_cost);
					}
				}
			}
		}

		//V. Find abstract path
		auto abstract_path = m_abstract_graph.findPath(m_abstract_graph.getVerticleByPos(start),
			m_abstract_graph.getVerticleByPos(finish));

		//VI. Refinement abstarct path
		std::vector<Vector> refinement_path;

		std::vector<std::future<std::vector<Vector>>> futures;

		for (int i = 1; i < abstract_path.size(); ++i)
		{
			Vector TopLeftClaster = floor(abstract_path[i]->position() / claster_size) * claster_size;
			Rect claster(TopLeftClaster, Vector(claster_size, claster_size));

			auto future = std::async(std::launch::async, &TileMap<ETiles>::findPath, m_map, abstract_path[i - 1]->position(), abstract_path[i]->position(),
				allowed_cell_pred, unit_size, claster);

			futures.push_back(std::move(future));
		}

		for (auto& future : futures)
		{
			auto path = future.get();
			refinement_path.insert(refinement_path.end(), path.begin(), path.end());
			refinement_path.pop_back();
		}

		for (int i = 0; i < refinement_path.size() / 2; ++i)
			std::swap(refinement_path[i], refinement_path[refinement_path.size() - i - 1]);

		//for (int i = 1; i < refinement_path.size(); ++i)
			//assert(refinement_path[i] != refinement_path[i - 1]);

		//clean-up injected verticles from absract graph
		for (int i = 0; i < arr_size; ++i)
			if (need_remove[i])
				m_abstract_graph.removeVerticle(m_abstract_graph.getVerticleByPos(injection_verticles_pos[i]));

		return refinement_path;
	}

	void update()
	{
		build(m_map, claster_size, unit_size);
	}

private:
	TileMap<T>* m_map;
	int claster_size;
	int unit_size;
	const int edge_cost = 10;
	Graph m_abstract_graph;
	std::vector<Rect> m_clasters;
	std::map<Vector, std::list<Vector>> m_trans_points;
	friend class CHPAVisualiser;
	std::mutex m_mutex;
	const AllowedCellPredicate<T>& allowed_cell_pred;
};
#endif





