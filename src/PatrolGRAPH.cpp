//	Copyright (c) 2014, Francesco Wanderlingh. 					//
//	All rights reserved.										//
//	License: BSD (http://opensource.org/licenses/BSD-3-Clause)	//

/*
 * PatrolGRAPH.cpp
 *
 *  Created on: Jun 21, 2014
 *      Author: francescow
 */

#include "PatrolGRAPH.h"
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <numeric>
#include <algorithm>
#include <cassert>
#include <limits>
#include "termColors.h"
#include "Utils.h"

#define STARTNODE 5

using std::cout;
using std::endl;

PatrolGRAPH::PatrolGRAPH() : gridSizeX(0), gridSizeY(0),
    currentNode(STARTNODE), unvisitedCount(std::numeric_limits<int>::max())
{
  // TODO Auto-generated constructor stub

}

PatrolGRAPH::~PatrolGRAPH()
{
  // TODO Auto-generated destructor stub
}

void PatrolGRAPH::loadMatrixFile(std::ifstream &access_mat){

  if( access_mat.is_open() ) {
    int val;
    int num_nl = 0;
    while( access_mat >> val ){
      if(access_mat.peek() == '\n') num_nl++;
      access_vec.push_back( val );
    }

    access_mat.close();

    gridSizeX = num_nl;
    gridSizeY = access_vec.size()/num_nl;

  }
  else{ cout << "Error reading file!" << endl; }

}


void PatrolGRAPH::initGraph(std::ifstream & INFILE){

  srand(time(NULL) xor getpid()<<16);
  loadMatrixFile(INFILE);       /// Filling the "access_vec" and defining grid sizes

  //cout << "Matrix size is: " << gridSizeX << "x" << gridSizeY << endl;

  graphNodes.resize(gridSizeX*gridSizeY);
  unvisited.resize(gridSizeX*gridSizeY, 0);

  // Graph initialisation - to every node is assigned a position in a Row-Major order
  for(int i=0; i<gridSizeX; i++){
    for(int j=0; j<gridSizeY; j++){
      graphNodes.at((i*gridSizeY) + j).setPos((float)i, (float)j);
      graphNodes.at((i*gridSizeY) + j).occupied = access_vec.at((i*gridSizeY) + j);

      if(access_vec.at((i*gridSizeY) + j) == 0)  unvisited.at((i*gridSizeY) + j) = 1;
      // cout << (int)graphNodes.at((i*gridSizeY) + j).occupied << " ";
    }
    //cout << endl;
  }
  unvisitedCount = std::accumulate(unvisited.begin(),unvisited.end(), 0);

  createEdgeMat();
  computeProbabilityMat();
}


void PatrolGRAPH::createEdgeMat(){

  const int n = gridSizeX*gridSizeY;

  assert(n == access_vec.size());
  /// Here we create a zero matrix of the size of the graph
  vector<int> _graph(n, 0);
  vector< vector<int> > __graph(n, _graph);
  graph = __graph;
  edgeCountMat = __graph;
  int row, col;    // Main indexes
  int row_shift, col_shift; // To move around spatial adjacents
  int nb_row, nb_col;       // Adjacent indexes
  /*
    printf("%sOccupancy Map:%s",TC_RED, TC_NONE);
    for(int j=0; j<n; j++){
      if(j%gridSizeY == 0) cout << endl;
      if( access_vec[j] == 1 ){
        printf("%s",TC_RED);
        Utils::spaced_cout(j);
        printf("%s", TC_NONE);
      }
      else Utils::spaced_cout(j);
    }
    cout << endl << endl;
   */

  for(int i=0; i<n; i++){
    if(access_vec[i] == 0){
      row = i/gridSizeY;
      col = i%gridSizeY;
      for(row_shift=-1; row_shift<=1; row_shift++){
        for(col_shift=-1; col_shift<=1; col_shift++){
          nb_row = row + row_shift;
          nb_col = col + col_shift;
          if( (nb_row>=0) && (nb_row<gridSizeX) && (nb_col>=0) && (nb_col<gridSizeY) ///RANGE CHECK
              && (row_shift!=0 || col_shift!=0)  ///<--- don't check same node of current
              //&& (row_shift*col_shift == 0)  ///<--- don't allow diagonal movements
          )
            //&& ( (row_shift*row_shift xor col_shift*col_shift)==1 ) <--last 2 statements compressed in one condition
          {
            if(access_vec[nb_row*gridSizeY+nb_col] == 0){
              /// Create the edge between "i" and its "free neighbour"
              graph[i][nb_row*gridSizeY+nb_col] = 1;
              graph[nb_row*gridSizeY+nb_col][i] = 1;
            }
          }
        }
      }
    }
  }


}

