/*
 * Copyright (C) 2024 Commissariat à l'énergie atomique et aux énergies alternatives (CEA)

 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at

 *    http://www.apache.org/licenses/LICENSE-2.0 

 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

#ifndef DIJKSTRA_HPP
#define DIJKSTRA_HPP


// Dijkstra's Algorithm ( Simple C++ Implementation)
// -------------------------------------------------
// Dijkstra's algorithm is simply BFS + priority queue and includes a relaxation step.
// For details please refer to Introduction to algorithm by Cormen.
// Complete c++ code with a test code is given below.

#include <iostream>
#include <vector>
#include <algorithm>
#include <limits>
#include <queue>
using namespace std;

namespace vpsim
{

//!
//! graph structure used for the dijkstra algorithm
//! it is a vector with the length of all Node length, of vectors representing all out edges of each node
//! an edge being represented as a pair (successor node , distance to node)
//!
typedef vector<vector<pair<int,float> > > Graph;

//!
//! Class used as functor to provide comparison used in STL sorting
//!
class Comparator
{
public:
	//!
	//! Distance comparison operator between second element element of pair
	//! @param [in] p1 : a pair (int,float) whose second element will be used for comparison with p2
	//! @param [in] p2 : a pair (int,float) whose second element will be used for comparison with p1
	//! @return true if p1.second>p2.second and false otherwise
	//!
    int operator() ( const pair<int,float>& p1, const pair<int,float> &p2);
};


//!
//! Disjktra shortest path algorithm
//! @param [in] G : A representation of a graph as Graph
//! @param [in] source : a source node whose shortest path to destination node is searched
//! @param [in] destination : a destination node
//! @param [in,out] path : as input an empty vector, as output a vector of successive node IDs representing the shortest path from @param source to @param destination
//!
void dijkstra(const Graph  &G,const int &source,const int &destination,vector<int> &path);

};//namespace vpsim


#endif //DIJKSTRA_HPP
