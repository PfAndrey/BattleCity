#include "Graphs.h"

Edge::Edge(Verticle* begin, Verticle* end, int _value)
{
value = _value;
setBegin(begin);
setEnd(end);
}

Verticle* Edge::beginVerticle()
{
	return m_begin;
}

Verticle* Edge::endVerticle()
{
	return m_end;
}

void Edge::setBegin(Verticle* verticle)
{
	m_begin = verticle;
}

void Edge::setEnd(Verticle* verticle)
{
	Edge::m_end = verticle;
}

bool Edge::isAdjacent(Verticle* verticle)
{
	if (m_begin == verticle || m_end == verticle)
	{
		return true;
	}
	return false;
}

//----------------------------------------------------------------------------------

void Verticle::connectEdge(Edge* edge)
{
	m_edges.push_back(edge);
}

void Verticle::disconnectEdge(Edge* edge)
{
	auto it = std::find(m_edges.begin(), m_edges.end(), edge);
	if (it != m_edges.end())
		m_edges.erase(it);
}

std::vector<Edge*>::const_iterator Verticle::edges_begin() const
{
	return m_edges.cbegin();
}

std::vector<Edge*>::const_iterator Verticle::edges_end() const
{
	return m_edges.cend();
}

std::vector<Verticle*> Verticle::getIncidentVerticles()
{
	std::vector<Verticle*> result;

	for (auto& e : m_edges)
		if (e->beginVerticle() == this)
			result.push_back(e->endVerticle());
		else
			result.push_back(e->beginVerticle());
	return result;
}

bool Verticle::isAdjacent(Edge* edge)
{
	for (auto& e : m_edges)
		if (e == edge)
			return true;
	return false;
}

void Verticle::clearEdges()
{

}

std::vector<Edge*> Verticle::getEdgesList() const
{
	return m_edges;
}

Vector Verticle::position() const
{
	return m_position;
}

void Verticle::setPosition(const Vector& vec)
{
	m_position = vec;
}

//----------------------------------------------------------------------------------

void Graph::clear()
{
	for (auto v : m_verticles)
		delete v;
	for (auto e : m_edges)
		delete e;
	m_edges.clear();
	m_verticles.clear();
	pos_to_verticles.clear();
}
	
Verticle* Graph::addVerticle(const Vector& pos)
{
	assert(!pos_to_verticles[pos]); //already exist
	Verticle* verticle = new Verticle();
	verticle->setPosition(pos);
	m_verticles.push_back(verticle);
	pos_to_verticles[pos] = verticle;
	return verticle;
}

Verticle* Graph::getVerticleByPos(const Vector& pos)
{
	return pos_to_verticles[pos];
}

Verticle* Graph::getVerticleByPos(float x, float y)
{
	return getVerticleByPos({ x,y });
}

void Graph::removeEdge(Edge* e)
{
	assert(e);
	auto it = std::find(m_edges.begin(), m_edges.end(), e);

	if (it != m_edges.end())
	{
		if (e->beginVerticle()) e->beginVerticle()->disconnectEdge(e);
		if (e->endVerticle()) e->endVerticle()->disconnectEdge(e);
		delete *it;
		m_edges.erase(it);
	}
		
}

void Graph::removeVerticle(Verticle* v)
{
	assert(v);
	auto it = std::find(m_verticles.begin(), m_verticles.end(), v);

	if (it != m_verticles.end())
	{
		auto edges_list = v->getEdgesList();
		for (auto& edge : edges_list)
			removeEdge(edge);
		pos_to_verticles[(*it)->position()] = NULL;
		delete *it;
		m_verticles.erase(it);
	}
}

void Graph::addEdge(Edge* edge)
{
	edge->beginVerticle()->connectEdge(edge);
	edge->endVerticle()->connectEdge(edge);
	m_edges.push_back(edge);
}

Edge* Graph::addEdge(Verticle* begin, Verticle* end, int value)
{
	Edge* edge = new Edge(begin,end,value);
	addEdge(edge);
	return edge;
}

std::vector<Verticle*> Graph::findPath(Verticle* start, Verticle* finish)
{
	assert(start && finish);

	auto getHeuristicValue = [&finish](Verticle* v) {return abs(v->position().x - finish->position().x) + abs(v->position().y - finish->position().y); };

	std::unordered_map<Verticle*, bool> close_list;
	std::unordered_map<Verticle*, Verticle*> parent_list;
	std::unordered_map<Verticle*, int> value_list;

	auto cmp = [&value_list](Verticle* a, Verticle* b) { return value_list[a] > value_list[b]; };

	std::priority_queue<Verticle*, std::vector<Verticle*>, decltype(cmp)> open_list(cmp);

	open_list.push(start);
	Verticle* current_verticle = NULL;
	while (!open_list.empty())
	{
		current_verticle = open_list.top();
		int current_value = value_list[current_verticle];

		if (current_verticle == finish)
			break;

		open_list.pop();
		//close_list[current_verticle] = true;

		assert(current_verticle);
		auto verticles = current_verticle->getIncidentVerticles();
		for (auto& v : verticles)
			if (!close_list[v])
			{
				parent_list[v] = current_verticle;
				close_list[v] = true;
				int neighbor_value = getEdge(current_verticle, v)->value;
				value_list[v] = (current_value + neighbor_value) + getHeuristicValue(v);
				open_list.push(v);
			}
	}

	std::vector<Verticle*> path;
	if (current_verticle == finish)
	{
		while (current_verticle != start)
		{
			path.push_back(current_verticle);
			current_verticle = parent_list[current_verticle];
		}
	}

	path.push_back(start);
	return path;
}

Edge* Graph::getEdge(Verticle* one, Verticle* two)
{
	for (auto e = one->edges_begin(); e != one->edges_end(); ++e)
		if ((*e)->isAdjacent(two))
			return *e;
	return NULL;

}