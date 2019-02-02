// My NuMVC test source file
//    MuMVC, a heuristic algorithm to solve the Minimum Vertex Cover
// Problem, raised by Cai Shaowei, 2013 in paper `NuMVC: An Efficient
//  Local Search Algorithm for Minimum Vertex Cover`, url as :
// https://arxiv.org/abs/1402.0584.
// Re-implemented by Zachary-ZS.

// #define Local

#include <string.h>
#include <time.h>
#include <cmath>
#include <iostream>
#include <list>
#include <set>
#include <stack>

using namespace std;
#define insert push_back
#define erase remove

/* parameters of the algorithm */
#define MAXV 780
#define MAXE 254000
typedef list<int> Set;

int V, E;  // |V| & |E| in this case

// To be set
//#define RANDOMSEED 23
int cutoff;     // Stop when time reaches cutoff
int threshold;  // Delicate w when \bar{w} reaches threshold
                // NOTICE: Cai set it to 0.5*|V|
float rho;      // forgetting factor

int conf_change[MAXV];
int dscore[MAXV];
int G[MAXV][MAXV] = {0};  // Graph
int edge_weight[MAXE] = {0};
int timer_mod[MAXV];  // Modified time, for vertex choosing
int v_in_c[MAXV] = {0};
int bestv_in_c[MAXV] = {0};
int best_dscv = 0;    // best dsc v in C with oldest time
int tabu_remove = 0;  // tabu removed, which is the last added in last round.

int average_weight = 0;
int delta_weight = 0;

/* Data structures for the problem */
struct Edge {
    int a, b;
    Edge() {}
    Edge(int aa, int bb) : a(aa), b(bb) {}
} edge[MAXE];

Set C;
Set bestC;
Set uncovered;
// linjieTable for vertexs
Set v_to_e[MAXV];

// cover the edge e
void cover(int e) {
    uncovered.erase(e);
}
// uncover the edge e
void uncover(int e) {
    uncovered.insert(e);
}

// Add vertex v into C and update other data accordingly.
void add(int v) {
    C.insert(v);
    v_in_c[v] = 1;
    dscore[v] = -dscore[v];

    Set::iterator pi = v_to_e[v].begin();
    for (; pi != v_to_e[v].end(); pi++) {
        int edgidx = *pi;  // v's pi'th edge
        int neigh = edge[edgidx].a + edge[edgidx].b - v;

        if (!v_in_c[neigh]) {  // neighbor isn't in the cover
                               // then the edge should be added to covered.
            dscore[neigh] -= edge_weight[edgidx];
            conf_change[neigh] = 1;
            cover(edgidx);
        } else {
            // needn't set confchange, cuz it must be 1 for those in C
            dscore[neigh] += edge_weight[edgidx];
        }
    }
}

// remove v from C
void remove(int v) {
    C.erase(v);
    v_in_c[v] = 0;
    dscore[v] = -dscore[v];
    conf_change[v] = 0;

    Set::iterator pi = v_to_e[v].begin();
    for (; pi != v_to_e[v].end(); pi++) {
        int edgidx = *pi;  // v's pi'th edge
        int neigh = edge[edgidx].a + edge[edgidx].b - v;

        if (!v_in_c[neigh]) {  // neighbor isn't in the cover
                               // then the edge should be removed from the
                               // covered.
            dscore[neigh] += edge_weight[edgidx];
            conf_change[neigh] = 1;
            uncover(edgidx);
        } else {
            // needn't set confchange, cuz it must be 1 for those in C
            dscore[neigh] -= edge_weight[edgidx];
        }
    }
}

// Update bestC as C
void update_bestC() {
    // bestC.clear();
    bestC.assign(C.begin(), C.end());
    for (int i = 1; i <= V; i++) {
        bestv_in_c[i] = v_in_c[i];
    }
}

