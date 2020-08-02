#ifndef HIERARCHICALPATHFINDER_H
#define HIERARCHICALPATHFINDER_H

#include "Graphs.h"
#include <vector>
#include <functional>
#include <mutex>


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





