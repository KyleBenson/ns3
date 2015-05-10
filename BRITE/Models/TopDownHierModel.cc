/****************************************************************************/
/*                  Copyright 2001, Trustees of Boston University.          */
/*                               All Rights Reserved.                       */
/*                                                                          */
/* Permission to use, copy, or modify this software and its documentation   */
/* for educational and research purposes only and without fee is hereby     */
/* granted, provided that this copyright notice appear on all copies and    */
/* supporting documentation.  For any other uses of this software, in       */
/* original or modified form, including but not limited to distribution in  */
/* whole or in part, specific prior permission must be obtained from Boston */
/* University.  These programs shall not be used, rewritten, or adapted as  */
/* the basis of a commercial software or hardware product without first     */
/* obtaining appropriate licenses from Boston University.  Boston University*/
/* and the author(s) make no representations about the suitability of this  */
/* software for any purpose.  It is provided "as is" without express or     */
/* implied warranty.                                                        */
/*                                                                          */
/****************************************************************************/
/*                                                                          */
/*  Author:    Alberto Medina                                               */
/*             Anukool Lakhina                                              */
/*  Title:     BRITE: Boston university Representative Topology gEnerator   */
/*  Revision:  2.0         4/02/2001                                        */
/****************************************************************************/
/****************************************************************************/
/*                                                                          */
/*  Modified lightly to easily interface with ns-3                          */
/*  Author:     Josh Pelkey <jpelkey@gatech.edu>                            */
/*  Date: 3/02/2011                                                         */
/****************************************************************************/
#pragma implementation "TopDownHierModel.h"

#include "TopDownHierModel.h"
#include "RouterWaxmanModel.h"
#include "RouterBarabasiAlbertModel.h"
#include "ASWaxmanModel.h"
#include "ASBarabasiAlbertModel.h"
#include "ImportedFileModel.h"

