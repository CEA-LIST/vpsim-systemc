// Dijkstra's Algorithm ( Simple C++ Implementation)
// -------------------------------------------------
// Dijkstra's algorithm is simply BFS + priority queue and includes a relaxation step.
// For details please refer to Introduction to algorithm by Cormen.
// Complete c++ code with a test code is given below.

#include "dijkstra.hpp"
using namespace std;

using namespace vpsim;

int Comparator::operator() ( const pair<int,float>& p1, const pair<int,float> &p2)
{
    return p1.second>p2.second;
};


void dijkstra(const Graph  &G,const int &source,const int &destination,vector<int> &path)
{
    vector<float> d(G.size());
    vector<int> parent(G.size());
    for(unsigned int i = 0 ;i < G.size(); i++)
    {
        d[i] = std::numeric_limits<float>::max();
        parent[i] = -1;
    }
    priority_queue<pair<int,float>, vector<pair<int,float> >, Comparator> Q;
    d[source] = 0.0f;
    Q.push(make_pair(source,d[source]));
    while(!Q.empty())
    {
        int u = Q.top().first;
        if(u==destination) break;
        Q.pop();
        for(unsigned int i=0; i < G[u].size(); i++)
        {
            int v= G[u][i].first;
            float w = G[u][i].second;
            if(d[v] > d[u]+w)
            {
                d[v] = d[u]+w;
                parent[v] = u;
                Q.push(make_pair(v,d[v]));
            }
        }
    }
    path.clear();
    int p = destination;
    path.push_back(destination);
    while(p!=source)
    {
        p = parent[p];
        path.push_back(p);
    }
}

// int main()
// {
//     /* Graph
//     GRAPH TYPE = UNDIRECTED
//     NUMBER OF VERTICES = 6 indexed from 0 to 5
//     NUMBER OF EDGES = 9
//     edge 0->1 weight = 7
//     edge 0->2 weight = 9
//     edge 0->5 weight = 14
//     edge 1->2 weight = 10
//     edge 1->3 weight = 15
//     edge 2->5 weight = 2
//     edge 2->3 weight = 11
//     edge 3->4 weight = 6
//     edge 4->5 weight = 9
//     */
//     Graph g;
//     g.resize(6);
//     g[0].push_back(make_pair(1,7));
//     g[1].push_back(make_pair(0,7));

//     g[0].push_back(make_pair(2,9));
//     g[2].push_back(make_pair(0,9));

//     g[0].push_back(make_pair(5,14));
//     g[5].push_back(make_pair(0,14));

//     g[1].push_back(make_pair(2,10));
//     g[2].push_back(make_pair(1,10));

//     g[1].push_back(make_pair(3,15));
//     g[3].push_back(make_pair(1,15));

//     g[2].push_back(make_pair(5,2));
//     g[5].push_back(make_pair(2,2));

//     g[2].push_back(make_pair(3,11));
//     g[3].push_back(make_pair(2,11));

//     g[3].push_back(make_pair(4,6));
//     g[4].push_back(make_pair(3,6));

//     g[4].push_back(make_pair(5,9));
//     g[5].push_back(make_pair(4,9));
//     vector<int> path;
//     dijkstra(g,0,4,path);
//     for(int i=path.size()-1;i>=0;i--)
//         cout<<path[i]<<"->";
//     return 0;
// }

/*
Description:
Graph is represented as adjacency List.
g.size(): total number of vertices in the graph.
g[i].size(): number of neighbors of vertex with index i.
g[i][j].first : index of jth  neighbor of vertex with index i.
g[i][j].second: distance from vertex i to j.
source: index of source vertex
destination: index of destination vertex
path: path from source to destination is stored in path vector.
Comparator: a class used two compare two vertices to maintain a priority. When we pop a vertex from priority queue the vertex with minimum d value is extracted.

This is code was tested on g++.
Worst case time complexity -O(E + |V*V|)
The code can be optimized(O(E+V*log(V)) using Fibonacci heap implementation for priority Queue.

*/