// update best dscore v with oldest time
void update_best_dscv() {
    Set::iterator pi = C.begin();
    best_dscv = *pi++;
    int tmpv;
    for (; pi != C.end(); pi++) {
        tmpv = *pi;
        if (tmpv == tabu_remove)
            continue;
        if (dscore[tmpv] < dscore[best_dscv])
            continue;
        else if (dscore[tmpv] > dscore[best_dscv])
            best_dscv = tmpv;
        // timer_mod less, means earlier modified.
        else if (timer_mod[tmpv] < timer_mod[best_dscv])
            best_dscv = tmpv;
    }
}

// weight forgetting mechanism
void forget_edge_weight(){
    int sum_w = 0;
    memset(dscore, 0, sizeof(dscore));

    for (int i = 1; i <= E; i++) {
        edge_weight[i] = edge_weight[i] * rho;
        sum_w += edge_weight[i];
        
        // update dscore
        // if both vertexs of edge are in C, dscore won't be affected
        // edge's two vertexs are neither in C
        if (v_in_c[edge[i].a] + v_in_c[edge[i].b] == 0) {
            dscore[edge[i].a] += edge_weight[i];
            dscore[edge[i].b] += edge_weight[i];
        }
        // one vertex is in C
        else if (v_in_c[edge[i].a] + v_in_c[edge[i].b] == 1) {
            if (v_in_c[edge[i].a])
                dscore[edge[i].a] -= edge_weight[i];
            else
                dscore[edge[i].b] -= edge_weight[i];
        }
    }
    average_weight = sum_w / E;
}

// update edge weights
void update_edge_weight(){
    Set::iterator pi = uncovered.begin();
    for (; pi != uncovered.end(); pi++) {
        int tmpe = *pi;
        edge_weight[tmpe]++;
        dscore[edge[tmpe].a]++;
        dscore[edge[tmpe].b]++;
        
    }
    delta_weight += uncovered.size();
    if (delta_weight >= E) {
        average_weight++;
        delta_weight -= E;
    }

    // Forget weights if ave_w reaches threshold
    if (average_weight >= threshold) {
        forget_edge_weight();
    }
}

// init the datas for the gragh and find the oroginal C greedily
void init_() {
    average_weight = 1;
    delta_weight = 0;
    for (int i = 1; i <= V; i++) {
        conf_change[i] = 1;
        dscore[i] = 0;
        timer_mod[i] = 0;
        v_in_c[i] = 0;
        bestv_in_c[i] = 0;
    }
    for (int i = 1; i <= E; i++) {
        edge_weight[i] = 1;
        dscore[edge[i].a] += edge_weight[i];
        dscore[edge[i].b] += edge_weight[i];
        // At first, all the edges are uncovered
        uncovered.insert(i);
    }

    // Find a cover greedily
    while (!uncovered.empty()) {
        int best_dscore = 0;
        int best_cnt = 0;
        int best_idxs[MAXV] = {0};
        for (int v = 1; v <= V; v++) {
            if (v_in_c[v])
                continue;
            if (dscore[v] > best_dscore) {
                best_dscore = dscore[v];
                best_idxs[0] = v;
                best_cnt = 1;
            } else if (dscore[v] == best_dscore) {
                // store all the idxs with the highest dscore
                best_idxs[best_cnt++] = v;
            }
        }
        if (best_cnt > 0) {
            add(best_idxs[rand() % best_cnt]);
        }
    }

    update_bestC();
}