using namespace std;
namespace brite {

TopDownHierModel::TopDownHierModel(TopDownPar* par) : models(2) {

  ASWaxman* as_wax_model;
  ASBarabasiAlbert* as_bar_model;
  RouterWaxman* rt_wax_model;
  RouterBarabasiAlbert* rt_bar_model;
  ImportedBriteTopologyModel* if_brite_model;
  ImportedGTitmTopologyModel* if_gtitm_model;  
  ImportedNLANRTopologyModel* if_nlanr_model;

  edge_conn_type = (EdgeConnType)par->GetEC();
  k = par->GetK();
  m_edges = par->GetM();
  type = TD_HIER;

  cout << "Creating TD model...\n" << flush;
  switch (((ModelPar*)(par->GetModelPar(0)))->GetModelType()) {
  case AS_WAXMAN:
    as_wax_model = new ASWaxman((ASWaxPar*)(par->GetModelPar(0)));
    models[0] = as_wax_model;
    break;

  case AS_BARABASI:
    as_bar_model = new ASBarabasiAlbert((ASBarabasiAlbertPar*)(par->GetModelPar(0)));
    models[0] = as_bar_model;
    break;

  case IF_AS:
    
    switch (((ImportedFilePar*)(par->GetModelPar(0)))->GetFormat()) {
    case ImportedFileModel::IF_BRITE:
      cout << "Importing brite...\n" << flush;
      if_brite_model = new ImportedBriteTopologyModel((ImportedFilePar*)(par->GetModelPar(0)));
      models[0] = if_brite_model;
      break;

    case ImportedFileModel::IF_GTITM:
    case ImportedFileModel::IF_GTITM_TS:
      cout << "Importing gtitm...\n" << flush;
      if_gtitm_model = new ImportedGTitmTopologyModel((ImportedFilePar*)(par->GetModelPar(0)));
      models[0] = if_gtitm_model;
      break;

    case ImportedFileModel::IF_NLANR:
      cout << "Importing nlanr..\n" << flush;
      if_nlanr_model = new ImportedNLANRTopologyModel((ImportedFilePar*)(par->GetModelPar(0)));
      models[0] = if_nlanr_model;
      break;
      
    case ImportedFileModel::IF_SKITTER:
      cerr << "Skitter model will be available soon...\n" << flush;
      exit(0);
      
    default:
      cerr << "TopDownHierModel(): Invalid file format for ImportedFileModel...\n" << flush;
      exit(0);
    }
    break;

  default:
    cerr << "Invalid model type for TD model...\n" << flush;
    assert(0);
  }

  switch (par->GetModelPar(1)->GetModelType()) {
  case RT_WAXMAN:
    rt_wax_model = new RouterWaxman((RouterWaxPar*)(par->GetModelPar(1)));
    models[1] = rt_wax_model;
    break;

  case RT_BARABASI:
    rt_bar_model = new RouterBarabasiAlbert((RouterBarabasiAlbertPar*)(par->GetModelPar(1)));
    models[1] = rt_bar_model;
    break;

  case IF_ROUTER:

    switch (((ImportedFilePar*)(par->GetModelPar(1)))->GetFormat()) {
    case ImportedFileModel::IF_BRITE:
      cout << "Importing brite...\n" << flush;
      if_brite_model = new ImportedBriteTopologyModel((ImportedFilePar*)(par->GetModelPar(1)));
      models[1] = if_brite_model;
      break;

    case ImportedFileModel::IF_GTITM:
      cout << "Importing gtitm...\n" << flush;
      if_gtitm_model = new ImportedGTitmTopologyModel((ImportedFilePar*)(par->GetModelPar(1)));
      models[1] = if_gtitm_model;
      break;

    case ImportedFileModel::IF_NLANR:
      cout << "Importing nlanr..\n" << flush;
      if_nlanr_model = new ImportedNLANRTopologyModel((ImportedFilePar*)(par->GetModelPar(1)));
      models[1] = if_nlanr_model;
      break;
      
    case ImportedFileModel::IF_SKITTER:
      cerr << "Skitter model will be available soon...\n" << flush;
      exit(0);
      
    default:
      cerr << "BottomUpHierModel(): Invalid file format for ImportedFileModel...\n" << flush;
      exit(0);
    }
    break;

  default:
    cerr << "Invalid model type for TD model...\n" << flush;
    assert(0);
  }
}


string TopDownHierModel::ToString() {
  
  ostringstream os;
  string s;

  os << "Model ( 5 ): " 
     << (int)edge_conn_type << " "
     <<  k  << " "
     << Scale_1 << " "
     << Scale_2 << "\n";

  os << "AS Level: ";
  switch (models[0]->GetType()) {
  case AS_WAXMAN:
    os  << ((ASWaxman*)models[0])->ToString();
    break;
  case AS_BARABASI:
    os  << ((ASBarabasiAlbert*)models[0])->ToString();
    break;
  case IF_AS:
    os  << ((ImportedFileModel*)models[0])->ToString();
    break;

  default:
    cerr << "TopDownHierModel::ToString(): Invalid AS model type...\n" << flush;
    exit(0);

  }

  os << "Router Level: ";
  switch (models[1]->GetType()) {
  case RT_WAXMAN:
    os  << ((RouterWaxman*)models[1])->ToString() << '\0';
    break;
  case RT_BARABASI:
    os  << ((RouterBarabasiAlbert*)models[1])->ToString()  << '\0';
    break;
  case IF_ROUTER:
    os  << ((ImportedFileModel*)models[1])->ToString()  << '\0';
    break;

  default:
    cerr << "TopDownHierModel::ToString(): Invalid Router model type...\n" << flush;
    exit(0);

  }

  return string(os.str());

}


Graph* TopDownHierModel::Generate() {

  cout << "Generating Top Down hierarchical topology...\n" << flush;

  /* Generate topology according to model for level 0 */
  Graph* graph = models[0]->Generate();

  /* Check for graph connectivity */
  vector<Color> color(graph->GetNumNodes());
  vector<int> pi(graph->GetNumNodes());

  for (int i = 0; i < graph->GetNumNodes(); i++) {
    color[i] = WHITE;
  }

  graph->DFS(color, pi, 0);

  int conn = 1;
  for (int i = 0; i < graph->GetNumNodes(); i++) {
    if (color[i] == WHITE) {
      conn = 0;
    }
  }
  assert(conn);

  try {

    for (int i = 0; i < graph->GetNumNodes(); i++) {
      cout << "Generating " << i + 1 << "th Router/level topology...\n" << flush;
      assert(graph->GetNodePtr(i)->GetNodeInfo()->GetNodeType() == NodeConf::AS_NODE);
      Topology* new_topology = new Topology(models[1]);
      assert(new_topology != NULL);

      // We need to make sure the topology is connected, but let's also
      // make sure that we don't loop here forever trying to do so
      int remainingTries = 100;
      while (!new_topology->IsConnected() and remainingTries-- > 0)
      {
        cout << "Topology not connected, retrying generation..." << endl << flush;
        delete new_topology;
        new_topology = new Topology(models[1]);
      }
      assert(new_topology->IsConnected());
      ((ASNodeConf*)graph->GetNodePtr(i)->GetNodeInfo())->SetTopology(new_topology, i);   
    }
  }
  catch (bad_alloc) {
    cerr << "ToDownHier::Generate(): Cannot allocate router-level topologies...\n" << flush; 
    exit(0);
  }

  cout << "Flattening topology...\n" << flush;
  Graph* flat_graph = FlattenGraph(graph);

  cout << "Interconnecting Border routers...\n" << flush;
  InterConnectBorders(graph, flat_graph);
  
  delete graph;

  /* Returns graph pointer to flattened topology */
  return flat_graph;
  
}


Graph* TopDownHierModel::FlattenGraph(Graph* g) {

  int n = 0;
  Topology* t;
  Graph* new_graph;
  
  for (int i = 0; i < g->GetNumNodes(); i++) {
    t = ((ASNodeConf*)(g->GetNodePtr(i)->GetNodeInfo()))->GetTopology();
    n += t->GetNumNodes();
  }

  try {
    cout << "Size of Flattened topology: " << n << "\n" << flush;
    new_graph = new Graph(n);
  }
  catch (bad_alloc) {
    cerr << "TopDownHier::FlattenGraph(): Cannot allocate new graph...\n" << flush;
    exit(0);
  }

  int index = 0;
  for (int i = 0; i < g->GetNumNodes(); i++) {

    t = ((ASNodeConf*)(g->GetNodePtr(i)->GetNodeInfo()))->GetTopology();
    assert(t != NULL);

    for (int j = 0; j < t->GetNumNodes(); j++) {

      BriteNode* node = t->GetGraph()->GetNodePtr(j);
      ((RouterNodeConf*)(node->GetNodeInfo()))->SetASId(i);
      new_graph->AddNode(node, index);
      node->SetId(index);
      index += 1;

    }
  }

  for (int i = 0; i < g->GetNumNodes(); i++) {

    assert(g->GetNodePtr(i)->GetNodeInfo()->GetNodeType() == NodeConf::AS_NODE);

    /* Grab topology associated with AS i */
    t = ((ASNodeConf*)(g->GetNodePtr(i)->GetNodeInfo()))->GetTopology();
    assert(t != NULL);

    list<Edge*>::iterator el;
    for (el = t->GetGraph()->edges.begin(); el != t->GetGraph()->edges.end(); el++) {
    
      Edge* edge = *el;
      assert(edge != NULL);
      new_graph->AddEdge(edge);
      new_graph->AddAdjListNode(edge->GetSrc()->GetId(), edge->GetDst()->GetId());
      new_graph->AddAdjListNode(edge->GetDst()->GetId(), edge->GetSrc()->GetId());
      
    }
  }
  
  return new_graph;

}


void TopDownHierModel::InterConnectBorders(Graph* g, Graph* flat_g) {

  int ASFrom, ASTo, num_nodes_i, n1, n2;
  BriteNode *Src, *Dst;
  Edge* edge;
  vector<int> positions(g->GetNumNodes());
  RandomVariable U(s_edgeconn);

  positions[0] = 0;
  for (int i = 1; i < g->GetNumNodes(); i++) {
      
      num_nodes_i = ((ASNodeConf*)g->GetNodePtr(i)->GetNodeInfo())->GetTopology()->GetNumNodes();
      positions[i] = positions[i - 1] + num_nodes_i;
      
  }

  list<Edge*>::iterator el;
  for (el = g->edges.begin(); el != g->edges.end(); el++) {
      
    assert((*el)->GetSrc()->GetNodeInfo()->GetNodeType() == NodeConf::AS_NODE);
    assert((*el)->GetDst()->GetNodeInfo()->GetNodeType() == NodeConf::AS_NODE);
    ASFrom = (*el)->GetSrc()->GetId();
    ASTo = (*el)->GetDst()->GetId();
    double edgebw = (*el)->GetConf()->GetBW(); 

    switch (edge_conn_type) {
	
    case TD_RANDOM: 
      n1 = GetFlatRandomNode(ASFrom, g, flat_g, positions, U);
      n2 = GetFlatRandomNode(ASTo, g, flat_g, positions, U);
      break;
      
    case TD_SMALLEST:
      
      n1 = GetFlatSmallest(ASFrom, g, flat_g, positions);
      n2 = GetFlatSmallest(ASTo, g, flat_g, positions);
      break;
	
    case TD_SMALLEST_NOLEAF:
	
      n1 = GetFlatSmallestNoLeaf(ASFrom, g, flat_g, positions);
      n2 = GetFlatSmallestNoLeaf(ASTo, g, flat_g, positions);
      break;
      
    case TD_K_DEGREE:
      
      n1 = GetFlatSmallestK(ASFrom, g, flat_g, positions);
      n2 = GetFlatSmallestK(ASTo, g, flat_g, positions);
      break;
      
    default:
      cerr << "Invalid Edge Connection method...\n" << flush;
      exit(0);
    }

    Src =  flat_g->GetNodePtr(n1);
    Dst =  flat_g->GetNodePtr(n2);
    assert(Src != NULL && Dst != NULL);
    
    /* Set nodes as Border routers */
    //((RouterNodeConf*)(Src->GetNodeInfo()))->SetRouterType(RouterNodeConf::RT_BORDER);
    //((RouterNodeConf*)(Dst->GetNodeInfo()))->SetRouterType(RouterNodeConf::RT_BORDER);
    
    /* Add new edge */
    try {
      edge = new Edge(Src, Dst);
      edge->SetDirection((*el)->GetDirection());
      RouterEdgeConf* re_conf = new RouterEdgeConf(edge->Length());
      re_conf->SetBW(((ASEdgeConf*)((*el)->GetConf()))->GetBW());
      re_conf->SetEdgeType(EdgeConf::RT_EDGE);
      re_conf->SetBW(edgebw);
      edge->SetConf(re_conf);
      flat_g->AddEdge(edge);
    }
    catch (bad_alloc) {
      cerr << "TopDown::InterconnectBorders(): Cannot allocate new edege...\n" << flush;
    }
      
    /* Update adjacency lists */
    flat_g->AddAdjListNode(n1,n2);
    flat_g->AddAdjListNode(n2,n1);

  }
}


int 
TopDownHierModel::GetFlatRandomNode(int ASid, Graph* g, Graph* flat_g, vector<int>& p, RandomVariable& U) 
{
    
    int n;

    if (ASid < g->GetNumNodes() - 1) {
	n = (int)floor(U.GetValUniform(p[ASid], p[ASid + 1]));
    }else {
	n = (int)floor(U.GetValUniform(p[ASid], flat_g->GetNumNodes()));
    }

    return n;

}

int 
TopDownHierModel::GetFlatSmallest(int ASid, Graph* g, Graph* flat_g, vector<int>& p)
{
    int up;

    if (ASid < g->GetNumNodes() - 1) {
	up = p[ASid + 1];
    }else {
	up = flat_g->GetNumNodes();
    }

    int n, smallest = MAXINT;
    for (int i = p[ASid]; i < up; i++) {
	BriteNode* node = flat_g->GetNodePtr(i);
	if (node->GetOutDegree() < smallest) {
	    n = i;
	    smallest = node->GetOutDegree();
	}
    }   

    return n;

}


int 
TopDownHierModel::GetFlatSmallestNoLeaf(int ASid, Graph* g, Graph* flat_g, vector<int>& p)
{
    int up;
    
    if (ASid < g->GetNumNodes() - 1) {
	up = p[ASid + 1];
    }else {
	up = flat_g->GetNumNodes();
    }

    int min = MAXINT;
    for(int i = 0; i < flat_g->GetNumNodes(); i++ ){
	BriteNode* node = flat_g->GetNodePtr(i);
	if (node->GetOutDegree() < min) {
	    min = node->GetOutDegree();
	}
    }
	
    int n, smallest = MAXINT;
    for (int i = p[ASid]; i < up; i++) {
      BriteNode* node = flat_g->GetNodePtr(i);
      if ((node->GetOutDegree() < smallest) && 
	  (node->GetOutDegree() > min)) {
	n = i;
	smallest = node->GetOutDegree();
      }
    }   
    
    return n;
    
}

int 
TopDownHierModel::GetFlatSmallestK(int ASid, Graph* g, Graph* flat_g, vector<int>& p)
{
    int up;
    
    if (ASid < g->GetNumNodes() - 1) {
      up = p[ASid + 1];
    }else {
      up = flat_g->GetNumNodes();
    }

    int n, smallest = MAXINT;
    for (int i = p[ASid]; i < up; i++) {
      BriteNode* node = flat_g->GetNodePtr(i);
      if ((node->GetOutDegree() < smallest) && 
	  (node->GetOutDegree() > k)) {
	n = i;
	smallest = node->GetOutDegree();
      }
    }   
    
    return n;
}

} // namespace brite