void PatrolGRAPH::computeProbabilityMat(){
  /**
   * Here, starting from the graph's edges matrix we derive the Probability
   * Transition Matrix (PTM), where p_ij = 1/|A_i| (A_i=number of incident edges).
   */

  const int n = gridSizeX*gridSizeY;
  vector<double> _temp_PTM(graph.size(), 0);
  vector< vector<double> > temp_PTM(graph.size(), _temp_PTM);


  double num_edges = 0;
  for(int i=0; i<graph.size(); i++){
    num_edges = std::accumulate(graph.at(i).begin(), graph.at(i).end(), 0.0);
    if(num_edges != 0){
      std::transform(graph.at(i).begin(), graph.at(i).end(),
                     temp_PTM.at(i).begin(), std::bind2nd(std::divides<double>(),num_edges));
    }
  }

  PTM = temp_PTM;
  /*
  cout << "\nPTM:\n";
  for(int i=0;i<PTM.size();i++){
    for(int j=0; j<PTM.size();j++){
      if(PTM[i][j] == 0) printf("   0  ");
      else printf("%.2f  ",PTM[i][j]);;
    }
    cout << endl;
  }
   */
}


void PatrolGRAPH::incrCount(int currIndex, bool currType, boost::array<int, 2> chosenEdge){

  /// Increment vertex count
  graphNodes.at(currIndex).nodeCount++;

  /// Increment edge count
  edgeCountMat[chosenEdge.at(0)][chosenEdge.at(1)]++;


  if(currType == 0){
    unvisited.at(currType) = 0;
    // The sum of all elements of unvisited is performed so that when sum is zero we
    // know we have finished. Check is performed in "findNext()"
    unvisitedCount = std::accumulate(unvisited.begin(),unvisited.end(), 0);
  }


  /*
 //PRINT MAP FOR DEBUGGING PURPOSES

  for(int i=0; i<gridSizeX; i++){
    for(int j=0; j<gridSizeY; j++){
      cout << (int)graphNodes.at((i*gridSizeY) + j).nodeCount << " ";
    }
    cout << endl;
  }

  cout << "====================================" << endl;
   */
}


/// *** MAIN METHOD *** ///
void PatrolGRAPH::findNext(){

  if( !isCompleted() ){
    /// Look for adjacent nodes and find the one with the smallest number of visits
    /// Before being able to do the adjacency check we have to recover the (i,j) index
    /// values encoded in the graph 1D array as "i*gridSizeY + j" (row-major order)

    ///The "i" index of the chosen edge will be the one of the node we're on now = current node
    chosenEdge.at(0) = currentNode;
    std::vector<int> best_vec;

    double deltaP_best = std::numeric_limits<double>::max();

    int current_i = currentNode/gridSizeY;
    int current_j = currentNode%gridSizeY;

    for(int i_shift=-1; i_shift<=1; ++i_shift){
      for(int j_shift=-1; j_shift<=1; ++j_shift){

        int tent_i = current_i + i_shift;
        int tent_j = current_j + j_shift;

        if( (tent_i>=0) && (tent_i<gridSizeX) && (tent_j>=0) && (tent_j<gridSizeY) && ///RANGE CHECK
            (i_shift!=0 || j_shift!=0)  ///<--- don't check same node of current
            && (i_shift*j_shift == 0) ///<--- don't check oblique nodes
        )
        {
          int tentIndex = tent_i*gridSizeY + tent_j;

          if( (graphNodes.at(tentIndex).occupied == 0) ){   ///OCCUPANCY CHECK


            double deltaP = abs( ((double)edgeCountMat[currentNode][tentIndex]/(double)graphNodes.at(tentIndex).nodeCount)
                                   - PTM[currentNode][tentIndex] );

            if(deltaP <= deltaP_best){
              if(deltaP == deltaP_best){
                best_vec.push_back(tentIndex);
              }else{
                best_vec.clear();
                best_vec.push_back(tentIndex);
              }
              deltaP_best = deltaP;
            }//End checkBest
          }//End occupancy check
        }//End range check
      }//End j_shift for-loop (y scan)
    } //End i_shift for-loop (x scan)

    /// Now if there is more than one element in the vector choose one randomly.
    /// If size()==1 the modulus function always returns 0 (the first element)


    assert(best_vec.size() != 0);
    int randIndex = rand()%best_vec.size();
    /// The "j" index of the chosen edge will be the one of the (best) target node,
    /// that now also becomes the new current node.
    chosenEdge.at(1) = currentNode = best_vec.at(randIndex);

    finalPath.push_back(currentNode);
  }
}

bool PatrolGRAPH::getCurrentType(){

  if(graphNodes.at(currentNode).nodeCount == 0)
    return 0;
  else
    return 1;
}

float PatrolGRAPH::getCurrentCoord(char coordinate){
  switch(coordinate){
    case 'x':
      return graphNodes.at(currentNode).posx;
      break;
    case 'y':
      return graphNodes.at(currentNode).posy;
      break;
    /// 'z' for now is omitted on purpose, since the height depends on the robot
    /// (check quadcopterRosCtrl.cpp or quadNodeCount.cpp for further details)
  }

}



bool PatrolGRAPH::isCompleted(){

  if(unvisitedCount == 0)
    return true;
  else
    return false;
}