// NuMVC algorithm
void solve() {
    init_();
    int step = 0;
    while (step < cutoff) {
        // printf("Step %d \n", step);
        if (uncovered.empty()) {
            // whether to check the size before?-----[TO DO]
            if (C.size() < bestC.size())
                update_bestC();
            // choose a vertex in C with the highest dscore and remove it
            Set::iterator pi = C.begin();
            int best_dscore = -10000000;
            int best_cnt = 0;
            int best_idxs[MAXV] = {0};
            for (; pi != C.end(); pi++) {
                int tmpv = *pi;
                if (dscore[tmpv] > best_dscore) {
                    best_dscore = dscore[tmpv];
                    best_idxs[0] = tmpv;
                    best_cnt = 1;
                } else if (dscore[tmpv] == best_dscore) {
                    best_idxs[best_cnt++] = tmpv;
                }
            }
            // breaking ties randomly
            if (best_cnt > 0) {
                remove(best_idxs[rand() % best_cnt]);
            }
            continue;
        }

        // choose a u \in C with the highest dscore, in favor of the oldest
        update_best_dscv();
        remove(best_dscv);
        // choose an uncovered edge randomly
        int randidx = rand() % (uncovered.size());
        Set::iterator pi = uncovered.begin();
        for (int i = 0; i < randidx; i++)
            pi++;
        int choose_edge = *pi;

        // choose a vertex of choose_edge to be added to C
        int v1 = edge[choose_edge].a, v2 = edge[choose_edge].b;
        int best_addv = 0;
        // cuz choose_edge is uncovered, one of its 2 vertexs must be
        // Confchanged 1.
        if (conf_change[v1] == 0)
            best_addv = v2;
        else if (conf_change[v2] == 0)
            best_addv = v1;
        // Both confchanged 1, choose the one whose dscore's higher which is older
        else {
            if (dscore[v1] > dscore[v2] || (dscore[v1] == dscore[v2] && timer_mod[v1] < timer_mod[v2]))
                best_addv = v1;
            else
                best_addv = v2;
        }
        add(best_addv);
        timer_mod[best_addv] = timer_mod[best_dscv] = step;
        tabu_remove = best_addv;
        update_edge_weight();
        step++;
    }
}

// Check if the res is valid
bool is_VC(){
    for (int i = 1; i <= E; i++) {
        if (bestv_in_c[edge[i].a] == 0 && bestv_in_c[edge[i].b] == 0) {
            //printf("Error!!! The Found C isn't a vertex cover.\n");
            return false;
        }
    }
    return true;
}

// print the result
void print_res(){
    printf("%d\n", V - bestC.size());
    bool firstone = true;
    for (int i = 1; i <= V; i++) {
        if (!bestv_in_c[i]) {
            if (firstone) {
                printf("%d", i);
                firstone = false;
            }
            else {
                printf(" %d", i);
            }
        }
    }
    printf("\n");
}

int main() {

#ifdef Local
    freopen("E:\\VSC-CPP\\MaxClique\\samples\\frb30-1.txt", "r", stdin);
    //freopen("E:\\VSC-CPP\\MaxClique\\result-frb30-1.txt", "w", stdout);
#endif

    // POJ
    srand(unsigned(time(NULL)));
    while (scanf("%d%d", &V, &E) != EOF) {
        // Build the graph
        int aa, bb;
        memset(G, 0, sizeof(G));
        memset(edge, 0, sizeof(edge));
        memset(conf_change, 0, sizeof(conf_change));
        C.clear();
        uncovered.clear();
        bestC.clear();
        for (int i = 1; i <= V; i++)
            v_to_e[i].clear();

        for (int i = 0; i < E; i++) {
            scanf("%d%d", &aa, &bb);
            G[aa][bb] = 1;
            G[bb][aa] = 1;
        }
        int numedge = 1;
        for (int i = 1; i <= V; i++)
            for (int j = i + 1; j <= V; j++)
                if (!G[i][j]) {
                    edge[numedge].a = i;
                    edge[numedge].b = j;
                    v_to_e[i].insert(numedge);
                    v_to_e[j].insert(numedge);
                    numedge++;
                }
        E = numedge - 1;
        // ---Set the parameters here.------------
        cutoff = V * 300;
        threshold = (int)(0.5 * V);
        rho = 0.3;
#ifdef Local
        printf("----------------Parameters:--------------\n");
        printf("      V: %d    E: %d\n    \\rho: %f", V, E, rho);
        printf("    cutoff: %d    threshold: %d\n", cutoff, threshold);
        printf("\n----------------Results:--------------\n");
#endif

        // Search using NuMVC
        solve();
        // Check if the res is valid
        if (is_VC()) {
#ifdef Local
            printf("The found clique size is %d.\n", V - bestC.size());
            printf("The found clique:\n");
#endif
            print_res();
        }
        else{
            printf("0\n0");
        }
    }

    return 0;
}