#ifndef  GRAPHS_H
#define GRAPHS_H

#include "Geometry.h"

class Verticle;

class Edge
{
public:
	Edge(Verticle* begin, Verticle* end, int value);
	int value;
	Verticle* beginVerticle();
	Verticle* endVerticle();
	void setBegin(Verticle* verticle);
	void setEnd(Verticle* verticle);
	bool isAdjacent(Verticle* verticle);
private:
	Verticle* m_begin;
	Verticle* m_end;
};

class Verticle
{
public:
	void connectEdge(Edge* edge);
	void disconnectEdge(Edge* edge);
	std::vector<Edge*>::const_iterator edges_begin() const;
	std::vector<Edge*>::const_iterator edges_end() const;
	std::vector<Verticle*> getIncidentVerticles();
	bool isAdjacent(Edge* edge);
	void clearEdges();
	std::vector<Edge*> getEdgesList() const;
	Vector position() const;
	void setPosition(const Vector& vec);
private:
	Vector m_position;
	std::vector<Edge*> m_edges;
};

class Graph
{
public:
	Graph() {};
	Graph(const Graph& graph) = delete;
	Graph& operator=(const Graph& graph) = delete;
	Graph(Graph&& graph) = delete;
	Graph& operator=(Graph&& graph) = delete;
	void clear();
	Verticle* addVerticle(const Vector& pos);
	Verticle* getVerticleByPos(const Vector& pos);
	Verticle* getVerticleByPos(float x, float y);
	Edge* getEdge(Verticle* one, Verticle* two);
	void removeEdge(Edge* e);
	void removeVerticle(Verticle* v);
	Edge* addEdge(Verticle* begin, Verticle* end, int value);
	void addEdge(Edge* edge);
	std::vector<Verticle*> findPath(Verticle* start, Verticle* finish);
private:
	std::unordered_map<Vector, Verticle*> pos_to_verticles;
	std::vector<Verticle*> m_verticles;
	std::vector<Edge*> m_edges;
};

int getLength(const std::vector<Verticle*>& path);
int getLength(const std::vector<Vector>& path);

#endif
